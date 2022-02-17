# AmebaPro2 SDK

### Document
- AmebaPro2 application note: [AN0700 Realtek AmebaPro2 application note.en.pdf](https://github.com/sychouwk/AmebaPro2_SDK_v9.0a/blob/v9.3a/doc/AN0700%20Realtek%20AmebaPro2%20application%20note.en.pdf)

### Clone Project  
To check out this repository:  

```
git clone -b v9.3a https://github.com/sychouwk/AmebaPro2_SDK_v9.0a.git
```

### Demo quick start

:bulb: **Edge AI - object detection on AmebaPro2 (YOLOv4-tiny)**
<a href="https://github.com/sychouwk/AmebaPro2_SDK_v9.0a/blob/v9.3a/doc/NN_example_README.md">
  <img src="https://img.shields.io/badge/-Getting%20Started-green" valign="middle" height=25px width=120px/>
</a>

:bulb: **NN tester - test your image dataset in SD card (YOLOv4-tiny)**
<a href="https://github.com/sychouwk/AmebaPro2_SDK_v9.0a/blob/v9.3a/doc/NN_file_tester_README.md">
  <img src="https://img.shields.io/badge/-Getting%20Started-green" valign="middle" height=25px width=120px/>
</a>

### Build project

- GCC build on Linux: 
  - please run the following command to prepare linux toolchain
    ```
    cd <AmebaPro2_SDK>/tools
    cat asdk-10.3.0-linux-newlib-build-3633-x86_64.tar.bz2.* | tar jxvf -
    export PATH=$PATH:<AmebaPro2_SDK>/tools/asdk-10.3.0/linux/newlib/bin
    ```
  - Run following commands to build the image with option **-DVIDEO_EXAMPLE=ON**. This option is used to build video related example.
    ```
    cd <AmebaPro2_SDK>/project/realtek_amebapro2_v0_example/GCC-RELEASE
    mkdir build
    cd build
    cmake .. -G"Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake -DVIDEO_EXAMPLE=ON -DDEBUG=ON
    cmake --build . --target flash -j4
    ```

### Download and Run

- If Nand flash
  - use **tools/Pro2_PG_tool_linux_v1.2.2.zip** command line tool to download image and reboot
    ```
    uartfwburn.linux -p dev/ttyUSB? -f flash_ntz.bin -b 3000000 -n pro2
    ```
    Note: It may require to copy the flash_ntz.bin to Pro2_PG_tool_linux_v1.2.2 folder before running this command 

- Configure WiFi Connection  
  - While runnung the example, you may need to configure WiFi connection by using these commands in uart terminal.  
    ```
    ATW0=<WiFi_SSID> : Set the WiFi AP to be connected
    ATW1=<WiFi_Password> : Set the WiFi AP password
    ATWC : Initiate the connection
    ```
