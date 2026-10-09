ENTRY(_start)

SECTIONS
{
    . = 1M;

    .text ALIGN(4K) : ALIGN(4K)
    {
        *(.multiboot)
        *(.text.prologue)
        *(.text)
    }

    .rodata ALIGN(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    .data ALIGN(4K) : ALIGN(4K)
    {
        *(.data)
    }

    .bss ALIGN(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    end = ALIGN(4K);

    /DISCARD/ :
	{
		*(.comment)
		*(.eh_frame)
		*(.note.gnu.build-id)
	}
}