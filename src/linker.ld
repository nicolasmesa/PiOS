SECTIONS
{
    . = 0xffff000000000000; /* The first 4 "f"s are ignored by the MMU, but they mean EL1 mapped memory */
    .text.boot : { *(.text.boot) }
    . = ALIGN(0x00001000);

    /* This space is for the user programs. All user programs start with the "user" prefix. */
    user_begin = .;
    .text.user : { build/user* (.text) }
    .rodata.user : { build/user* (.rodata) }
    .data.user : { build/user* (.data) }
    .bss.user : { build/user* (.bss) }
    user_end = .;

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