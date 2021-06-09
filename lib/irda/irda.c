#include "irda.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <furi.h>
#include "irda_i.h"


struct IrdaHandler {
    void** ctx;
};

typedef struct {
    IrdaAlloc alloc;
    IrdaDecode decode;
    IrdaReset reset;
    IrdaFree free;
} IrdaDecoders;

typedef struct {
    IrdaEncode encode;
} IrdaEncoders;

typedef struct {
    IrdaProtocol protocol;
    const char* name;
    IrdaDecoders decoder;
    IrdaEncoders encoder;
    uint8_t address_length;
    uint8_t command_length;
} IrdaProtocolImplementation;


// TODO: replace with key-value, Now we refer by enum index, which is dangerous.
static const IrdaProtocolImplementation irda_protocols[] = {
    // #0
    { .protocol = IrdaProtocolSamsung32,
      .name ="Samsung32",
      .decoder = {
          .alloc = irda_decoder_samsung32_alloc,
          .decode = irda_decoder_samsung32_decode,
          .reset = irda_decoder_samsung32_reset,
          .free = irda_decoder_samsung32_free},
      .encoder = {
          .encode = irda_encoder_samsung32_encode},
      .address_length = 2,
      .command_length = 2,
    },
    // #1
    { .protocol = IrdaProtocolNEC,
      .name = "NEC",
      .decoder = {
          .alloc = irda_decoder_nec_alloc,
          .decode = irda_decoder_nec_decode,
          .reset = irda_decoder_nec_reset,
          .free = irda_decoder_nec_free},
      .encoder = {
          .encode = irda_encoder_nec_encode},
      .address_length = 2,
      .command_length = 2,
    },
    // #2 - have to be after NEC
    { .protocol = IrdaProtocolNECext,
      .name = "NECext",
      .decoder = {
          .alloc = irda_decoder_necext_alloc,
          .decode = irda_decoder_nec_decode,
          .reset = irda_decoder_nec_reset,
          .free = irda_decoder_nec_free},
      .encoder = {
          .encode = irda_encoder_necext_encode},
      .address_length = 4,
      .command_length = 2,
    },
};


const IrdaMessage* irda_decode(IrdaHandler* handler, bool level, uint32_t duration) {
    furi_assert(handler);

    IrdaMessage* message = NULL;
    IrdaMessage* result = NULL;

    for (int i = 0; i < COUNT_OF(irda_protocols); ++i) {
        if (irda_protocols[i].decoder.decode) {
            message = irda_protocols[i].decoder.decode(handler->ctx[i], level, duration);
            if (!result && message) {
                message->protocol = irda_protocols[i].protocol;
                result = message;
            }
        }
    }

    return result;
}

IrdaHandler* irda_alloc_decoder(void) {
    IrdaHandler* handler = furi_alloc(sizeof(IrdaHandler));
    handler->ctx = furi_alloc(sizeof(void*) * COUNT_OF(irda_protocols));

    for (int i = 0; i < COUNT_OF(irda_protocols); ++i) {
        handler->ctx[i] = 0;
        if (irda_protocols[i].decoder.alloc)
            handler->ctx[i] = irda_protocols[i].decoder.alloc();
    }

    return handler;
}

void irda_free_decoder(IrdaHandler* handler) {
    furi_assert(handler);
    furi_assert(handler->ctx);

    for (int i = 0; i < COUNT_OF(irda_protocols); ++i) {
        if (irda_protocols[i].decoder.free)
            irda_protocols[i].decoder.free(handler->ctx[i]);
    }

    free(handler->ctx);
    free(handler);
}

void irda_reset_decoder(IrdaHandler* handler) {
    for (int i = 0; i < COUNT_OF(irda_protocols); ++i) {
        if (irda_protocols[i].decoder.reset)
            irda_protocols[i].decoder.reset(handler->ctx[i]);
    }
}

void irda_send(const IrdaMessage* message, int times) {
    furi_assert(message);
    furi_assert(irda_is_protocol_valid(message->protocol));

    for (int i = 0; i < times; ++i) {
        if(irda_protocols[message->protocol].encoder.encode) {
            __disable_irq();
            irda_protocols[message->protocol].encoder.encode(message->address, message->command, !!i);
            __enable_irq();
        }
    }
}

bool irda_is_protocol_valid(IrdaProtocol protocol) {
    return (protocol >= 0) && (protocol < COUNT_OF(irda_protocols));
}

IrdaProtocol irda_get_protocol_by_name(const char* protocol_name) {
    for (int i = 0; i < COUNT_OF(irda_protocols); ++i) {
        if (!strcmp(irda_protocols[i].name, protocol_name))
            return i;
    }
    return IrdaProtocolUnknown;
}

const char* irda_get_protocol_name(IrdaProtocol protocol) {
    if (irda_is_protocol_valid(protocol))
        return irda_protocols[protocol].name;
    else
        return "Invalid";
}

uint8_t irda_get_protocol_address_length(IrdaProtocol protocol) {
    if (irda_is_protocol_valid(protocol))
        return irda_protocols[protocol].address_length;
    else
        return 0;
}

uint8_t irda_get_protocol_command_length(IrdaProtocol protocol) {
    if (irda_is_protocol_valid(protocol))
        return irda_protocols[protocol].command_length;
    else
        return 0;
}

