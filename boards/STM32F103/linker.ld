MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
    RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 64K
}

ENTRY(Reset_Handler)

/* Stack heap size */
__stack_size = DEFINED(__stack_size) ? __stack_size : 0x400;
__heap_size  = DEFINED(__heap_size)  ? __heap_size  : 0x200;

/* Stack start from the end of RAM */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* KEEP ép linker không được tối ưu section nào có tên .section */
    /* *(...) tìm từ tất cả các file objects */
    /* .abc lấy các section có tên .abc */
    /* .abc* lấy các section có tên bắt đầu bằng .abc */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))       
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)                    /* Lấy tất cả các section .text */
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* Lấy địa chỉ vùng .data trong flash */
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)       /* Gom các biến trùng tên ở các file khác nhau vào thành 1 */
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        _sheap = .;

        . = . + __heap_size;
        
        . = ALIGN(8);
        _eheap = .;
    } > RAM

    _stack_limit = _estack - __stack_size;

    PROVIDE(__StackTop = _estack);
    PROVIDE(__StackLimit = _stack_limit);
}
