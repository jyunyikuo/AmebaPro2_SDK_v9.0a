 .section .rodata
    .global _binary_iq_gc4653_bin_start
    .align  4
_binary_iq_gc4653_bin_start:
    .incbin "iq_gc4653.bin"
_binary_iq_gc4653_bin_end:
    .global _binary_iq_gc4653_bin_size
    .align  4
_binary_iq_gc4653_bin_size:
    .int    _binary_iq_gc4653_bin_end - _binary_iq_gc4653_bin_start
	.end