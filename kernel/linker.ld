ENTRY(_start)

SECTIONS
{
    . = 1M;
    kernel_start = .;

    .text ALIGN(4K) : ALIGN(4K)
    {
        *(.multiboot)
        *(.text.prologue)
        *(.text .text.*)
    }

    .rodata ALIGN(4K) : ALIGN(4K)
    {
        *(.rodata .rodata.*)
    }

    .data ALIGN(4K) : ALIGN(4K)
    {
        *(.data .data.*)
    }

    .bss ALIGN(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss .bss.*)
    }

    end = ALIGN(4K);

    /DISCARD/ :
	{
		*(.comment)
		*(.eh_frame)
		*(.note.gnu.build-id)
	}
}
