# Multi-sensor-driver-host-computer-embedded-software-and-hardware
A complete embedded system solution based on **STM32F407** for multi-sensor data acquisition, processing, and visualization.

## 📋 Project Overview

This project integrates **hardware design**, **embedded firmware**, and **PC host software** to realize high-performance multi-sensor data collection and real-time monitoring.

### Key Features
- **Multi-sensor support**: ADC, I2C, SPI, UART and other interfaces
- **Real-time data acquisition and transmission** via USB CDC
- **Qt-based Host Computer** with waveform display, data logging and analysis
- **Complete hardware design**: Schematic, PCB, and mechanical files

## 🛠️ Project Structure
Multi-sensor-driver-host-computer-embedded-software-and-hardware/
├── Multi-sensor-driver/          # STM32 Embedded Firmware
│   ├── Core/                     # Application code
│   ├── Drivers/                  # STM32 HAL & CMSIS
│   ├── Middlewares/              # USB Device Library
│   └── USB_DEVICE/               # USB CDC implementation
│
├── host-computer/                # Qt Upper Computer (PC Software)
│   ├── main.cpp
│   ├── mainwindow.cpp
│   ├── sensorprotocol.cpp        # Communication protocol
│   └── sensor_upper.pro          # Qt Project file
│
├── hardware/                     # Hardware Design Files
│   ├── Schematics & PCB/
│   ├── CAM/                      # Gerber files
│   ├── Datasheets/
│   └── Documents/
│
└── README.md
## 🎯 Main Functions

### Embedded System (STM32F407)
- Multi-sensor driver development (ADC, DAC, I2C, SPI, Timer, etc.)
- Sensor data processing and protocol parsing
- USB CDC virtual serial port communication
- Real-time multi-task management

### Host Computer (Qt)
- Real-time data receiving and parsing
- Waveform display (ScrollableChartView)
- Data logging and export
- User-friendly graphical interface

## 🛠️ Getting Started

### Embedded Development
1. Open `Multi-sensor-driver/Multi-sensor-driver.ioc` with STM32CubeMX
2. Compile with Keil MDK / STM32CubeIDE
3. Download firmware to STM32F407ZGT6 board

### Host Computer
1. Open `host-computer/sensor_upper.pro` with Qt Creator
2. Configure Qt version (Qt5/Qt6 recommended)
3. Build and run the application

## 📁 Hardware

- Main Control: STM32F407ZGT6
- Multi-sensor interface board
- Gerber files, BOM, and assembly files available in `hardware/`

## 📄 Documentation

- Hardware schematic and PCB layout
- Sensor communication protocol
- User manual (to be added)

## 🔧 Technologies Used

- **MCU**: STM32F407ZGT6
- **IDE**: STM32CubeIDE / Keil MDK
- **Framework**: STM32 HAL Library + FreeRTOS (optional)
- **PC Software**: Qt6 + QCustomPlot / Qt Charts
- **Communication**: USB CDC + Custom Protocol

## 📬 Contact

- Author: Astrid-bot-bit
- GitHub: [Astrid-bot-bit](https://github.com/Astrid-bot-bit)
