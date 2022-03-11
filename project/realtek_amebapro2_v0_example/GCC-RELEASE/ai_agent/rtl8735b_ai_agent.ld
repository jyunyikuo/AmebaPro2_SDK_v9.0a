INCLUDE "romsym_is.so"

/* FPGA Linker script to configure memory regions.  */
MEMORY
{

  ITCM_RAM (wrx) 		: ORIGIN = 0x00010000, LENGTH = 0x00018000 - 0x00010000  /* 32KB */
  
  DTCM_RAM (wrx) 		: ORIGIN = 0x20000000, LENGTH = 0x20004000 - 0x20000000  /*  16KB */

  VECTORS_RAM (rwx)     : ORIGIN = 0x20100000, LENGTH = 0x20100200 - 0x20100000  /* 512B */
  
  /* Jump cross 0x20100200 ~ 0x20100A00 for NS ROM code used */
  RAM_FUN_TABLE (rwx)   : ORIGIN = 0x20100A00, LENGTH = 0x20100AF0 - 0x20100A00  /* 240B */
  RAM_IMG_SIGN (rwx)    : ORIGIN = 0x20100AF0, LENGTH = 0x20100B00 - 0x20100AF0  /*  16B */
  RAM (rwx)             : ORIGIN = 0x20100B00, LENGTH = 0x20177B00 - 0x20100B00  /* 476KB */
  RAM_NC (rwx)          : ORIGIN = 0x20177B00, LENGTH = 0x20179800 - 0x20177B00  /*  7KB*/
  /* Jump cross 0x2017BB00 ~ 0x20180000 for S ROM code used*/
  
/* DDR memory */
						/* Raymond  workaround for JIRA-537 issue 0x70000000 --> 0x70000040 */
						  
  VOE    (rwx)          : ORIGIN = 0x70000000, LENGTH = 0x70100000 - 0x70000000	/*  1MB */
  DDR    (rwx) 		    : ORIGIN = 0x70100000, LENGTH = 0x71F00000 - 0x70100000 /* 55MB -> 30MB */
  DDR2   (rwx) 		    : ORIGIN = 0x71F00000, LENGTH = 0x72000000 - 0x71F00000 /* 1MB */
  NN     (rwx)      : ORIGIN = 0x72000000, LENGTH = 0x74000000 - 0x72000000	/* 8MB -> 32MB */
  
  BTRACE (rx)          : ORIGIN = 0x00800000, LENGTH = 0x00C00000 - 0x00800000	/* Bluetooth Trace */
}

/* total code section on ROM space for FPGA by Raymond */


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
ENTRY(Reset_Handler)

_start_addr = 0x10000000;
ENTRY(_start_addr)

__sram_end__		= ORIGIN(RAM) + LENGTH(RAM);
__eram_end__		= ORIGIN(DDR) + LENGTH(DDR);		

__voe_code_size__   = LENGTH(VOE);

__nn_eram_start__   = ORIGIN(NN);
__nn_eram_end__     = ORIGIN(NN) + LENGTH(NN);

SECTIONS
{
    /* function table for ai agent and be used by RAM code */
    .ddr.aiagent.stubs :
	{
		. = ALIGN(4);
		__eram2_aiagent_func_stubs_start__ = .;
		KEEP(*(.sdram.aiagent.stubs*))
		__eram2_aiagent_func_stubs_end__ = .;
        __eram2_mainapp_func_stubs_start__ = .;
        KEEP(*(.sdram.mainapp.stubs*))
        __eram2_mainapp_func_stubs_end__ = .;
	} > DDR2

    .ddr.text :
	{
		. = ALIGN(4);
		__eram2_text_start__ = .;
		KEEP(*(.text*))
		KEEP(*(.sdram.text*))	
		__eram2_text_end__ = .;
	} > DDR2
    
	.ddr.bss :
	{
		. = ALIGN(4);
		__eram2_bss_start__ = .;	
        KEEP(*(.bss*))
		KEEP(*(.sdram.bss*))
		__eram2_bss_end__ = .;			
	} > DDR2
	
	.ddr.rodata : 
	{
		. = ALIGN(4);
		__eram2_rodata_start__ = .;
		KEEP(*(.rodata*))
		KEEP(*(.rodata.str1*))
		KEEP(*(.sdram.rodata*))
		__eram2_rodata_end__ = .;
	} > DDR2

	.ddr.data :
	{
		. = ALIGN(4);
		__eram2_data_start__ = .;
		KEEP(*(.data*))
		KEEP(*(.sdram.data*))
		KEEP(*(.ddr.data))
		__eram2_data_end__ = .;
		__eram2_heap_start__ = .;
	} > DDR2

}
