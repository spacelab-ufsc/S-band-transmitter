# Lesson 8: FreeRTOS Synchronization

# Introduction

In the previous lessons, I learned:

- Tasks
- Scheduler
- Task States
- Task Control Blocks
- Task Stacks
- Queues
- Core FreeRTOS APIs

Queues solve one important problem:

> Passing data safely between tasks.

However, not every situation involves passing data.

Sometimes a task simply needs to know:

- A resource is available.
- A hardware interrupt occurred.
- A shared peripheral is free.
- Another task has finished some work.

FreeRTOS provides several synchronization mechanisms for these situations.

The main synchronization primitives are:

- Binary Semaphore
- Counting Semaphore
- Mutex
- Task Notification
- Event Groups

---

# 1. What is Synchronization?

Synchronization means coordinating multiple tasks so they can safely work together.

Example:

```text
Payload Task
      |
      v
Packet Queue
      |
      v
NGHam Task
```

The Payload Task must create a packet before the NGHam Task can process it.

The queue automatically synchronizes this producer-consumer relationship.

Not all problems can be solved with queues.

---

# 2. Why Not Just Use Global Variables?

Suppose two tasks use the same UART.

```text
Payload Task

↓

UART

↑

NGHam Task
```

If both write simultaneously,

their data may become mixed.

Synchronization ensures that only one task accesses the shared resource at a time.

---

# 3. Binary Semaphore

A Binary Semaphore is a signaling mechanism.

It has only two states:

```text
Available

Unavailable
```

A task waits:

```c
xSemaphoreTake();
```

Another task or interrupt signals:

```c
xSemaphoreGive();
```

Conceptually:

```text
Task A

↓

Wait

↓

Semaphore

↓

Signal

↓

Task B continues
```

Binary Semaphores are commonly used to signal that an event has occurred.

---

# 4. Binary Semaphore Example

Suppose an interrupt indicates:

```text
Payload Memory Ready
```

Interrupt:

```text
Give Semaphore
```

Payload Task:

```text
Wait for Semaphore

↓

Read Payload
```

The Payload Task remains Blocked until the interrupt signals it.

---

# 5. Counting Semaphore

A Counting Semaphore stores a count instead of only two states.

Example:

Maximum count:

```text
5
```

Current value:

```text
3
```

Every Give:

```text
Count++
```

Every Take:

```text
Count--
```

Useful when multiple identical resources exist.

---

# 6. Mutex

A Mutex protects a shared resource.

Example:

```text
Payload Task

↓

Shared UART

↑

NGHam Task
```

Without protection:

```text
Payload:

ABCDEF

NGHam:

123456

Possible Output:

AB12CD3456EF
```

With a Mutex:

```text
Payload locks UART

↓

Uses UART

↓

Unlocks UART

↓

NGHam uses UART
```

Only one task owns the Mutex at a time.

---

# 7. Binary Semaphore vs Mutex

Although both use Take/Give APIs,

their purposes are different.

Binary Semaphore:

```text
Signal events
```

Mutex:

```text
Protect shared resources
```

A Mutex also supports:

> Priority Inheritance

Binary Semaphores do not.

---

# 8. Priority Inheritance

Suppose:

```text
Low Priority Task

owns UART Mutex
```

Then:

```text
High Priority Task

wants UART
```

Without Priority Inheritance:

High Priority waits.

Meanwhile,

Medium Priority runs continuously.

Low Priority never gets CPU.

High Priority waits indefinitely.

This is called:

```text
Priority Inversion
```

With Priority Inheritance:

The Low Priority Task temporarily inherits the higher priority,

finishes using the Mutex,

releases it,

then returns to its original priority.

This prevents long blocking.

---

# 9. Task Notifications

Task Notifications are a lightweight signaling mechanism.

Instead of creating a Semaphore,

FreeRTOS allows one task to directly notify another.

Send:

```c
xTaskNotifyGive();
```

Receive:

```c
ulTaskNotifyTake();
```

Advantages:

- Faster
- Less RAM
- Lower overhead

Useful when only one task needs to receive the notification.

---

# 10. Event Groups

Sometimes a task must wait for multiple events.

Example:

```text
Payload Ready

AND

Radio Ready

AND

GPS Ready
```

Instead of several semaphores,

FreeRTOS provides Event Groups.

Conceptually:

```text
Bit 0

Payload Ready

Bit 1

GPS Ready

Bit 2

Radio Ready
```

A task can wait until selected bits become set.

---

# 11. Synchronization Primitive Comparison

| Primitive | Purpose |
|-----------|----------|
| Queue | Transfer data |
| Binary Semaphore | Signal an event |
| Counting Semaphore | Count available resources |
| Mutex | Protect shared resource |
| Task Notification | Fast task-to-task signal |
| Event Group | Wait for multiple events |

---

# 12. Which Ones Will My Internship Use?

Based on the project description:

Payload Task

↓

Queue

↓

NGHam Task

↓

Queue

↓

Radio Task

The primary communication mechanism is:

✅ Queue

Possible additional synchronization:

Payload Memory Ready

↓

Binary Semaphore or Task Notification

Shared SPI/UART

↓

Mutex

Task Notifications may also be used for efficiency.

---

# 13. Why Queues Are Still the Main Component

Suppose the Payload Task creates:

```text
Packet
```

The NGHam Task needs:

The actual packet data.

A Binary Semaphore cannot carry the packet.

A Mutex cannot carry the packet.

A Task Notification cannot carry an entire Packet structure.

Only the Queue stores and transfers the complete Packet.

Therefore,

Queues remain the correct solution for packet transfer.

---

# 14. Common Mistakes

Using a Mutex for signaling.

Using a Semaphore to transfer large data.

Using global variables without synchronization.

Creating unnecessary synchronization objects.

Ignoring Priority Inheritance.

---

# 15. Applying Synchronization to My Internship

Payload Task:

Reads payload memory.

↓

Creates Packet.

↓

Queue.

NGHam Task:

Receives Packet.

↓

Creates NGHam Frame.

↓

Queue.

Radio Task:

Receives Frame.

↓

Transmit.

Possible synchronization:

Memory Ready Interrupt

↓

Task Notification

Shared SPI Bus

↓

Mutex

Packet Transfer

↓

Queue

---

# 16. What I Learned

Queues transfer data.

Semaphores signal events.

Mutexes protect shared resources.

Task Notifications provide lightweight signaling.

Event Groups synchronize multiple events.

Each synchronization primitive solves a different problem.

Choosing the correct one simplifies the application and avoids unnecessary complexity.

---

# 17. Key Takeaways

- Synchronization coordinates multiple tasks.
- Queues transfer data.
- Binary Semaphores signal events.
- Counting Semaphores count resources.
- Mutexes protect shared resources.
- Mutexes support Priority Inheritance.
- Task Notifications are lightweight alternatives to Binary Semaphores.
- Event Groups synchronize multiple conditions.
- The internship architecture primarily relies on Queues.
- Additional synchronization may be required for interrupts or shared peripherals.

---

# 18. Next Lesson

# Lesson 9: Memory Management and Project Structure

Topics:

- Dynamic vs Static Allocation
- Heap
- FreeRTOS Heap Managers
- heap_1 to heap_5
- FreeRTOSConfig.h
- Project Organization
- Static Tasks
- Static Queues
- Choosing Memory Allocation Strategy
- Best Practices for MSP430

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
| 9 | Memory Management & Project Structure | Next |

---

*These notes document my understanding while learning FreeRTOS for an embedded systems internship involving an MSP430-based payload data-handling and communication system.*
