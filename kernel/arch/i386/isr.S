BITS 32

section .text

extern isr_dispatch
global idt_flush
global isr_stub_table

idt_flush:
    mov eax, [esp + 4]
    lidt [eax]
    ret

isr_common:
    cld

    push ds
    push es
    push fs
    push gs

    pushad

    mov ax, 0x18
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isr_dispatch
    add esp, 4

    popad

    pop gs
    pop fs
    pop es
    pop ds

    add esp, 8

    iretd

%macro ISR_NOERR 1
global isr%1
isr%1:
    push dword 0
    push dword %1
    jmp isr_common
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    push dword %1
    jmp isr_common
%endmacro

%assign i 0
%rep 48
%if i = 8 || i = 10 || i = 11 || i = 12 || i = 13 || i = 14 || i = 17 || i = 21 || i = 29 || i = 30
    ISR_ERR i
%else
    ISR_NOERR i
%endif
%assign i i+1
%endrep

section .rodata
align 4

isr_stub_table:
%assign i 0
%rep 48
    dd isr%+i
%assign i i+1
%endrep
