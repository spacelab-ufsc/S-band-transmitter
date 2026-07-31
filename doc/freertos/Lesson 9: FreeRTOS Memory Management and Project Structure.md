# Lesson 9: FreeRTOS Memory Management and Project Structure

# Introduction

In the previous lessons, I learned about:

- Tasks
- Scheduler
- Task States
- Task Control Blocks
- Task Stacks
- Queues
- Core APIs
- Synchronization

This final theory lesson focuses on how FreeRTOS manages memory and how a FreeRTOS project is typically organized.

Memory management is especially important because the target platform for my internship is an **MSP430 microcontroller**, which has limited RAM.

Topics covered:

- Static vs Dynamic Memory Allocation
- Heap
- Heap Managers (heap_1 to heap_5)
- FreeRTOSConfig.h
- Project Structure
- Static Task Creation
- Static Queue Creation
- Memory Planning
- Best Practices

The internship architecture remains:

```text
Payload Task
      |
      v
Packet Queue
      |
      v
NGHam Task
      |
      v
Radio Task
```

---

# 1. Why Memory Management Matters

Every FreeRTOS object requires RAM.

Examples:

```text
Task

↓

TCB

↓

Stack

↓

Queue

↓

Semaphore

↓

Mutex
```

On a desktop computer, memory is usually abundant.

On an MSP430, RAM is limited.

Therefore, memory must be planned carefully.

---

# 2. Types of Memory

A simplified memory layout looks like:

```text
+----------------------+
| Program Flash        |
+----------------------+
| Global Variables     |
+----------------------+
| Heap                 |
+----------------------+
| Stack(s)             |
+----------------------+
```

Each section has a different purpose.

---

# 3. Static Memory

Static memory is allocated before the program starts.

Example:

```c
Packet packet;
```

Memory exists for the entire lifetime of the program.

Advantages:

- Predictable
- No allocation failure at runtime
- Faster access

Disadvantages:

- Cannot be resized
- Memory remains reserved even if unused

---

# 4. Dynamic Memory

Dynamic memory is allocated while the program is running.

Example:

```c
pvPortMalloc();
```

Memory can later be released using:

```c
vPortFree();
```

Advantages:

- Flexible
- Allocate only when needed

Disadvantages:

- Allocation can fail
- Possible fragmentation
- Less predictable

---

# 5. What is the Heap?

The heap is a region of RAM used for dynamic allocation.

Conceptually:

```text
Heap

↓

Task Allocation

↓

Queue Allocation

↓

Semaphore Allocation
```

Whenever FreeRTOS dynamically creates an object, memory usually comes from the heap.

---

# 6. Dynamic Task Creation

When using:

```c
xTaskCreate()
```

FreeRTOS dynamically allocates:

- Task Control Block
- Task Stack

Conceptually:

```text
Heap

↓

Allocate TCB

↓

Allocate Stack

↓

Task Created
```

If insufficient heap exists,

task creation fails.

---

# 7. Dynamic Queue Creation

Example:

```c
packetQueue =
xQueueCreate(
10,
sizeof(Packet)
);
```

Memory allocated includes:

- Queue control structure
- Queue storage area

Both are allocated from the heap.

---

# 8. Static Task Creation

Instead of:

```c
xTaskCreate()
```

FreeRTOS provides:

```c
xTaskCreateStatic()
```

Now the application provides:

- Stack
- TCB

Example:

```c
StaticTask_t payloadTCB;

StackType_t payloadStack[512];
```

Then:

```c
xTaskCreateStatic(...);
```

No heap allocation occurs.

---

# 9. Static Queue Creation

Instead of:

```c
xQueueCreate()
```

FreeRTOS provides:

```c
xQueueCreateStatic()
```

The application provides:

- Queue buffer
- Queue control structure

Again,

no heap allocation occurs.

---

# 10. Which Should Be Used?

Dynamic Allocation

Advantages:

- Easier
- Less code
- Flexible

Static Allocation

Advantages:

- Deterministic
- No runtime allocation failure
- Better for safety-critical systems

Many embedded projects prefer static allocation whenever practical.

---

# 11. FreeRTOS Heap Managers

FreeRTOS provides several heap implementations.

---

## heap_1

Characteristics:

- Allocation only
- No free

Advantages:

- Very simple
- Deterministic

Disadvantages:

Cannot release memory.

---

## heap_2

Supports:

- Allocation
- Free

Simple implementation.

Limited fragmentation handling.

---

## heap_3

Uses the standard C library:

```text
malloc()

free()
```

Rarely used in deeply embedded systems.

---

## heap_4

Most commonly used.

Supports:

- Allocation
- Free
- Coalescing adjacent free blocks

Good balance between performance and fragmentation.

---

## heap_5

Similar to heap_4.

Supports multiple non-contiguous memory regions.

Useful on systems with multiple RAM areas.

---

# 12. Which Heap Will My Internship Use?

I should inspect:

```text
FreeRTOSConfig.h
```

and the project files.

The heap manager is selected by including one of:

```text
heap_1.c

heap_2.c

heap_3.c

heap_4.c

heap_5.c
```

Many modern FreeRTOS projects use:

```text
heap_4
```

but I should verify the actual firmware.

---

# 13. FreeRTOSConfig.h

This is one of the most important files in every FreeRTOS project.

It contains configuration options such as:

```text
Tick Rate

Maximum Priorities

Heap Size

Stack Overflow Checking

Static Allocation Support

Dynamic Allocation Support
```

Examples:

```c
configTOTAL_HEAP_SIZE

configMAX_PRIORITIES

configUSE_PREEMPTION

configUSE_TIME_SLICING
```

Understanding this file is essential when joining an existing project.

---

# 14. Typical Project Structure

Example:

```text
Project

|

+-- main.c

+-- FreeRTOSConfig.h

+-- tasks/

|

+-- PayloadTask.c

|

+-- NGHamTask.c

|

+-- RadioTask.c

+-- queue/

+-- drivers/

+-- protocol/

+-- include/
```

Keeping tasks separated into individual source files improves maintainability.

---

# 15. Memory Planning for My Internship

Memory consumers include:

```text
Payload Task Stack

NGHam Task Stack

Radio Task Stack

Packet Queue

Frame Queue

Global Variables

Protocol Buffers
```

All must fit inside the available RAM.

---

# 16. Example Memory Estimate

Suppose:

Payload Stack

```text
1024 Bytes
```

NGHam Stack

```text
1024 Bytes
```

Radio Stack

```text
512 Bytes
```

Packet Queue

```text
10 × 80 Bytes

=

800 Bytes
```

Already:

```text
3360 Bytes
```

before adding global variables and other kernel objects.

This illustrates why stack sizing and queue sizing are important.

---

# 17. Common Memory Problems

Creating too many tasks.

Oversized stacks.

Very large queues.

Large global arrays.

Dynamic allocation failures.

Ignoring allocation return values.

---

# 18. Best Practices

Check every allocation result.

Choose stack sizes carefully.

Keep queues only as large as necessary.

Avoid unnecessary tasks.

Enable stack overflow checking.

Understand FreeRTOSConfig.h before modifying kernel behavior.

---

# 19. Applying to My Internship

The final architecture becomes:

```text
Payload Memory

↓

Payload Task

↓

Packet Queue

↓

NGHam Task

↓

Frame Queue

↓

Radio Task

↓

Transmitter
```

Memory requirements include:

Each Task

↓

TCB

↓

Stack

Queues

↓

Storage Buffers

Kernel

↓

Heap

Everything must fit within the available MSP430 RAM.

---

# 20. Common Misconceptions

## Misconception 1

Dynamic allocation is always better.

False.

Embedded systems often prefer static allocation.

---

## Misconception 2

The heap is unlimited.

False.

The heap has a fixed size.

---

## Misconception 3

Large stacks are harmless.

False.

They waste RAM.

---

## Misconception 4

Queue memory is free.

False.

Queues occupy RAM proportional to:

Queue Length × Item Size.

---

## Misconception 5

Every FreeRTOS project uses heap_4.

False.

The project determines which heap implementation is used.

---

# 21. What I Learned

Every FreeRTOS object consumes memory.

Tasks require:

- TCB
- Stack

Queues require:

- Queue storage
- Queue control structure

Dynamic allocation uses the heap.

Static allocation avoids runtime memory allocation.

The correct memory strategy depends on the application.

For an MSP430 system, careful memory planning is essential.

---

# 22. Key Takeaways

- Memory is a limited resource on embedded systems.
- Every task needs a TCB and stack.
- Queues consume RAM.
- Dynamic allocation uses the heap.
- Static allocation avoids runtime allocation.
- FreeRTOS provides five heap managers.
- FreeRTOSConfig.h controls kernel behavior.
- Always check allocation results.
- Proper stack and queue sizing prevents wasted RAM.
- Memory planning is essential for MSP430 firmware.

---

# 23. Theory Complete

After completing Lessons 1–9, I now understand:

- FreeRTOS architecture
- Scheduler
- Context switching
- Task states
- Tasks
- Task Control Blocks
- Task stacks
- Queues
- Core APIs
- Synchronization
- Memory management
- Project organization

This provides the theoretical foundation needed before implementing the packet-handling system for my internship.

---

# 24. Practical Roadmap

The next phase is implementation.

Planned order:

```text
1. Install and configure FreeRTOS development environment.

↓

2. Create a simple FreeRTOS project.

↓

3. Create two basic tasks.

↓

4. Learn task scheduling by experimentation.

↓

5. Create a Queue.

↓

6. Send and receive simple data.

↓

7. Create a Packet structure.

↓

8. Simulate Payload Memory.

↓

9. Payload Task:
Read memory and fragment data.

↓

10. Push Packets into Queue.

↓

11. NGHam Task:
Receive Packets.

↓

12. Generate NGHam-like Frames.

↓

13. Radio Task:
Simulate transmission.

↓

14. Compare implementation with TT&C firmware.

↓

15. Adapt code to MSP430.
```

---

## Progress

| Lesson | Topic | Status |
|---------|--------------------------------------|-----------|
| 1 | Introduction | ✅ |
| 2 | Scheduler | ✅ |
| 3 | Task States | ✅ |
| 4 | Tasks & TCB | ✅ |
| 5 | Stack | ✅ |
| 6 | Queues | ✅ |
| 7 | Core APIs | ✅ |
| 8 | Synchronization | ✅ |
| 9 | Memory Management & Project Structure | ✅ |

# 🎉 Theory Completed (9/9)

---

*These notes document my understanding while learning FreeRTOS for an embedded systems internship involving an MSP430-based payload data-handling and communication system.*
