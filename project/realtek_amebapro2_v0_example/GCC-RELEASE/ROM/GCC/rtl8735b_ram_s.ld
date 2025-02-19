/* Linker script to configure memory regions. */

/* !! the include symbole may failed if the symbol file name is too long!! */
INCLUDE "romsym_s.so"

MEMORY
{
  /* !!! These memory map configuration MUST match with the SAU setting !!! */
  ITCM_RAM (wrx) 		: ORIGIN = 0x00028000, LENGTH = 0x00030000 - 0x00028000		/*  32KB */
  
  /* The vector table, it must start with 256 bytes aligned address */
  VECTORS_RAM (rwx)     : ORIGIN = 0x20002000, LENGTH = 0x20002200 - 0x20002000		/*  512B     */
  DTCM_RAM (wrx) 		: ORIGIN = 0x20002200, LENGTH = 0x20004000 - 0x20002200 	/*  7.5KB    */

  /* Non-Secure Callable: for Non-Secure callable functions and secure gateway */  
  NSC (rwx)             : ORIGIN = 0x2015E000, LENGTH = 0x20166300 - 0x2015E000	    /*  32.75KB     */

  /* Internal SRAM for text, rodata & data */
  RAM (rwx)             : ORIGIN = 0x20172400, LENGTH = 0x2017A800 - 0x20172400		/*  33KB  */
  
  /* RAM image Signature */
  RAM_IMG_SIGN (rwx)    : ORIGIN = 0x201723F0, LENGTH = 0x20172400 - 0x201723F0		/*  16B      */
  
  /* RAM functions entry table */
  RAM_FUN_TABLE (rwx)   : ORIGIN = 0x20172300, LENGTH = 0x201723F0 - 0x20172300     /* 240B */ 

  DDR (rwx)	            : ORIGIN = 0x70000040, LENGTH = 0x70200000 - 0x70000040		/*2047kB+960B */

}

/* Library configurations */
GROUP(libgcc.a libc.a libm.a libnosys.a)

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 *   __Vectors_End
 *   __Vectors_Size
 */
ENTRY(ram_start)

__sram_end__		= ORIGIN(RAM) + LENGTH(RAM);
__eram_end__		= ORIGIN(DDR) + LENGTH(DDR);

SECTIONS
{
	.itcm_ram.code_rodata :
	{
		. = ALIGN(4);
		__itcm_ram_code_rodata_start__ = .;
		__itcm_ram_code_rodata_end__ = .;
	} > ITCM_RAM

	.ram.vector :
	{
		. = ALIGN(0x4);
		__ram_vector_start__ = .;
		KEEP(*(.ram_vector_table))
		__ram_vector_end__ = .;
	} > VECTORS_RAM

	.ram.func.table :
	{
		__ram_start_table_start__ = .;
		KEEP(*(SORT(.start.ram.data*)))
		__ram_start_table_end__ = .;

	} > RAM_FUN_TABLE

	.ram.img.signature :
	{
		__ram_img_signature__ = .;
		KEEP(*(.start.ram.sign*))		
	} > RAM_IMG_SIGN

	.dtcm_ram.bss :
	{
		/* Note: Boot loader data buffer & stack is allocated in DTCM RAM !! */
		/*       so do not allocate data/rodata/text in this section, it may cause boot loader failed */
		__dtcm_ram_bss_start__ = .;

		*(.dtcm_ram.bss*)
		
		__dtcm_ram_bss_end__ = .;
	} > DTCM_RAM

	/* Non-secure callable function */
	/* .sgstubs : for Old ASDK */
	.sgstubs :
	{
		KEEP(*(.ns_entry_func.text*))
	} > NSC

	.gnu.sgstubs :
	{
		__nsc_start__ = .;
		KEEP(*(.ns_entry_func.text*))
		__nsc_end__ = .;
	} > NSC

	.ram.code_text : AT (__etext2)
	{
		. = ALIGN(4);
		__etext2 = .;

		. = ALIGN(32);
		__ram_entry_text_start__ = .;
		*(.ram_entry_func.text*)
		__ram_entry_text_end__ = .;
		
		__ram_code_text_start__ = .;
		*ram_start.o(.text*)
		*app_start.o(.text*)
		*mpu_config.o(.text*)
		*hal_sys_ctrl.o(.text*)
		*hal_dram*.o(.text*)
		*hal_timer.o(.text*)
		*hal_uart.o(.text*)
		*hal_pinmux.o(.text*)
		*hal_gpio.o(.text*)
		*hal_wdt.o(.text*)
		*libc_wrap.o(.text*)
		*lib_a*(.text*)
        *rtos_api_s.o(.text*)
        *stack_s.o(.text*)
        *task_s.o(.text*)
        *cmse_nonsecure_call.o*(.text*)
		
		*lib_a*(.rodata*)
		*hal_sys_ctrl.o(.rodata*)
		*hal_dram*.o(.rodata*)
		*hal_flash.o(.rodata*)
		. = ALIGN(4);
		__ram_code_text_end__ = .;
	} > RAM
		
	.ram.data : AT (__etext)
	{
		__fw_img_start__ = .;
  	    __etext = .; 
		__data_start__ = .;
		*(vtable)
		*(.data*)
		*hal_dram*.o(.data*)

		. = ALIGN(4);
		/* preinit data */
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);

		. = ALIGN(4);
		/* init data */
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);

		. = ALIGN(4);
		/* finit data */
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);

		KEEP(*(.jcr*))
		. = ALIGN(4);
		/* All data end */
		__data_end__ = .;

	} > ITCM_RAM
	
	.ram.bss :
	{
		. = ALIGN(16);
		*(.ram.bss.noinit*)

		__bss_start__ = .;
		*lib_a*(.bss*)
		*(.bss*)
		*(COMMON)
		*hal_sys_ctrl.o(.bss*)
		*hal_dram*.o(.bss*)
		*hal_flash.o(.bss*)
		__bss_end__ = .;
		__sram_heap_start__ = .;
		
	} > RAM
	
	.heap (COPY):
	{
		__HeapBase = .;
		__end__ = .;
		end = __end__;
		KEEP(*(.heap*))
		__HeapLimit = .;
	} > DTCM_RAM

	/* .stack_dummy section doesn't contains any symbols. It is only
	 * used for linker to calculate size of stack sections, and assign
	 * values to stack symbols later */
	.stack_dummy (COPY):
	{
		KEEP(*(.stack*))
	} > DTCM_RAM
		
	.ddr.text :
	{
		. = ALIGN(4);
		__eram_text_start__ = .;	
		*(.text*)							
		__eram_text_end__ = .;	
	} > DDR

	.ddr.data :
	{
		. = ALIGN(4);
		__eram_data_start__ = .;	
		*(.data*)							
		__eram_data_end__ = .;	
	} > DDR

	.ddr.rodata : 
	{
		. = ALIGN(4);
		__eram_rodata_start__ = .;
		*(.rodata*)
		*(.rodata.str1*)
		__eram_rodata_end__ = .;
	} > DDR
	
	.ddr.bss :
	{
		. = ALIGN(4);
		__eram_bss_start__ = .;	
		__eram_bss_end__ = .;
		__eram_heap_start__ = .;
		
	} > DDR
	
	/* Set stack top to end of RAM, and stack limit move down by
	 * size of stack_dummy section */
	__StackTop = ORIGIN(DTCM_RAM) + LENGTH(DTCM_RAM);
	__StackLimit = __StackTop - SIZEOF(.stack_dummy);
	PROVIDE(__stack = __StackTop);

	/* Check if data + heap + stack exceeds RAM limit */
/* TODO:	ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack") */
}
