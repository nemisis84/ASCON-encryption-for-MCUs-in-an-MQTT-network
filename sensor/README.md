# Sensor

This folder contains the source code for the sensor device in the project. The target device is a Raspberry Pi Pico W. It has been built for testing. By default it runs 12 experiments with different payloads and transmission intervals with a given encryption algorithm. Server.c is the main file, and initilises the devices. Server_common contains all the essential logic for connection with BLE, sending, reciving and crypto operation. CMAKELISTS.txt holds the instructions for building the project. In this file you should also define the encryption method on line 33.


## Install guide sensor-MCU

This project have been made with the getting started with VS code guide from https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf . Given the code, these steps are recommended to be taken to succesfully flash and run the code and a Raspberry Pi Pico W 

1. Go to extensions and download the extension Raspberry Pi Pico
2. Make sure that VS code is opened with the sensor folder as root folder. In the left side bar, press **Compile project**.
3. Plug in your Raspberry Pi Pico while pressing the Bootsel button. This will make the device ready to be flashed. Make sure the device appears on your machine. 
4. Open file explorer or use the command line and find the .uf2 file in the /build folder. Copy the .uf2 file to the Raspberry Pi. This will flash and boot the Pico.
5. The program will now run whenever the device is powered. 


#### Command line

If you'd rather use the command line first clone the pico-sdk: https://github.com/raspberrypi/pico-sdk# .
Export the path to PICO_SDK_PATH environment variable:

```
export PICO_SDK_PATH=$(pwd)/pico-sdk
```

Run this to compile the code. Must be ran in /sensor. 

```
rm -rf build/*
cd build
cmake ..
make -j$(nproc)
```

## Debugging

The easiest way to debug the Pico is to send print statements back to the computer. https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf also explains how a debug probe can be used. However, This guide only concerns debugging with printf().


1. In the source code, include: stdio_init_all();. This will allow us to use the serial port of the USB cable. 
2. Add your printf() statements in the code. 
3. Flash the device and open device manager.
4. Look for Seriel USB-device (COMX).
5. Dowload a program that can print serial output such as the ARDUINO IDE.
6. Select the correct COMX output and open the serial monitor. 

