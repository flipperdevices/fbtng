#include "canvas.h"
#include "canvas_i.h"
#include <nema_vg_safe.h>
#include "font/fonts.h"
#include "font/font_render.h"

static Canvas* instance = NULL;

#define FB_WIDTH 128
#define FB_HEIGHT 64
#define FB_FORMAT NEMA_RGB332
#define FB_FORMAT_SIZE 1
#define FB_STRIDE -1

static const uint8_t* fonts[] = {
    [FontPrimary] = u8g2_font_helvB08_tr,
    [FontSecondary] = u8g2_font_haxrcorp4089_tr,
    [FontKeyboard] = u8g2_font_profont11_mr,
    [FontBigNumbers] = u8g2_font_profont22_tn,
};

static const uint8_t* canvas_font_get(Font font) {
    if(font >= sizeof(fonts) / sizeof(fonts[0])) {
        furi_crash("Invalid font");
    }

    return (const uint8_t*)fonts[font];
}

struct Canvas {
    CanvasOrientation orientation;
    int32_t offset_x;
    int32_t offset_y;
    size_t width;
    size_t height;
    Color color;
    U8G2FontRender font_render;

    CanvasCallbackPairArray_t canvas_callback_pair;
    FuriMutex* mutex;

    uint8_t* dest;
    nema_cmdlist_t cl;
};

static uint32_t canvas_get_color(Canvas* canvas) {
    switch(canvas->color) {
    case ColorWhite:
        return nema_rgba(235, 134, 56, 0xff);
    case ColorBlack:
    default:
        return nema_rgba(0, 0, 0, 0xff);
    }
}

static uint32_t canvas_get_bg_color(Canvas* canvas) {
    switch(canvas->color) {
    case ColorWhite:
        return nema_rgba(0, 0, 0, 0xff);
    case ColorBlack:
    default:
        return nema_rgba(235, 134, 56, 0xff);
    }
}

static void canvas_draw_pixel_fg(int32_t x, int32_t y, void* context) {
    Canvas* canvas = (Canvas*)context;
    x += canvas->offset_x;
    y += canvas->offset_y;
    nema_fill_rect(x, y, 1, 1, canvas_get_color(canvas));
}

static void canvas_draw_pixel_bg(int32_t x, int32_t y, void* context) {
    Canvas* canvas = (Canvas*)context;
    x += canvas->offset_x;
    y += canvas->offset_y;
    nema_fill_rect(x, y, 1, 1, canvas_get_bg_color(canvas));
}

static void canvas_alloc_dest(Canvas* canvas, size_t width, size_t height) {
    if(canvas->dest != NULL) {
        free(canvas->dest);
    }
    canvas->dest = malloc(width * height * FB_FORMAT_SIZE);
}

static uintptr_t canvas_get_dest(Canvas* canvas) {
    return (uintptr_t)canvas->dest;
}

static void canvas_cb_lock(Canvas* canvas) {
    furi_assert(canvas);
    furi_check(furi_mutex_acquire(canvas->mutex, FuriWaitForever) == FuriStatusOk);
}

static void canvas_cb_unlock(Canvas* canvas) {
    furi_assert(canvas);
    furi_check(furi_mutex_release(canvas->mutex) == FuriStatusOk);
}

static void canvas_cb_call(Canvas* canvas) {
    canvas_cb_lock(canvas);
    for
        M_EACH(p, canvas->canvas_callback_pair, CanvasCallbackPairArray_t) {
            p->callback(
                canvas_get_buffer(canvas),
                canvas_get_buffer_size(canvas),
                canvas_get_orientation(canvas),
                p->context);
        }
    canvas_cb_unlock(canvas);
}

void canvas_add_framebuffer_callback(Canvas* canvas, CanvasCommitCallback callback, void* context) {
    furi_check(canvas);

    const CanvasCallbackPair p = {callback, context};

    canvas_cb_lock(canvas);
    furi_check(!CanvasCallbackPairArray_count(canvas->canvas_callback_pair, p));
    CanvasCallbackPairArray_push_back(canvas->canvas_callback_pair, p);
    canvas_cb_unlock(canvas);
}

void canvas_remove_framebuffer_callback(
    Canvas* canvas,
    CanvasCommitCallback callback,
    void* context) {
    furi_check(canvas);

    const CanvasCallbackPair p = {callback, context};

    canvas_cb_lock(canvas);
    furi_check(CanvasCallbackPairArray_count(canvas->canvas_callback_pair, p) == 1);
    CanvasCallbackPairArray_remove_val(canvas->canvas_callback_pair, p);
    canvas_cb_unlock(canvas);
}

static void canvas_start_draw_call(Canvas* canvas) {
    canvas->cl = nema_cl_create();
    nema_cl_bind(&canvas->cl);

    nema_bind_dst_tex(
        canvas_get_dest(canvas), canvas->width, canvas->height, FB_FORMAT, FB_STRIDE);
    nema_set_clip(0, 0, canvas->width, canvas->height);
}

static void canvas_end_draw_call(Canvas* canvas) {
    nema_cl_unbind();
    nema_cl_submit(&instance->cl);

    furi_check(nema_cl_wait(&instance->cl) == 0, "Nema: wait irq failed");
    nema_cl_destroy(&instance->cl);
}

Canvas* canvas_init(void) {
    // TODO: moar than one canvas?
    furi_check(instance == NULL, "Canvas already initialized");
    instance = malloc(sizeof(Canvas));

    instance->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    instance->orientation = CanvasOrientationHorizontal;
    instance->width = FB_WIDTH;
    instance->height = FB_HEIGHT;
    instance->offset_x = 0;
    instance->offset_y = 0;
    instance->font_render = u8g2_font_render(
        canvas_font_get(FontSecondary), canvas_draw_pixel_fg, canvas_draw_pixel_bg, instance);

    canvas_alloc_dest(instance, instance->width, instance->height);

    nema_vg_init(instance->width, instance->height);

    canvas_start_draw_call(instance);

    canvas_reset(instance);
    canvas_commit(instance);

    return instance;
}

void canvas_reset(Canvas* canvas) {
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontSecondary);
    // canvas_set_font_direction(canvas, CanvasDirectionLeftToRight);
}

void canvas_clear(Canvas* canvas) {
    furi_check(canvas);
    nema_clear(canvas_get_bg_color(canvas));
}

void canvas_set_color(Canvas* canvas, Color color) {
    furi_check(canvas);
    canvas->color = color;
}

void canvas_invert_color(Canvas* canvas) {
    furi_check(canvas);
    canvas->color = (canvas->color == ColorBlack) ? ColorWhite : ColorBlack;
}

void canvas_set_font(Canvas* canvas, Font font) {
    canvas->font_render = u8g2_font_render(
        canvas_font_get(font), canvas_draw_pixel_fg, canvas_draw_pixel_bg, instance);
}

void canvas_set_font_direction(Canvas* canvas, CanvasDirection dir) {
    UNUSED(canvas);
    UNUSED(dir);
    furi_crash("Not implemented");
}

void canvas_commit(Canvas* canvas) {
    furi_check(canvas);

    canvas_end_draw_call(canvas);

    canvas_cb_call(canvas);

    canvas_start_draw_call(instance);
}

uint8_t* canvas_get_buffer(Canvas* canvas) {
    furi_check(canvas);
    return canvas->dest;
}

size_t canvas_get_buffer_size(const Canvas* canvas) {
    furi_check(canvas);
    return canvas->width * canvas->height * FB_FORMAT_SIZE;
}

CanvasOrientation canvas_get_orientation(const Canvas* canvas) {
    furi_check(canvas);
    return canvas->orientation;
}

void canvas_draw_dot(Canvas* canvas, int32_t x, int32_t y) {
    furi_check(canvas);
    x += canvas->offset_x;
    y += canvas->offset_y;
    nema_fill_rect(x, y, 1, 1, canvas_get_color(canvas));
}

void canvas_draw_box(Canvas* canvas, int32_t x, int32_t y, size_t width, size_t height) {
    furi_check(canvas);
    x += canvas->offset_x;
    y += canvas->offset_y;
    nema_fill_rect(x, y, width, height, canvas_get_color(canvas));
}

void canvas_draw_rbox(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    size_t width,
    size_t height,
    size_t radius) {
    furi_check(canvas);
    x += canvas->offset_x;
    y += canvas->offset_y;
    nema_fill_rounded_rect(x, y, width, height, radius, canvas_get_color(canvas));
}

void canvas_draw_frame(Canvas* canvas, int32_t x, int32_t y, size_t width, size_t height) {
    furi_check(canvas);
    x += canvas->offset_x;
    y += canvas->offset_y;
    nema_draw_rect(x, y, width, height, canvas_get_color(canvas));
}

void canvas_draw_rframe(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    size_t width,
    size_t height,
    size_t radius) {
    furi_check(canvas);
    x += canvas->offset_x;
    y += canvas->offset_y;
    nema_draw_rounded_rect(x, y, width, height, radius, canvas_get_color(canvas));
}

void canvas_draw_line(Canvas* canvas, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    furi_check(canvas);
    x1 += canvas->offset_x;
    y1 += canvas->offset_y;
    x2 += canvas->offset_x;
    y2 += canvas->offset_y;
    nema_draw_line(x1, y1, x2, y2, canvas_get_color(canvas));
}

void canvas_draw_circle(Canvas* canvas, int32_t x, int32_t y, size_t radius) {
    furi_check(canvas);
    x += canvas->offset_x;
    y += canvas->offset_y;
    nema_draw_circle(x, y, radius, canvas_get_color(canvas));
}

void canvas_draw_disc(Canvas* canvas, int32_t x, int32_t y, size_t radius) {
    furi_check(canvas);
    x += canvas->offset_x;
    y += canvas->offset_y;
    nema_fill_circle(x, y, radius, canvas_get_color(canvas));
}

size_t canvas_width(const Canvas* canvas) {
    furi_check(canvas);
    return canvas->width;
}

size_t canvas_height(const Canvas* canvas) {
    furi_check(canvas);
    return canvas->height;
}

void canvas_draw_str(Canvas* canvas, int32_t x, int32_t y, const char* str) {
    furi_check(canvas);
    x += canvas->offset_x;
    y += canvas->offset_y;
    u8g2_font_render_print(&canvas->font_render, x, y, str);
}

size_t canvas_string_width(Canvas* canvas, const char* str) {
    furi_check(canvas);
    if(!str) return 0;
    return u8g2_font_str_width_get(&canvas->font_render, str);
}

static size_t canvas_font_get_ascent(Canvas* canvas) {
    return canvas->font_render.header.ascent_A;
}

void canvas_draw_str_aligned(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    Align horizontal,
    Align vertical,
    const char* str) {
    furi_check(canvas);
    if(!str) return;

    switch(horizontal) {
    case AlignLeft:
        break;
    case AlignRight:
        x -= canvas_string_width(canvas, str);
        break;
    case AlignCenter:
        x -= (canvas_string_width(canvas, str) / 2);
        break;
    default:
        furi_crash();
        break;
    }

    switch(vertical) {
    case AlignTop:
        y += canvas_font_get_ascent(canvas);
        break;
    case AlignBottom:
        break;
    case AlignCenter:
        y += (canvas_font_get_ascent(canvas) / 2);
        break;
    default:
        furi_crash();
        break;
    }

    canvas_draw_str(canvas, x, y, str);
}

void canvas_set_bitmap_mode(Canvas* canvas, bool alpha) {
    UNUSED(canvas);
    UNUSED(alpha);
    // u8g2_SetBitmapMode(&canvas->fb, alpha ? 1 : 0);
}

void canvas_frame_set(
    Canvas* canvas,
    int32_t offset_x,
    int32_t offset_y,
    size_t width,
    size_t height) {
    furi_check(canvas);
    canvas->offset_x = offset_x;
    canvas->offset_y = offset_y;
    canvas->width = width;
    canvas->height = height;
}

void canvas_set_orientation(Canvas* canvas, CanvasOrientation orientation) {
    furi_check(canvas);
    // const u8g2_cb_t* rotate_cb = NULL;
    // bool need_swap = false;
    if(canvas->orientation != orientation) {
        canvas->orientation = orientation;
        // TODO:
        // switch(orientation) {
        // case CanvasOrientationHorizontal:
        //     need_swap = canvas->orientation == CanvasOrientationVertical ||
        //                 canvas->orientation == CanvasOrientationVerticalFlip;
        //     rotate_cb = U8G2_R0;
        //     break;
        // case CanvasOrientationHorizontalFlip:
        //     need_swap = canvas->orientation == CanvasOrientationVertical ||
        //                 canvas->orientation == CanvasOrientationVerticalFlip;
        //     rotate_cb = U8G2_R2;
        //     break;
        // case CanvasOrientationVertical:
        //     need_swap = canvas->orientation == CanvasOrientationHorizontal ||
        //                 canvas->orientation == CanvasOrientationHorizontalFlip;
        //     rotate_cb = U8G2_R3;
        //     break;
        // case CanvasOrientationVerticalFlip:
        //     need_swap = canvas->orientation == CanvasOrientationHorizontal ||
        //                 canvas->orientation == CanvasOrientationHorizontalFlip;
        //     rotate_cb = U8G2_R1;
        //     break;
        // default:
        //     furi_crash();
        // }

        // if(need_swap) FURI_SWAP(canvas->width, canvas->height);
        // u8g2_SetDisplayRotation(&canvas->fb, rotate_cb);
        // canvas->orientation = orientation;
    }
}

void canvas_draw_icon(Canvas* canvas, int32_t x, int32_t y, const Icon* icon) {
    UNUSED(canvas);
    UNUSED(x);
    UNUSED(y);
    UNUSED(icon);
}