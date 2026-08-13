ENTRY(_start)

SECTIONS
{
    /* Begin at 1 MB physical — standard for Multiboot-compliant kernels */
    . = 1M;
    _kernel_phys_start = .;

    /* Boot section: must reside below the higher-half offset so that the
       bootloader can find the Multiboot header and early bootstrap code. */
    .boot :
    {
        KEEP(*(.multiboot))
        *(.boot.text)
        *(.boot.data)
        *(.boot.bss)
*(.trampoline)
    }

    /* Switch to the higher-half virtual base.  Everything that follows is
       linked at canonical kernel-space addresses. */
    . += 0xFFFFFFFF80000000;
    _kernel_virt_base = .;
    _kernel_start = .;

    /* Executable code */
    .text ALIGN(4K) : AT(ADDR(.text) - 0xFFFFFFFF80000000)
    {
        *(.text)
        *(.text.*)
    }

    /* Read-only data (string literals, constant tables, etc.) */
    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xFFFFFFFF80000000)
    {
        *(.rodata)
        *(.rodata.*)
    }

    /* Read-write data */
    .data ALIGN(4K) : AT(ADDR(.data) - 0xFFFFFFFF80000000)
    {
        *(.data)
        *(.data.*)
    }

    /* Zero-initialised data */
    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xFFFFFFFF80000000)
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /* End-of-kernel markers */
    _kernel_end = .;
    _kernel_phys_end = . - 0xFFFFFFFF80000000;

    /* Discard sections that are not needed at runtime */
    /DISCARD/ :
    {
        *(.note.gnu.build-id)
        *(.comment)
    }
}
