# Lesson 1: Introduction to FreeRTOS

## My FreeRTOS Learning Journey

This is the first lesson in my FreeRTOS learning journey. I started learning FreeRTOS as part of my summer internship work involving embedded systems, packet handling, payload data processing, and communication between different tasks.

The final system that I am working towards can be thought of as a pipeline:

```text
Payload Storage Memory
        |
        v
Read Payload Data
        |
        v
Fragment Data into Smaller Packets
        |
        v
Store Packets in FIFO Queue
        |
        v
Convert Packets into NGHam-like Frames
        |
        v
Send Frames to the Transmitter
```

The actual target microcontroller used in the project belongs to the **MSP430 family**.

Before directly implementing this system, I decided to first understand the fundamentals of FreeRTOS properly. This lesson covers the most basic question:

> What exactly is FreeRTOS, why do embedded systems need it, and what problem does it solve?

---

## 1. What Is an Embedded System?

An embedded system is a computer designed to perform a specific task or a small set of specific tasks.

Examples include washing machines, satellites, drones, routers, automotive control units, smart watches, industrial controllers, medical devices, and communication systems.

Unlike a laptop or desktop computer, an embedded system usually has limited resources such as limited RAM, program memory, processing power, and energy.

At the center of many embedded systems is a microcontroller. The project connected with my internship uses an MCU from the MSP430 family.

A microcontroller usually contains:

```text
+------------------------------------+
|          Microcontroller           |
|                                    |
|   +--------+      +-------------+  |
|   |  CPU   |      |     RAM     |  |
|   +--------+      +-------------+  |
|                                    |
|   +--------+      +-------------+  |
|   | Flash  |      | Peripherals |  |
|   +--------+      +-------------+  |
+------------------------------------+
```

The CPU executes instructions, RAM stores temporary data, Flash stores the program, and peripherals allow communication with external hardware.

---

## 2. Programming a Microcontroller Without an RTOS

A simple embedded C program often looks like this:

```c
int main(void)
{
    initializeHardware();

    while (1)
    {
        readSensor();
        processData();
        transmitData();
        blinkLED();
    }

    return 0;
}
```

The microcontroller repeatedly executes the functions one after another:

```text
Read Sensor
     |
     v
Process Data
     |
     v
Transmit Data
     |
     v
Blink LED
     |
     v
Repeat
```

This approach is commonly called a **super loop** or **bare-metal super-loop architecture**.

For small applications, this can work perfectly well. However, problems start appearing as the system becomes more complex.

---

## 3. The Problem With One Large Loop

Imagine a satellite or communication system that must perform several jobs:

1. Read payload memory.
2. Process payload data.
3. Fragment large data into packets.
4. Monitor battery status.
5. Receive commands.
6. Encode communication frames.
7. Transmit telemetry.
8. Save logs.
9. Handle errors.
10. Service a watchdog.

We could try writing:

```c
while (1)
{
    readPayloadMemory();
    processPayload();
    createPackets();
    checkBattery();
    receiveCommands();
    encodeFrame();
    transmitTelemetry();
    saveLogs();
    checkErrors();
    serviceWatchdog();
}
```

Technically, this may work for some systems, but it creates several problems.

### Problem 1: One slow function can delay everything else

Suppose the transmitter function takes two seconds:

```text
Read Payload       -> 1 ms
Process Payload    -> 5 ms
Transmit Data      -> 2000 ms
Check Battery      -> 1 ms
```

Depending on how the code and hardware drivers are designed, other important operations may be delayed.

### Problem 2: Different jobs have different timing requirements

```text
Battery Monitoring  -> Every 5 seconds
Payload Processing  -> Whenever new data arrives
Radio Handling      -> Whenever a packet is ready
Status LED           -> Every 1 second
```

These jobs do not naturally belong inside one simple sequential loop.

### Problem 3: Code becomes difficult to maintain

As the project grows, the main loop may become increasingly complicated:

```c
while (1)
{
    if (payloadReady)
    {
        // Handle payload
    }

    if (radioReady)
    {
        // Handle radio
    }

    if (batteryTimerExpired)
    {
        // Check battery
    }

    if (commandReceived)
    {
        // Process command
    }

    if (packetAvailable)
    {
        // Encode packet
    }
}
```

Eventually, understanding the interaction between different parts of the system becomes difficult.

---

## 4. What Is an RTOS?

RTOS stands for **Real-Time Operating System**.

An RTOS is an operating system designed especially for systems where timing, predictability, responsiveness, and coordination between multiple activities are important.

Instead of writing one huge loop containing every operation, the application can be divided into smaller independent units called **tasks**.

```text
+----------------------+
|     Payload Task     |
+----------------------+

+----------------------+
|      Radio Task      |
+----------------------+

+----------------------+
|     Battery Task     |
+----------------------+

+----------------------+
|       LED Task       |
+----------------------+
```

Each task has one clear responsibility. The RTOS decides which task should execute at a particular moment.

---

## 5. What Is FreeRTOS?

FreeRTOS is a real-time operating system kernel designed for microcontrollers and small embedded processors.

It provides mechanisms such as:

- Tasks
- Scheduling
- Queues
- Semaphores
- Mutexes
- Software timers
- Event groups
- Task notifications

The most important idea is that FreeRTOS allows us to divide a complex application into separate tasks.

Instead of one giant loop, we can conceptually create:

```text
Payload Task
    |
    v
Packet Queue
    |
    v
Encoding Task
    |
    v
Transmitter Task
```

Each task focuses on one responsibility.

---

## 6. What Is a Task?

A task is essentially a function managed by the FreeRTOS scheduler.

A simplified task may look like:

```c
void PayloadTask(void *pvParameters)
{
    while (1)
    {
        readPayloadMemory();
    }
}
```

Another task could be:

```c
void TransmitterTask(void *pvParameters)
{
    while (1)
    {
        transmitNextPacket();
    }
}
```

Instead of directly calling these functions ourselves, we create tasks and allow the FreeRTOS scheduler to manage their execution.

```text
                    FreeRTOS Scheduler
                           |
             +-------------+-------------+
             |             |             |
             v             v             v
       Payload Task    NGHam Task   Transmitter Task
```

---

## 7. Does FreeRTOS Mean All Tasks Run at the Same Time?

Not necessarily.

On a single-core microcontroller, at one exact instant, the CPU can normally execute only one stream of instructions.

If we have four tasks, the CPU may rapidly switch between them:

```text
Time ------------------------------------------------->

Task A  | RUN |     |     | RUN |
Task B  |     | RUN |     |     |
Task C  |     |     | RUN |     |
Task D  |     |     |     | RUN |
```

The switching may happen quickly enough that the tasks appear to progress concurrently. This is called **multitasking**.

The component responsible for deciding which task runs is called the **scheduler**.

---

## 8. A Simple Real-Life Analogy

Imagine one chef working in a restaurant. The chef has four jobs:

```text
Prepare Burger
Prepare Pizza
Prepare Coffee
Prepare Fries
```

A smarter approach than waiting idly during each operation is to organize work according to what is ready and what is waiting:

```text
Start burger
     |
     v
Burger is cooking -> work on pizza
     |
     v
Pizza is waiting for oven -> prepare coffee
     |
     v
Burger is ready -> finish burger
```

The chef is still one person. Similarly, on a single-core MCU, FreeRTOS manages different tasks and decides which ready task should run.

---

## 9. Why Is It Called Real-Time?

The term **real-time** does not simply mean very fast.

Real-time means that the correctness of a system may depend not only on producing the correct result, but also on producing it within an acceptable time limit.

For example:

```text
Airbag activates after 5 seconds
```

The result is technically an airbag activation, but it is useless because it happened too late.

There are two broad categories:

### Hard Real-Time

Missing a deadline may cause system failure or unacceptable consequences.

Examples can include certain flight-control operations, automotive safety systems, and industrial protection systems.

### Soft Real-Time

Missing a deadline occasionally may reduce quality or performance but is not necessarily catastrophic.

Examples may include audio streaming, video processing, and some user-interface operations.

FreeRTOS provides mechanisms needed to build real-time applications, but simply using FreeRTOS does not automatically guarantee that an application will meet every deadline. The application must still be designed correctly.

---

## 10. FreeRTOS Does Not Make the CPU Faster

This is an important point.

FreeRTOS does not magically increase CPU frequency. The MCU remains the same hardware.

What FreeRTOS provides is **organization and coordination**.

```text
Without RTOS:

One Large Loop
      |
      v
Everything manually coordinated
      |
      v
Can become difficult as complexity grows
```

With an RTOS:

```text
Task 1    Task 2    Task 3    Task 4
   \         |         |         /
    \        |         |        /
             v
          Scheduler
```

FreeRTOS helps decide which task should execute and provides mechanisms for tasks to communicate safely.

---

## 11. Main Components of FreeRTOS

### 11.1 Tasks

A task is a unit of work.

```text
Payload Task
Battery Task
Radio Task
Logging Task
```

### 11.2 Scheduler

The scheduler decides which ready task gets CPU time.

### 11.3 Queues

Queues allow data to be passed safely between tasks.

```text
Producer Task
      |
      v
+----------------+
|     Queue      |
| Packet 1       |
| Packet 2       |
| Packet 3       |
+----------------+
      |
      v
Consumer Task
```

This is extremely important for my internship project because payload data must be fragmented into packets and transmitted in the correct order.

### 11.4 Semaphores

Semaphores can be used for signaling and synchronization.

### 11.5 Mutexes

A mutex is commonly used to protect a shared resource, ensuring that only one task uses it at a time.

---

## 12. Connecting FreeRTOS to My Internship Project

The main system I am working towards involves payload data handling.

A simplified version of the problem is:

1. Data exists in payload storage memory.
2. The data may be too large to send in one transmission packet.
3. The data must be split into smaller chunks.
4. Each chunk needs metadata such as sequence number, occultation ID, and payload data size.
5. Packets must preserve transmission order.
6. Packets are adapted into an NGHam-like format.
7. The resulting frames are passed toward the transmitter.

The conceptual architecture is:

```text
+---------------------------+
|   Payload Storage Memory  |
+-------------+-------------+
              |
              v
+---------------------------+
|      Payload Reader       |
|  - Read memory            |
|  - Obtain payload data    |
+-------------+-------------+
              |
              v
+---------------------------+
|   Packet Fragmentation    |
|  - Split large data       |
|  - Add sequence number    |
|  - Add occultation ID     |
|  - Add payload size       |
+-------------+-------------+
              |
              v
+---------------------------+
|       FIFO Queue          |
| Packet 1                  |
| Packet 2                  |
| Packet 3                  |
+-------------+-------------+
              |
              v
+---------------------------+
|     NGHam Adaptation      |
+-------------+-------------+
              |
              v
+---------------------------+
|        Transmitter        |
+---------------------------+
```

This is why FreeRTOS is relevant. Different parts of the processing pipeline can be organized as separate tasks that communicate using queues or other synchronization mechanisms.

---

## 13. Example Without FreeRTOS

```c
int main(void)
{
    initializeHardware();

    while (1)
    {
        readPayloadMemory();
        fragmentPayload();
        createPacket();
        encodeNGHamFrame();
        transmitFrame();
    }

    return 0;
}
```

This performs everything sequentially.

---

## 14. Example With FreeRTOS

Conceptually, we could divide the work:

```c
void PayloadTask(void *pvParameters)
{
    while (1)
    {
        readPayloadMemory();
        fragmentPayload();
        createPackets();
    }
}
```

Another task:

```c
void NGHamTask(void *pvParameters)
{
    while (1)
    {
        receiveNextPacket();
        encodeNGHamFrame();
    }
}
```

And another:

```c
void TransmitterTask(void *pvParameters)
{
    while (1)
    {
        receiveNextFrame();
        transmitFrame();
    }
}
```

Communication could conceptually become:

```text
Payload Task
      |
      | Packet
      v
Packet Queue
      |
      v
NGHam Task
      |
      | Encoded Frame
      v
Frame Queue
      |
      v
Transmitter Task
```

This is only a conceptual example. The exact final architecture should follow the actual project requirements and existing TT&C firmware design.

---

## 15. Why Not Use FreeRTOS Everywhere?

FreeRTOS is useful, but it is not automatically the best choice for every embedded application.

For a tiny application such as:

```c
while (1)
{
    readButton();
    controlLED();
}
```

introducing tasks, queues, synchronization, stacks, and scheduler overhead may be unnecessary.

An important engineering principle is:

> Use the simplest architecture that correctly solves the problem.

---

## 16. Important Misconceptions

### Misconception 1: FreeRTOS runs all tasks simultaneously

Not necessarily. On a single-core MCU, normally only one task executes at one exact instant.

### Misconception 2: FreeRTOS makes the MCU faster

No. It organizes CPU time and provides multitasking and communication mechanisms.

### Misconception 3: Every embedded system needs FreeRTOS

No. Simple applications may be better implemented using bare-metal code.

### Misconception 4: Real-time means extremely fast

No. Real-time primarily means responding within required timing constraints.

### Misconception 5: Creating more tasks always improves performance

No. Every task consumes resources, including stack memory and scheduler overhead.

---

## 17. What I Learned From This Lesson

After completing this lesson, my understanding is:

> A normal embedded application may execute everything inside one main loop. As the application becomes more complex, coordinating many independent operations manually becomes difficult. FreeRTOS allows the application to be divided into tasks. A scheduler decides which ready task gets CPU time, while mechanisms such as queues allow tasks to communicate safely.

For my internship project:

```text
Payload Memory
      |
      v
Read and Fragment Data
      |
      v
Create Packets
      |
      v
FIFO Queue
      |
      v
NGHam Processing
      |
      v
Transmission
```

FreeRTOS gives us tools to organize these operations as independent but connected parts of the system.

---

## 18. Key Takeaways

- FreeRTOS is a real-time operating system kernel designed for embedded systems.
- It allows complex applications to be divided into independent tasks.
- The scheduler decides which ready task gets CPU time.
- On a single-core MCU, only one task normally executes at a particular instant.
- FreeRTOS does not make the CPU faster; it helps organize and coordinate work.
- Queues allow tasks to safely pass data.
- Mutexes help protect shared resources.
- Semaphores and task notifications can be used for synchronization and signaling.
- Real-time does not simply mean fast; it means satisfying timing requirements.
- FreeRTOS should be used when it provides real architectural value.

---

## 19. Next Lesson

In the next lesson, I study the **FreeRTOS Scheduler** in depth.

The main questions are:

- How can one CPU manage multiple tasks?
- What is context switching?
- How does the scheduler choose which task runs?
- What are task priorities?
- What is preemption?
- What is time slicing?
- What happens when a higher-priority task becomes ready?

The connected example will continue with:

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
Transmitter Task
```

---

## Progress

| Lesson | Topic | Status |
|---|---|---|
| 1 | Introduction to FreeRTOS | Completed |
| 2 | Scheduler and Context Switching | Next |
| 3 | Task States | Upcoming |
| 4 | Tasks and Task Control Block | Upcoming |
| 5 | Stack | Upcoming |
| 6 | Queues | Upcoming |
| 7 | Core FreeRTOS APIs | Upcoming |
| 8 | Synchronization | Upcoming |
| 9 | Memory Management and Project Structure | Upcoming |

---

*These notes document my understanding while learning FreeRTOS for an embedded systems internship involving an MSP430-based payload data-handling and communication system.*

---

**Author:** Hardik Singhal
**GitHub:** [@GoldernHaze](https://github.com/GoldernHaze)
