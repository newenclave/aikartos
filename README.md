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

The `src/tests/` directory contains example projects demonstrating the usage of different schedulers.

| Example                  | Description                                                                 |
|--------------------------|-----------------------------------------------------------------------------|
| `round_robin.cpp`        | Demonstrates basic Round-Robin task switching between three simple infinite loops. |
| `edf.cpp`                | Demonstrates Earliest Deadline First (EDF) scheduling with tasks having different deadlines. |
| `fixed_priority.cpp`     | Demonstrates Fixed Priority scheduling where tasks are executed based on static priorities. |
| `lottery.cpp`            | Demonstrates Lottery Scheduling where tasks are chosen randomly based on ticket allocation. |
| `priority_aging.cpp`     | Demonstrates Priority Scheduling with Aging to prevent starvation of low-priority tasks. |
| `weighted_lottery.cpp`   | Demonstrates Weighted Lottery Scheduling where tasks have different chances of being selected based on weight. |
| `stack_overflow.cpp`     | Demonstrates system behavior when a stack overflow occurs in a task. Useful for testing robustness. |


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


