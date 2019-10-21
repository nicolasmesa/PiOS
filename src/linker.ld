SECTIONS
{
    . = 0xffff000000000000; /* The first 4 "f"s are ignored by the MMU, but they mean EL1 mapped memory */
    .text.boot : { *(.text.boot) }
    .text :  { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    . = ALIGN(0x8);
    bss_begin = .; /* Data that should be initialized to 0 */
    .bss : { *(.bss*) } 
    bss_end = .;
    . = ALIGN(0x00001000);
	pg_dir = .;
	.data.pgd : { . += (3 * (1 << 12)); }
}