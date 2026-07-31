# Lesson 6: FreeRTOS Queues

# Introduction

Until now I have learned:

- Tasks
- Scheduler
- Task States
- Task Control Blocks
- Task Stacks

Now comes one of the most important FreeRTOS features:

> **Queues**

A queue is the primary mechanism used by FreeRTOS tasks to communicate with each other.

In my internship, queues are one of the core components of the packet handling pipeline.

The overall architecture is:

```text
Payload Memory
      |
      v
Payload Task
      |
      | Packet
      v
FreeRTOS Queue (FIFO)
      |
      v
NGHam Task
      |
      | NGHam Frame
      v
Radio / Transmitter
```

The Payload Task creates packets.

The NGHam Task receives packets.

The Queue safely transfers packets between the two tasks.

---

# 1. Why Do We Need Queues?

Suppose the Payload Task creates a packet.

How does the NGHam Task receive it?

One possibility is using a global variable.

Example:

```c
Packet packet;
```

Payload Task:

```c
packet.sequence = 1;
```

NGHam Task:

```c
processPacket(packet);
```

This approach quickly creates problems.

Problems include:

- Data corruption
- Synchronization issues
- Race conditions
- Difficult debugging
- No ordering
- No buffering

Instead,

FreeRTOS provides queues.

---

# 2. What is a Queue?

A queue is a FIFO (First In First Out) data structure managed by FreeRTOS.

FIFO means:

```text
First Packet Entered

↓

First Packet Removed
```

Example:

```text
Insert:

Packet 1

Packet 2

Packet 3

Queue:

Front

↓

[Packet1]

[Packet2]

[Packet3]

↑

Back
```

Removing packets:

```text
Receive

↓

Packet1

Receive

↓

Packet2

Receive

↓

Packet3
```

The order never changes.

---

# 3. Why FIFO is Important

Suppose payload data is fragmented into packets.

Example:

```text
Packet 1

Packet 2

Packet 3

Packet 4
```

These packets contain sequential payload data.

If transmission order changes:

```text
Packet 3

Packet 1

Packet 4

Packet 2
```

The receiver cannot reconstruct the original payload correctly.

Therefore,

FIFO ordering is essential.

---

# 4. Queue Memory

A queue is stored entirely inside RAM.

Conceptually:

```text
Queue

+----------------------+

Packet 1

+----------------------+

Packet 2

+----------------------+

Packet 3

+----------------------+

Packet 4

+----------------------+
```

Each queue has:

- Storage area
- Maximum capacity
- Item size
- Current number of items

---

# 5. Creating a Queue

Queues are created using

```c
xQueueCreate()
```

Syntax:

```c
QueueHandle_t xQueueCreate(

    UBaseType_t queueLength,

    UBaseType_t itemSize

);
```

Example:

```c
QueueHandle_t packetQueue;

packetQueue =
xQueueCreate(
    10,
    sizeof(Packet)
);
```

Meaning:

```text
Queue Length

↓

10 packets maximum

Item Size

↓

sizeof(Packet)
```

The queue can store ten Packet structures.

---

# 6. Queue Handle

Just like tasks have handles,

queues also have handles.

Example:

```c
QueueHandle_t packetQueue;
```

The handle identifies the queue.

Every queue operation requires the queue handle.

Example:

```c
xQueueSend(
    packetQueue,
    ...
);
```

---

# 7. Queue Item Size

Suppose:

```c
typedef struct
{
    uint16_t sequence;

    uint16_t occultationID;

    uint16_t payloadSize;

    uint8_t payload[64];

} Packet;
```

Queue creation:

```c
xQueueCreate(
    10,
    sizeof(Packet)
);
```

Each queue element stores one complete Packet.

Conceptually:

```text
Queue

↓

Packet

↓

Packet

↓

Packet
```

FreeRTOS copies the entire structure into queue memory.

---

# 8. Sending Data

Data is sent using

```c
xQueueSend()
```

Example:

```c
Packet packet;

xQueueSend(

    packetQueue,

    &packet,

    portMAX_DELAY

);
```

Meaning:

Queue:

```text
packetQueue
```

Item:

```text
packet
```

Wait forever if queue is full.

---

# 9. Receiving Data

Receiving:

```c
Packet packet;

xQueueReceive(

    packetQueue,

    &packet,

    portMAX_DELAY

);
```

Meaning:

Wait until a packet becomes available.

When received,

the queue removes the oldest packet.

---

# 10. Queue is FIFO

Suppose Payload Task creates:

```text
Packet1

Packet2

Packet3
```

Queue:

```text
Front

↓

Packet1

Packet2

Packet3

↑

Back
```

NGHam Task receives:

```text
Packet1

↓

Packet2

↓

Packet3
```

Exactly the same order.

---

# 11. What Happens When Queue is Empty?

Suppose:

```text
Queue

EMPTY
```

NGHam executes:

```c
xQueueReceive(
    packetQueue,
    &packet,
    portMAX_DELAY
);
```

Result:

```text
NGHam Task

↓

BLOCKED
```

The task sleeps until data arrives.

No CPU is wasted.

---

# 12. What Happens When Queue is Full?

Suppose queue capacity is:

```text
10 packets
```

Current contents:

```text
Packet1

Packet2

...

Packet10
```

Payload Task tries:

```c
xQueueSend(
    packetQueue,
    &packet,
    portMAX_DELAY
);
```

Since queue is full,

Payload Task becomes

```text
BLOCKED
```

until space becomes available.

---

# 13. Queue Timeout

Instead of waiting forever,

we can wait a fixed time.

Example:

```c
xQueueReceive(

    packetQueue,

    &packet,

    pdMS_TO_TICKS(500)

);
```

Meaning:

Wait

```text
500 ms
```

If packet arrives,

receive it.

Otherwise,

timeout.

---

# 14. Queue Return Values

Both

```c
xQueueSend()

xQueueReceive()
```

return:

```c
pdPASS
```

or

```c
errQUEUE_FULL

errQUEUE_EMPTY
```

depending on the operation and timeout.

Always check return values.

Example:

```c
if(

xQueueReceive(

packetQueue,

&packet,

portMAX_DELAY

)==pdPASS)

{

processPacket();

}
```

---

# 15. Queue Copy Behavior

Very important.

Queue stores

a COPY

of the item.

Example:

```c
Packet packet;
```

Payload:

```text
packet

↓

Queue Copy

↓

NGHam receives copy
```

Queue does NOT store the pointer.

It copies the entire structure.

---

# 16. Queue Flow in My Internship

Payload Task

↓

Read Payload Memory

↓

Fragment Payload

↓

Packet

↓

Queue

↓

NGHam Task

↓

Receive Packet

↓

Generate NGHam Frame

↓

Transmit

Exactly matching my supervisor's explanation.

---

# 17. Choosing Queue Length

Example:

Queue Length

```text
10
```

Meaning:

Maximum

10 packets

stored simultaneously.

Too small:

Payload blocks frequently.

Too large:

Wastes RAM.

Choosing queue length depends on:

- Packet size
- Producer speed
- Consumer speed
- Available RAM

---

# 18. Queue Memory Usage

Suppose:

Packet size:

```text
80 Bytes
```

Queue length:

```text
10
```

RAM used:

```text
80 × 10

=

800 Bytes
```

Plus internal queue overhead.

On MSP430,

this matters.

---

# 19. Queue vs Global Variable

Global Variable:

❌ No ordering

❌ One value

❌ Synchronization problems

Queue:

✅ FIFO

✅ Multiple packets

✅ Safe task communication

✅ Blocking support

---

# 20. Common Mistakes

Creating queue with wrong item size.

Ignoring return values.

Using queue as shared memory.

Making queue unnecessarily large.

Polling instead of blocking.

---

# 21. Applying to My Internship

Task 1

↓

Read payload memory

↓

Fragment payload

↓

Create Packet

↓

Queue

Task 2

↓

Receive Packet

↓

Generate NGHam Frame

Task 3

↓

Transmit Frame

This is almost exactly what my supervisor described.

---

# 22. Key Takeaways

- Queue is FIFO.
- Queue allows communication between tasks.
- Queue copies data.
- Queue has maximum capacity.
- Queue can block sender.
- Queue can block receiver.
- Queue preserves order.
- Queue length directly affects RAM usage.
- Queue is central to my internship implementation.

---

# 23. Next Lesson

Lesson 7:

Core FreeRTOS APIs

Topics:

- vTaskStartScheduler()
- vTaskDelay()
- vTaskDelete()
- vTaskSuspend()
- vTaskResume()
- xTaskNotify()
- xTaskGetTickCount()
- Task priorities
- Task handles
- APIs used most frequently in real FreeRTOS projects.

---

**Author:** Hardik Singhal
**Email:** 24uec235@lnmiit.ac.in
**GitHub:** [@GoldernHaze](https://github.com/GoldernHaze)
