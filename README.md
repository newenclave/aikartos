# AikaRTOS

<p align="center">
  <img src="aikartos_logo.svg" alt="AikaRTOS Logo" width="200"/>
</p>

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Made for STM32](https://img.shields.io/badge/Platform-STM32-blue.svg)](#)
[![Work in Progress](https://img.shields.io/badge/Status-Work%20in%20Progress-orange)](#)

AikaRTOS is a simple educational RTOS project originally targeting the STM32 NUCLEO-F411RE board, now also tested on the NUCLEO-H753ZI.

It is designed to explore basic task management, task scheduling, and memory allocation on embedded systems.

The project is written in **C++20**.

> **Note:** This is a purely educational project. 
> It was originally developed using a NUCLEO-F411RE board for experimentation purposes.
> Recently tested and confirmed to run on NUCLEO-H753ZI as well, though full multi-platform support is still a work in progress.

## Table of Contents
- [Features](#features)
- [Examples](#examples)
- [🎮 Snake Demo](#snake-demo)
- [License](#license)
- [Getting Started](#getting-started)
- [Build Requirements](#build-requirements)

---

## Features

- Basic task switching
- Multiple scheduler implementations (e.g., Round-Robin, Fixed Priority)
- Simple memory allocator implementations (e.g., bump, free list)
- Simple startup sequence
- Designed for STM32 Cortex-M4/M7 (tested on F411RE and H753ZI)
- Written in C++20

---

## Examples 

> All examples can be found in the [`aikartos/src/tests`](aikartos/src/tests) folder.


| Example | Description |
|------|-------------|
| [`round_robin.cpp`](aikartos/src/tests/round_robin.cpp) | Demonstrates basic Round-Robin task switching between three simple infinite loops. |
| [`edf.cpp`](aikartos/src/tests/edf.cpp) | Demonstrates Earliest Deadline First (EDF) scheduling with tasks having different deadlines. |
| [`fixed_priority.cpp`](aikartos/src/tests/fixed_priority.cpp) | Demonstrates Fixed Priority scheduling where tasks are executed based on static priorities. |
| [`lottery.cpp`](aikartos/src/tests/lottery.cpp) | Demonstrates Lottery Scheduling where tasks are chosen randomly based on ticket allocation. |
| [`priority_aging.cpp`](aikartos/src/tests/priority_aging.cpp) | Demonstrates Priority Scheduling with Aging to prevent starvation of low-priority tasks. |
| [`weighted_lottery.cpp`](aikartos/src/tests/weighted_lottery.cpp) | Demonstrates Weighted Lottery Scheduling where tasks have different chances of being selected based on weight. |
| [`stack_overflow.cpp`](aikartos/src/tests/stack_overflow.cpp) | Demonstrates system behavior when a stack overflow occurs in a task. Useful for testing robustness. |
| [`producer_consumer.cpp`](aikartos/src/tests/producer_consumer.cpp) | Demonstrates a simple Producer-Consumer system using a shared lock-free queue and cooperative task switching. |
| [`coop_preemptive.cpp`](aikartos/src/tests/coop_preemptive.cpp) | Demonstrates hybrid Cooperative-Preemptive scheduling where each task can have its own quantum or run cooperatively. |
| [`sch_cfs_like.cpp`](aikartos/src/tests/sch_cfs_like.cpp) | Demonstrates a CFS-like scheduler where tasks are selected based on the smallest virtual runtime to ensure balanced CPU time distribution.
| [`sch_mlfq.cpp`](aikartos/src/tests/sch_mlfq.cpp) | Demonstrates a Multilevel Feedback Queue scheduler with per-task quantum levels and automatic priority boosting. |
| [`memory_allocator_bump.cpp`](aikartos/src/tests/memory_allocator_bump.cpp) | Demonstrates a simple bump allocator used to manage memory in a linear fashion. |
| [`memory_allocator_free_list.cpp`](aikartos/src/tests/memory_allocator_free_list.cpp) | Demonstrates a basic free-list memory allocator with support for reuse and fragmentation handling. |
| [`memory_allocator_dlist.cpp`](aikartos/src/tests/memory_allocator_dlist.cpp) | Demonstrates a double-linked free-list allocator with bidirectional coalescing and minimal overhead on allocation. |
| [`memory_allocator_buddy.cpp`](aikartos/src/tests/memory_allocator_buddy.cpp) | Tests an arena-based buddy allocator (`buddy::impl::region`) that stores all metadata in external memory.|
| [`memory_allocator_buddy_fixed.cpp`](aikartos/src/tests/memory_allocator_buddy_fixed.cpp) | Tests a statically bounded buddy allocator (`buddy::impl::fixed`) with internal free list table.         |
| [`memory_allocator_tlsf.cpp`](aikartos/src/tests/memory_allocator_tlsf.cpp)         | Tests a dynamic TLSF allocator (`tlsf::impl::region`) that places bucket index tables inside the managed memory block. |
| [`memory_allocator_tlsf_fixed.cpp`](aikartos/src/tests/memory_allocator_tlsf_fixed.cpp) | Tests a statically sized TLSF allocator (`tlsf::impl::fixed`) with a fully internalized index table stored in `std::array`. |

---

## Snake Demo

[![Cooperative](https://img.shields.io/badge/Scheduler-Cooperative-blue)](#)

🎮🎮🎮

<p align="center">
  <img src="video/the_snake_demo.gif" alt="AikaRTOS Snake Demo" width="480"/>
</p>

A tiny cooperative Snake game demo running on AikaRTOS.
Implements movement via timer-driven tasks, showcasing clean task switching and input handling.

[`the_snake.cpp`](aikartos/src/tests/_the_snake.cpp)

## License

This project is licensed under the [MIT License](LICENSE).

---

## Build Requirements

To build and run this project, you will need:

- **ARM toolchain** (e.g., `arm-none-eabi-gcc`)
- **STM32Cube Firmware Package for STM32F4** 
  This includes the necessary device headers like `<stm32f4xx.h>` and HAL/LL drivers.
  You can download it via:
  - [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)
  - [STM32CubeF4 Package](https://www.st.com/en/embedded-software/stm32cubef4.html)

> Make sure the firmware package is correctly installed and accessible to your build system or IDE.

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


