# ASCON-encryption-for-MCUs-in-an-MQTT-network
Source code for TTM4905 Master thesis spring 2025. The project implements end-to-end encryption for MCUs in an MQTT 5.0 network. The encryption is done using the NIST LWC winner ASCON, which provides AEAD. 

## Project Structure

### [Sensor](sensor/README.md)
The sensor component is responsible for collecting data and encrypting it using ASCON before transmitting it to the gateway via BLE. It is implemented on an Raspberry Pi Pico. 

- **Key Features**:
  - ASCON encryption for secure data transmission.
  - BLE communication with the gateway.
  - Configurable experiment settings.

Go to the [Sensor README](sensor/README.md) for installation and usage instructions.

---

### [Gateway](gateway/gateway_app/README.md)
The gateway acts as a bridge between the sensor and the MQTT broker. It receives encrypted data from the sensor via BLE, decrypts it, and forwards it to the MQTT broker.

- **Key Features**:
  - BLE GATT client for communication with sensors.
  - Wi-Fi Enterprice connection
  - MQTT 5.0 client for broker communication.

Go to the [Gateway README](gateway/gateway_app/README.md) for setup and configuration details.

---

### [Data Analysis](Data%20analysis/README.md)
This folder contains scripts and tools for analyzing the data collected from the MQTT broker. It includes performance metrics such as energy consumption, execution times, and average power consumption.

- **Key Features**:
  - Visualization of encryption performance.
  - Analysis of energy and power consumption.
  - Generation of figures for the thesis.

Go to the [Data Analysis README](Data%20analysis/README.md) for more information.

---

### [Data Storage](data-storage/README.md)
This component handles the storage and management of data collected from the MQTT broker. It includes scripts for processing and storing results in a structured format.

- **Key Features**:
  - MQTT client reciving and transmitting data back to the sensor.
  - Encryption and decryption.
  - Python environment setup (for Data analysis and data storage code).

Go to the [Data Storage README](data-storage/README.md) for setup instructions.

## Disclaimer

This project is a research prototype and is not intended for production use. The authors take no responsibility for any issues, damages, or losses that may arise from using this code. Significant performance, security, automation and scalebility improvements must be implemented before a production state is achieved. 

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
