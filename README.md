# Self-Adaptive Smart City Traffic System

This repository contains the source code and documentation for a self-adaptive smart city traffic system. The project simulates an intelligent urban intersection that dynamically adapts its traffic light cycles based on real-time data from various sensors.

## 🚀 Overview

The primary goal of this project is to create a traffic control system that is context-aware and can adjust its behavior to optimize traffic flow, prioritize pedestrian safety, and respond to environmental conditions. This simulation is built using an ESP32-S3 microcontroller and is designed to be a foundational step towards more complex, fully autonomous urban infrastructure.

This first generation of the system is a **"low-level" self-adaptive system**, which means it operates with a closed-loop control mechanism and adjusts its setpoints based on sensor inputs. It does not yet have the capability to learn from past experiences or make structural changes to its logic.

## ✨ Features (Generation 1)

- **Context-Aware Control**: The system uses sensors to understand its environment and make decisions.
- **Multiple Operation Modes**: It can switch between different modes to handle various scenarios like nighttime, heavy traffic, and pedestrian crossings.
- **Dynamic Setpoint Adjustment**: Traffic light timings are not fixed; they are adjusted automatically based on the current operation mode.
- **Real-time Monitoring**: A 20x4 LCD provides real-time feedback on the system's status, including the current mode, traffic light timers, vehicle counts, and air quality.

## 🛠️ System Components

### Hardware
- **Microcontroller**: ESP32-S3 DevKit
- **Traffic Lights**: 2 sets of Red, Yellow, and Green LEDs to simulate an intersection.
- **Vehicle Sensors**: 6 x CNY70 infrared sensors to detect and count vehicles in two directions.
- **Pedestrian Buttons**: 2 x push buttons for pedestrians to request a crossing signal.
- **Environmental Sensors**:
  - 2 x Light Dependent Resistors (LDRs) to detect ambient light levels.
  - 1 x CO2 sensor to monitor air quality.
- **Display**: 1 x 20x4 I2C LCD.

## ⚙️ Operating Modes

The system currently supports four distinct operating modes:

1.  **NORMAL MODE** (Default)
    - Standard, balanced traffic light cycle for both directions.
    - Green Light: 10 seconds, Yellow Light: 3 seconds.

2.  **NIGHT MODE**
    - Activated when low ambient light is detected by both LDR sensors.
    - Features faster cycles with reduced wait times to improve efficiency during low-traffic hours.
    - Green Light: 6 seconds, Yellow Light: 2 seconds.

3.  **HEAVY TRAFFIC MODE**
    - Triggered when a significant difference in vehicle count is detected between the two directions.
    - The system extends the green light duration for the more congested direction (15s) while shortening it for the other (5s).

4.  **PEDESTRIAN PRIORITY MODE**
    - Activated immediately when a pedestrian presses a crossing button.
    - All vehicle traffic is stopped (all lights turn red) for 15 seconds to allow safe pedestrian crossing.

## 💻 How to Use

The core logic is contained in the `gen1_traffic_control.ino` file. This file can be compiled and uploaded to an ESP32-S3 board that is connected to the hardware components as defined by the pin layout in the code.

The simulation can also be run using a virtual environment like Wokwi, for which a `wokwi.toml` and `diagram.json` are provided.
