#ifndef ISR_H
#define ISR_H

typedef unsigned int isr_u32;
typedef unsigned short isr_u16;
typedef unsigned char isr_u8;

typedef struct interrupt_frame {
    isr_u32 edi;
    isr_u32 esi;
    isr_u32 ebp;
    isr_u32 esp;
    isr_u32 ebx;
    isr_u32 edx;
    isr_u32 ecx;
    isr_u32 eax;
    isr_u32 gs;
    isr_u32 fs;
    isr_u32 es;
    isr_u32 ds;
    isr_u32 vector;
    isr_u32 error_code;
    isr_u32 eip;
    isr_u32 cs;
    isr_u32 eflags;
} interrupt_frame_t;

typedef void (*interrupt_handler_t)(interrupt_frame_t *);

void isr_dispatch(interrupt_frame_t *frame);
void isr_register_handler(isr_u8 vector, interrupt_handler_t handler);
void isr_unregister_handler(isr_u8 vector);
void isr_set_exception_handler(interrupt_handler_t handler);
void isr_set_irq_handler(isr_u8 irq, interrupt_handler_t handler);

#endif
