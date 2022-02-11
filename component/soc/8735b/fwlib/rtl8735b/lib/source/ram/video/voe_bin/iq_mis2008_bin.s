 .section .rodata
    .global _binary_iq_mis2008_bin_start
    .align  4
_binary_iq_mis2008_bin_start:
    .incbin "iq_mis2008.bin"
_binary_iq_mis2008_bin_end:
    .global _binary_iq_mis2008_bin_size
    .align  4
_binary_iq_mis2008_bin_size:
    .int    _binary_iq_mis2008_bin_end - _binary_iq_mis2008_bin_start
	.end