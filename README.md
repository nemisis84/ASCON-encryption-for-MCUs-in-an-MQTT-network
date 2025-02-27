# ASCON-encryption-for-MCUs-in-an-MQTT-network
Source code for TTM4905 Master thesis spring 2025. The project implements end-to-end encryption for MCUs in an MQTT 5.0 network. The encryption is done using the NIST LWC winner ASCON, which provides AEAD. 


## Install guide sensor-MCU

This guide is based on https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf getting started guide with VS code. 

1. Go to extensions and download the extension Raspberry Pi Pico
2. A new icon should appear in the VS code left sidebar. Select the new Raspberry Pi Pico icon.
3. To create a new project, press New C/C++ Project or import project. Select your settings to fit your setup.
4. Make sure that VS code is opened with the project as root folder. In the left side bar, press **Compile project**.
5. Plug in your Raspberry Pi Pico while pressing the Bootsel button. This will make the device ready to be flashed. Make sure the device appears on your machine. 
6. Open file explorer or use the command line and find the .uf2 file in the /build folder. Copy the .uf2 file to the Raspberry Pi. This will flash and boot the Pico.
7. The program will now run whenever the device is powered. Press BOOTSEL and plug the USB to your laptop to replace the program. 


#### Command line
```
export PICO_SDK_PATH=$(pwd)/pico-sdk
```


```
rm -rf build/*
cd build
cmake ..
make -j$(nproc)
```



### Debugging

The easiest way to debug the Pico is to send print statements back to the computer. https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf also explains how a debug probe can be used. However, This guide only concerns debugging with printf().


1. In the source code, include: stdio_init_all();. This will allow us to use the serial port of the USB cable. 
2. Add your printf() statements in the code. 
3. Flash the device and open device manager.
4. Look for Seriel USB-device (COMX).
5. Dowload a program that can print serial output such as the ARDUINO IDE.
6. Select the correct COMX output and open the serial monitor. 

## Install guide gateway

Install packages needed
```
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```