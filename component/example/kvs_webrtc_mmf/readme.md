# Amazon KVS WebRTC demo on AmebaPro2 #

## Download the necessary source code from Github
- Go to `project/realtek_amebapro2_v0_example/src/amazon_kvs/lib_amazon`
    ```
    cd project/realtek_amebapro2_v0_example/src/amazon_kvs/lib_amazon
    ```
- Clone the following repository for KVS webrtc
	- amazon-kinesis-video-streams-webrtc-sdk-c
    ```
    git clone -b webrtc-on-freertos-wss-0126-R https://github.com/ambiot-mini/amazon-kinesis-video-streams-webrtc-sdk-c.git
    ```
    - cisco/libsrtp
    ```
    git clone -b webrtc-on-freertos https://github.com/ambiot-mini/libsrtp.git
    ```
    - tatsuhiro-t/wslay
    ```
    git clone https://github.com/ambiot-mini/wslay.git
    ```
    - nodejs/llhttp
    ```
    git clone -b release https://github.com/nodejs/llhttp.git
    cd llhttp
    git reset --hard a4aa7a70e8b9a67f378b53264c61bb044a224366
    ```
    - sctplab/usrsctp
    ```
    git clone -b webrtc-on-freertos https://github.com/ambiot-mini/usrsctp.git
    ```

## Set mbedtls version
- In KVS webrtc project, we have to use some function in mbedtls-2.16.6  
- Set mbedtls version to 2.16.6 in `project/realtek_amebapro2_v0_example/GCC-RELEASE/application/CMakeLists.txt`
    ```
    set(mbedtls "mbedtls-2.16.6")
    ```

## Modify lwipopts.h
- Modify lwipopts.h in `component/lwip/api/`
    ```
    #define LWIP_IPV6       1
    ```
    
## Enlarge SKB buffer number
- go to `component/wifi/driver/src/core/option/rtw_opt_skbuf_rtl8735b.c`  
    ```
    #define MAX_SKB_BUF_NUM      1024
    ```

## No using the wrapper function for snprintf 
- In `project/realtek_amebapro2_v0_example/GCC-RELEASE/toolchain.cmake`, comment the following wrapper function
    ```
    # "-Wl,-wrap,sprintf"
    # "-Wl,-wrap,snprintf"
    # "-Wl,-wrap,vsnprintf"
    ```

## Congiure the example
- configure AWS key channel name in `component/example/kvs_webrtc_mmf/sample_config_webrtc.h`
    ```
    /* Enter your AWS KVS key here */
    #define KVS_WEBRTC_ACCESS_KEY   "xxxxxxxxxx"
    #define KVS_WEBRTC_SECRET_KEY   "xxxxxxxxxx"

    /* Setting your signaling channel name */
    #define KVS_WEBRTC_CHANNEL_NAME "xxxxxxxxxx"
    ```
- configure video parameter in `component/example/kvs_webrtc_mmf/example_kvs_webrtc_mmf.c`
    ```
    ...
    #define V1_RESOLUTION VIDEO_HD
    #define V1_FPS 30
    #define V1_GOP 30
    #define V1_BPS 1024*1024
    ```

## Prepare cert
- Put the cert to SD card (`component/example/kvs_webrtc_mmf/certs/cert.pem`), and set its path in `sample_config_webrtc.h`
    ```
    /* Cert path */
    #define KVS_WEBRTC_ROOT_CA_PATH "0://cert.pem"
    ```

## Select camera sensor

- Check your camera sensor model, and define it in <AmebaPro2_SDK>/project/realtek_amebapro2_v0_example/inc/platform_opts.h
    ```
    //SENSOR_GC2053, SENSOR_PS5258
    #define USE_SENSOR SENSOR_PS5258
    ```
    
## Using AWS-IoT credential (optional)

- Testing Amazon KVS WebRTC with IAM user key(AK/SK) is easy but it is not recommended, user can refer the following links to set up webrtc with AWS-IoT credential
  - With AWS IoT Thing credentials, it can be managed more securely.(https://iotlabtpe.github.io/Amazon-KVS-WebRTC-WorkShop/lab/lab-4.html)
  - Script for generate iot credential: https://github.com/awslabs/amazon-kinesis-video-streams-webrtc-sdk-c/blob/master/scripts/generate-iot-credential.sh

## Build the project
- run following commands to build the image with option `-DEXAMPLE=kvs_webrtc_mmf`
    ```
    cd project/realtek_amebapro2_v0_example/GCC-RELEASE
    mkdir build
    cd build
    cmake .. -G"Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake -DEXAMPLE=kvs_webrtc_mmf
    cmake --build . --target flash
    ```

- use image tool to download the image to AmebaPro2 and reboot

- configure WiFi Connection  
    While runnung the example, you may need to configure WiFi connection by using these commands in uart terminal.  
    ```
    ATW0=<WiFi_SSID> : Set the WiFi AP to be connected
    ATW1=<WiFi_Password> : Set the WiFi AP password
    ATWC : Initiate the connection
    ```

- if everything works fine, you should see the following log
    ```
    ...
    wifi connected
    [KVS Master] Using trickleICE by default
    cert path:0://cert.pem
    look for ssl cert successfully
    [KVS Master] Created signaling channel My_KVS_Signaling_Channel
    [KVS Master] Finished setting audio and video handlers
    [KVS Master] KVS WebRTC initialization completed successfully
    N:  mem: platform fd map:   120 bytes
    N: lws_tls_client_create_vhost_context: using mem client CA cert 1424
    [KVS Master] Signaling client created successfully
    [KVS Master] Signaling client connection to socket established
    [KVS Master] Channel My_KVS_Signaling_Channel set up done
    ...
    ```

## Validate result
- we can use KVS WebRTC Test Page to test the result  
https://awslabs.github.io/amazon-kinesis-video-streams-webrtc-sdk-js/examples/index.html
