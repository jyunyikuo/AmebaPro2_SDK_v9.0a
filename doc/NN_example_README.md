# YOLOv4 Object Detection Demo Quick Start

This demo will demonsrate how to deploy a pre-trained YOLOv4 model on AmebaPro2 to do object detection.

## Configure the NN demo

### Select camera sensor 

Check your camera sensor model, and define it in `<AmebaPro2_SDK>/project/realtek_amebapro2_v0_example/inc/platform_opts.h`

```
//SENSOR_GC2053, SENSOR_PS5258
#define USE_SENSOR SENSOR_PS5258
```

### Enable NN example

go to `<AmebaPro2_SDK>/project/realtek_amebapro2_v0_example/src/mmfv2_video_example/video_example_media_framework.c` to uncomment the following example
```
mmf2_video_example_vipnn_rtsp_init();
```

## Prepare YOLOv4 model

### Get model NBG file

If you have a pre-trained YOLO model(.cfg & .weights), it can be converted to an NBG file(.nb) by Acuity tool.

Note: If you don't have a pre-trained YOLO model, you can use the model given in SDK to validate the demo.
- `<AmebaPro2_SDK>/project/realtek_amebapro2_v0_example/src/test_model/yolov4_tiny.nb`

### Choose the model used

Go to `project\realtek_amebapro2_v0_example\GCC-RELEASE\config.cmake` and set model .nb be used
```
set(USED_NN_MODEL ${prj_root}/src/test_model/yolov4_tiny.nb)
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

## Validation

### Configure WiFi connection and check log

Reboot your device and check the logs.

While runnung the example, you may need to configure WiFi connection by using these commands in uart terminal.

```
ATW0=<WiFi_SSID> : Set the WiFi AP to be connected
ATW1=<WiFi_Password> : Set the WiFi AP password
ATWC : Initiate the connection
```

If everything works fine, you should see the following logs

```
...
[VOE]RGB3 640x480 1/5
[VOE]Start Mem Used ISP/ENC:     0 KB/    0 KB Free=  701
hal_rtl_sys_get_clk 2
GCChipRev data = 8020
GCChipDate data = 20190925
queue 20121bd8 queue mutex 71691380
npu gck vip_drv_init, video memory heap base: 0x71B00000, size: 0x01300000
yuv in 0x714cee00
[VOE][process_rgb_yonly_irq][371]Errrgb ddr frame count overflow : int_status 0x00000008 buf_status 0x00000010 time 15573511 cnt 0
input 0 dim 640 352 3 1, data format=2, quant_format=2, scale=0.003635, zero_point=0
ouput 0 dim 20 11 18 1, data format=2, scale=0.085916, zero_point=161
ouput 1 dim 40 22 18 1, data format=2, scale=0.083689, zero_point=159
---------------------------------
input count 1, output count 2
input param 0
        data_format  2
        memory_type  0
        num_of_dims  4
        quant_format 2
        quant_data  , scale=0.003635, zero_point=0
        sizes        280 160 3 1 0 0
output param 0
        data_format  2
        memory_type  0
        num_of_dims  4
        quant_format 2
        quant_data  , scale=0.085916, zero_point=161
        sizes        14 b 12 1 0 0
output param 1
        data_format  2
        memory_type  0
        num_of_dims  4
        quant_format 2
        quant_data  , scale=0.083689, zero_point=159
        sizes        28 16 12 1 0 0
---------------------------------
in 0, size 640 352
VIPNN opened
siso_array_vipnn started
nn tick[0] = 213
object num = 0
RGB release 0x714cee00
yuv in 0x714cee00
nn tick[0] = 223
object num = 0
RGB release 0x714cee00
yuv in 0x714cee00
nn tick[0] = 222
object num = 0
RGB release 0x714cee00
yuv in 0x714cee00
nn tick[0] = 219
object num = 0
RGB release 0x714cee00
yuv in 0x714cee00
nn tick[0] = 216
object num = 0
...

```

### Use VLC to validate the result

Then, open VLC and create a network stream with URL: rtsp://192.168.x.xx:554

If everything works fine, you should see the object detection result on VLC player.

<p align="center"> <img src="https://user-images.githubusercontent.com/56305789/136002827-1317982e-cec6-4e30-9aa7-45e4ad873407.JPG" alt="test image size" height="80%" width="80%"></p>
