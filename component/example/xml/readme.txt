XML EXAMPLE

Description:
The creation of a light XML document is used as the example of XML document generation.
The processing of a sensor XML document is used as the example of XML document parsing.

Configuration:
[platform_opts.h]
	#define CONFIG_EXAMPLE_XML    1
For AmebaD2
	GCC: use CMD "make xip EXAMPLE=xml" to compile xml example.

Execution:
An XML example thread will be started automatically when booting.

[Supported List]
	Supported :
	    Ameba-1, Ameba-z, Ameba-pro, Ameba-z2, Ameba-D, AmebaD2