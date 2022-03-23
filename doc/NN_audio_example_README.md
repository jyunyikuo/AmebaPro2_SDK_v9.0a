# YAMNet Audio Recognition Demo Quick Start

This demo will demonsrate how to deploy a pre-trained YAMNet model on AmebaPro2 to do audio recognition.

## Configure the NN demo

### Enable NN example

go to `<AmebaPro2_SDK>/project/realtek_amebapro2_v0_example/src/mmfv2_video_example/video_example_media_framework.c` to uncomment the following example
```
mmf2_video_example_audio_vipnn_init();
```

## Prepare YAMNet model

### Choose the model used

Go to `project/realtek_amebapro2_v0_example/GCC-RELEASE/mpamebapro2_fwfs_nn_models.json` and set model YAMNet_s - "MODEL2" be used
```
{
    "msg_level":3,

	"PROFILE":["FWFS"],
	"FWFS":{
        "files":[
			"MODEL2"
		]
	},
    "MODEL0":{
		"name" : "yolov4_tiny.nb",
		"source":"binary",
		"file":"yolov4_tiny.nb"
 
    },
    "MODEL1":{
		"name" : "yamnet_fp16.nb",
		"source":"binary",
		"file":"yamnet_fp16.nb"

    },
    "MODEL2":{
		"name" : "yamnet_s.nb",
		"source":"binary",
		"file":"yamnet_s.nb"

    }
}
```

## Build image

To build the example run the following command:
```
cd <AmebaPro2_SDK>/project/realtek_amebapro2_v0_example/GCC-RELEASE
mkdir build
cd build
cmake .. -G"Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake -DVIDEO_EXAMPLE=ON -DDEBUG=ON
cmake --build . --target flash_nn -j4
```
The image is located in `<AmebaPro2_SDK>/project/realtek_amebapro2_v0_example/GCC-RELEASE/build/flash_ntz.nn.bin`

## Download image

Make sure your AmebaPro2 is connected and powered on. Use the Realtek image tool to flash image:

- If Nand flash  
  use **tools/Pro2_PG_tool_linux_v1.2.3.zip** command line tool to download image and reboot
  ```
  uartfwburn.linux -p dev/ttyUSB? -f flash_ntz.nn.bin -b 3000000 -n pro2
  ```
  Note: It may require to copy the flash_ntz.nn.bin to Pro2_PG_tool_linux_v1.2.3 folder before running this command 
  
- If Nor flash  
  use image tool(AmebaZII_PGTool_Linux) to download the image to AmebaPro2 and reboot

## Validation

### Configure WiFi connection and check log

Reboot your device and check the logs.

If everything works fine, you should see the following logs

```
...
Deploy YAMNET_S
fci part tbl start   10
fci part tbl dup cnt 8
update page size 2048  page per block 64
type_name NN_MDL, file_name yamnet_s.nb
open: part_rec 7043d6a0, part_recs_cnt 1, type_id 81cf
file yamnet_s.nb, len 678336
network 70431540
input 0 dim 1 64 96 1, data format=1, quant_format=0, none-quant
ouput 0 dim 3 1 0 0, data format=1, none-quant
---------------------------------
input count 1, output count 1
input param 0
        data_format  1
        memory_type  0
        num_of_dims  4
        quant_format 0
        quant_data  , none-quant
        sizes        1 40 60 1 0 0
output param 0
        data_format  1
        memory_type  0
        num_of_dims  2
        quant_format 0
        quant_data  , none-quant
        sizes        3 1 0 0 0 0
---------------------------------
in 0, size 1 64
VIPNN opened
siso_audio_vipnn started
YAMNET_S tick[0] = 2
class 1, prob 1.00
YAMNET_S tick[0] = 2
class 1, prob 1.00
YAMNET_S tick[0] = 1
class 1, prob 1.00
YAMNET_S tick[0] = 1
class 1, prob 1.00
YAMNET_S tick[0] = 1
class 1, prob 1.00

...

```

### Use audio sample to validate the result

Then, user can use CO & smoke audio smaple in `project/realtek_amebapro2_v0_example/src/test_model/audio_test_sample` to verify the result.

YAMNet_s can recognize 3 audio classes:
1. CO
2. Others
3. Smoke
