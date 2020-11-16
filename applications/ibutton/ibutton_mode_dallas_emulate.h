#pragma once
#include "ibutton.h"
#include "one_wire_slave_gpio.h"

class AppiButtonModeDallasEmulate : public AppTemplateMode<AppiButtonState, AppiButtonEvent> {
public:
    const char* name = "dallas emulate";
    AppiButton* app;
    OneWireGpioSlave* onewire_slave;

    void event(AppiButtonEvent* event, AppiButtonState* state);
    void render(CanvasApi* canvas, AppiButtonState* state);
    void acquire();
    void release();

    AppiButtonModeDallasEmulate(AppiButton* parent_app) {
        app = parent_app;

        // TODO open record
        GpioPin one_wire_pin = {iBTN_GPIO_Port, iBTN_Pin};
        GpioPin* one_wire_pin_record = &one_wire_pin;
        onewire_slave = new OneWireGpioSlave(one_wire_pin_record);
    };
};

void AppiButtonModeDallasEmulate::event(AppiButtonEvent* event, AppiButtonState* state) {
    if(event->type == AppiButtonEvent::EventTypeTick) {
        acquire();
        if(onewire_slave->emulate(state->dallas_address, 8)) {
            app->blink_green();
        } else {
            
        }
    }
}

void AppiButtonModeDallasEmulate::render(CanvasApi* canvas, AppiButtonState* state) {
    canvas->set_font(canvas, FontSecondary);
    canvas->draw_str(canvas, 2, 25, "< dallas emulate");
    canvas->draw_str(canvas, 2, 37, "give me domophone");
    {
        char buf[24];
        sprintf(
            buf,
            "%x:%x:%x:%x:%x:%x:%x:%x",
            state->dallas_address[0],
            state->dallas_address[1],
            state->dallas_address[2],
            state->dallas_address[3],
            state->dallas_address[4],
            state->dallas_address[5],
            state->dallas_address[6],
            state->dallas_address[7]);
        canvas->draw_str(canvas, 2, 50, buf);
    }
}

void AppiButtonModeDallasEmulate::acquire() {
    onewire_slave->start();
}

void AppiButtonModeDallasEmulate::release() {
    onewire_slave->stop();
}