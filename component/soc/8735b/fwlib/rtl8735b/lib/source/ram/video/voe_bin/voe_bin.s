 .section .rodata
    .global _binary_voe_bin_start
    .align  4
_binary_voe_bin_start:
    .incbin "voe.bin"
_binary_voe_bin_end:
    .global _binary_voe_bin_size
    .align  4
_binary_voe_bin_size:
    .int    _binary_voe_bin_end - _binary_voe_bin_start
	.end