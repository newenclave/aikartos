# AikaRTOS

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Made for STM32](https://img.shields.io/badge/Platform-STM32-blue.svg)](#)
[![Work in Progress](https://img.shields.io/badge/Status-Work%20in%20Progress-orange)](#)

AikaRTOS is a simple educational RTOS project targeting the STM32 NUCLEO-F411RE board.  
It is designed to explore basic task management and scheduling concepts on embedded systems.  
The project is written in **C++20**.

> **Note:** This is a purely educational project.  
> It was developed using a NUCLEO-F411RE board for experimentation purposes.  
> Compatibility with other devices has not been tested.

---

## Features

- Basic task switching
- Multiple scheduler implementations (e.g., Round-Robin, Fixed Priority)
- Simple startup sequence
- Designed for STM32 Cortex-M4 MCUs
- Written in C++20

---

## Tests

The `Src/tests/` directory contains example projects demonstrating the usage of different schedulers.

| Example               | Description                                    |
|------------------------|------------------------------------------------|
| `round_robin.cpp`      | Demonstrates basic Round-Robin task switching between three simple infinite loops. |

> **Note:** These examples are primarily targeted for the NUCLEO-F411RE board and were built using STM32CubeIDE.

---

## License

This project is licensed under the [MIT License](LICENSE).

---

## Getting Started

1. Clone the repository:

   ```bash
   git clone https://github.com/your-username/aikartos.git
   ```

2. Open the project in your preferred IDE:

   * STM32CubeIDE (tested)
   * VSCode with Cortex-Debug plugin
   * Any other IDE supporting STM32 development

3. Flash the project to your NUCLEO-F411RE board.


