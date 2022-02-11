 .section .rodata
    .global _binary_iq_imx307hdr_bin_start
    .align  4
_binary_iq_imx307hdr_bin_start:
    .incbin "iq_imx307hdr.bin"
_binary_iq_imx307hdr_bin_end:
    .global _binary_iq_imx307hdr_bin_size
    .align  4
_binary_iq_imx307hdr_bin_size:
    .int    _binary_iq_imx307hdr_bin_end - _binary_iq_imx307hdr_bin_start
	.end