#include "isr.h"
#include "idt.h"

static interrupt_handler_t handlers[256];
static interrupt_handler_t exception_handler = 0;

void isr_register_handler(isr_u8 vector, interrupt_handler_t handler) {
    handlers[vector] = handler;
}

void isr_unregister_handler(isr_u8 vector) {
    handlers[vector] = 0;
}

void isr_set_exception_handler(interrupt_handler_t handler) {
    exception_handler = handler;
}

void isr_set_irq_handler(isr_u8 irq, interrupt_handler_t handler) {
    if (irq >= 16)
        return;

    handlers[32 + irq] = handler;
}

void isr_dispatch(interrupt_frame_t *frame) {
    if (!frame)
        return;

    isr_u32 vector = frame->vector;

    if (vector >= 256)
        return;

    if (vector >= 32 && vector < 48) {
        isr_u8 irq = (isr_u8)(vector - 32);

        if (irq == 7 || irq == 15) {
            isr_u16 port = irq == 7 ? 0x20 : 0xA0;
            isr_u8 isr_value;

            __asm__ volatile (
                "outb %0, %1"
                :
                : "a"((isr_u8)0x0B), "Nd"(port)
            );

            __asm__ volatile (
                "inb %1, %0"
                : "=a"(isr_value)
                : "Nd"(port)
            );

            if (!(isr_value & 0x80)) {
                if (irq == 15) {
                    __asm__ volatile (
                        "outb %0, %1"
                        :
                        : "a"((isr_u8)0x20), "Nd"((unsigned short)0x20)
                    );
                }

                return;
            }
        }

        if (handlers[vector])
            handlers[vector](frame);

        pic_send_eoi(irq);
        return;
    }

    if (handlers[vector]) {
        handlers[vector](frame);
        return;
    }

    if (vector < 32) {
        if (exception_handler)
            exception_handler(frame);

        __asm__ volatile ("cli");

        for (;;)
            __asm__ volatile ("hlt");
    }
}
