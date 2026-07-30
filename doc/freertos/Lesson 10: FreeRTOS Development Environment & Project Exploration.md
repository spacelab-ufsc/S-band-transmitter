# Lesson 10: FreeRTOS Development Environment & Project Exploration

# Introduction

After completing the nine theory lessons, I had a solid understanding of how FreeRTOS works internally:

- Tasks
- Scheduler
- Task States
- Task Control Blocks
- Task Stacks
- Queues
- Core APIs
- Synchronization
- Memory Management

At this stage, the next objective was no longer learning theory.

Instead, the goal became preparing a complete development environment and understanding the official FreeRTOS project before writing any code.

This lesson documents that setup process.

---

# Objective

Before writing any FreeRTOS application, I wanted to ensure that:

- The development environment works correctly.
- The FreeRTOS source code builds successfully.
- I understand the repository structure.
- I know where my own code should be written.
- I know which files belong to the FreeRTOS kernel.

---

# Development Environment

Development machine:

```text
MacBook Air M4
Apple Silicon
macOS
```

Installed tools:

- Homebrew
- Git
- GCC
- CMake
- Ninja
- Visual Studio Code

These tools are required to build the POSIX FreeRTOS demo.

---

# Downloading FreeRTOS

The official FreeRTOS repository was downloaded.

Repository structure:

```text
FreeRTOS/

├── FreeRTOS/
├── FreeRTOS-Plus/
├── tools/
├── README.md
├── LICENSE.md
```

Inside:

```text
FreeRTOS/

├── Demo/
├── Source/
├── Test/
├── README.md
```

---

# Understanding the Repository

## Source

The `Source` directory contains the actual FreeRTOS kernel.

Example files:

```text
tasks.c

queue.c

list.c

timers.c

event_groups.c
```

These files implement the operating system itself.

Application code should normally use these files rather than modifying them.

---

## Demo

The `Demo` directory contains example applications for many different processors and platforms.

Examples include:

```text
STM32

AVR

MSP430

Windows

POSIX

RISC-V
```

For development on macOS, the selected platform was:

```text
Demo/

└── Posix_GCC/
```

---

# Why POSIX?

Although the internship target is an MSP430 microcontroller, development begins with the POSIX simulator because:

- No hardware is required.
- Faster compilation.
- Easier debugging.
- Same FreeRTOS API.
- Focus remains on learning FreeRTOS rather than hardware setup.

Later, the same concepts will be transferred to the MSP430 firmware.

---

# Exploring the POSIX Demo

The selected demo contains:

```text
Posix_GCC/

├── CMakeLists.txt
├── Makefile
├── FreeRTOSConfig.h
├── main.c
├── main_blinky.c
├── main_full.c
├── main_packet_demo.c
├── build/
```

Purpose of important files:

## main.c

Program entry point.

Contains the application's `main()` function.

---

## main_blinky.c

Small demonstration project.

Useful for understanding:

- Task creation
- Scheduler startup
- Basic queue usage

---

## main_full.c

Larger demonstration containing many FreeRTOS features.

Useful as a future reference.

---

## main_packet_demo.c

Packet-based example application.

Particularly interesting because my internship involves packet handling.

This file will be studied later.

---

## FreeRTOSConfig.h

Kernel configuration file.

Contains settings such as:

- Tick rate
- Heap size
- Maximum priorities
- Stack overflow detection
- Scheduler configuration

---

## CMakeLists.txt

Defines:

- Source files
- Compiler options
- Include directories
- Build configuration

---

# Building FreeRTOS

The POSIX demo was built using CMake.

Configuration:

```bash
cmake -B build
```

Compilation:

```bash
cmake --build build
```

The build completed successfully.

Final output:

```text
[100%] Built target posix_demo
```

The generated executable:

```text
build/posix_demo
```

This confirmed that:

- GCC works correctly.
- CMake is configured properly.
- The FreeRTOS kernel compiles successfully.
- The POSIX simulator builds without errors.

---

# Build Process

The complete build flow is:

```text
Source Code

↓

Compiler

↓

Object Files

↓

Linker

↓

Executable

↓

posix_demo
```

This is essentially the same build process used by embedded firmware projects.

---

# Repository Rules

During development, the following rules were established:

## Do not modify

```text
Source/
```

This directory contains the FreeRTOS kernel.

---

## Use Demo only for learning

The POSIX demo is used to understand FreeRTOS.

---

## Create separate projects

Instead of modifying the official demo extensively, new learning projects will be created separately.

This keeps the FreeRTOS repository clean.

---

# Practical Roadmap

After successfully building FreeRTOS, the planned implementation sequence is:

```text
Run POSIX Demo

↓

Create Hello FreeRTOS

↓

Create Two Tasks

↓

Experiment with Scheduler

↓

Queue Demonstration

↓

Queue of Packet Structures

↓

Payload Memory Simulation

↓

Packet Fragmentation

↓

NGHam Adaptation

↓

Study TT&C Firmware

↓

MSP430 Integration
```

---

# Connection to the Internship

The final internship architecture remains:

```text
Payload Memory

↓

Payload Task

↓

Packet Queue

↓

NGHam Task

↓

Radio Task

↓

Transmitter
```

Everything developed during the practical phase will gradually build toward this architecture.

---

# What I Learned

This lesson marks the transition from studying FreeRTOS concepts to building FreeRTOS applications.

I now understand:

- The structure of the official FreeRTOS repository.
- Which files belong to the kernel.
- Which files are example applications.
- How the POSIX simulator is organized.
- How FreeRTOS projects are built using CMake.
- How to compile the official FreeRTOS POSIX demo successfully.

The development environment is now fully prepared for practical implementation.

---

# Key Takeaways

- FreeRTOS kernel code is located in `Source/`.
- Demo projects are located in `Demo/`.
- The POSIX demo allows FreeRTOS development without hardware.
- `main.c` is the application entry point.
- `FreeRTOSConfig.h` controls kernel configuration.
- `CMakeLists.txt` defines the build process.
- The POSIX demo successfully compiled into the `posix_demo` executable.
- The development environment is fully configured.
- The next phase focuses entirely on writing FreeRTOS applications.

---

# Next Lesson

# Practical 1

Topics:

- Running the POSIX Demo
- Understanding `main()`
- Creating the first FreeRTOS task
- Using `xTaskCreate()`
- Starting the scheduler
- Using `vTaskDelay()`
- Building and executing the first custom FreeRTOS application

---

## Progress

| Phase | Status |
|---------|--------|
| Lessons 1–9 (Theory) | ✅ Completed |
| Development Environment | ✅ Completed |
| Repository Exploration | ✅ Completed |
| FreeRTOS Build | ✅ Completed |
| Running First Demo | ⏳ Next |
| Writing First Task | ⏳ Upcoming |
| Queue Programming | ⏳ Upcoming |
| Internship Project | ⏳ Upcoming |
| TT&C Firmware Study | ⏳ Upcoming |
| MSP430 Integration | ⏳ Upcoming |

---

*These notes document my practical journey of learning FreeRTOS before implementing an MSP430-based packet handling system for my summer internship.*

---

