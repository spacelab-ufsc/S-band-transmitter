# Lesson 3: FreeRTOS Task States

## Introduction

In the previous lessons, I learned:

- What FreeRTOS is and why it is used.
- How an embedded application can be divided into separate tasks.
- How the FreeRTOS scheduler decides which task gets CPU time.
- What context switching is.
- How priorities and preemption affect task execution.

In this lesson, I focus on **task states**.

A FreeRTOS task does not remain continuously Running from the moment it is created. During execution, it moves between different states depending on whether it:

- Currently owns the CPU.
- Is ready to execute.
- Is waiting for data.
- Is waiting for time to pass.
- Has been explicitly suspended.
- Has been deleted.

The main task states are:

```text
Running
Ready
Blocked
Suspended
Deleted
```

For my internship project, the connected system remains:

```text
Payload Storage Memory
        |
        v
Payload Task
        |
        | Fragmented Packet
        v
FIFO Packet Queue
        |
        v
NGHam Task
        |
        | Encoded Frame
        v
Transmitter
```

Understanding task states is important because the Payload Task, NGHam Task, and any transmission-related tasks should not continuously consume CPU time when they have nothing useful to do.

---

# 1. Overview of Task States

A simplified FreeRTOS task-state model is:

```text
                    +-----------+
                    |  Running  |
                    +-----+-----+
                          |
                          |
              +-----------+-----------+
              |                       |
              v                       v
         +---------+             +---------+
         |  Ready  |             | Blocked |
         +---------+             +---------+
              ^                       |
              |                       |
              +-----------------------+

                    +-----------+
                    | Suspended |
                    +-----------+
```

A task normally moves between **Running**, **Ready**, and **Blocked** during normal execution.

The **Suspended** state is different because a suspended task does not automatically become Ready because time passed or data arrived.

The **Deleted** state means the task has been removed from normal scheduling.

---

# 2. Running State

A task is in the **Running** state when the CPU is currently executing that task's instructions.

For example:

```text
Payload Task    -> RUNNING
NGHam Task      -> BLOCKED
Radio Task      -> BLOCKED
```

The CPU is currently executing the Payload Task.

On a single-core MCU, normally only one task can be Running at one exact instant.

Conceptually:

```text
CPU
 |
 v
Payload Task
    |
    +---- Read payload memory
    |
    +---- Fragment payload
    |
    +---- Create packet
```

If another task should execute, the scheduler may perform a context switch.

The currently Running task then leaves the Running state.

---

# 3. Ready State

A task is in the **Ready** state when:

- It is capable of executing immediately.
- It is not waiting for time.
- It is not waiting for data or an event.
- But another task currently has the CPU.

For example:

```text
NGHam Task      Priority 3    RUNNING

Payload Task    Priority 2    READY

LED Task        Priority 1    READY
```

Both the Payload Task and LED Task are capable of running, but the NGHam Task currently owns the CPU.

The scheduler selects a task from the Ready state according to the scheduling rules and task priorities.

The important distinction is:

```text
Running:
The task currently has the CPU.

Ready:
The task could run immediately but does not currently have the CPU.
```

---

# 4. Blocked State

A task enters the **Blocked** state when it is waiting for something.

For example, a task may wait for:

- A delay to expire.
- Data to arrive in a queue.
- Space to become available in a queue.
- A semaphore.
- A mutex.
- A task notification.
- An event group condition.

A Blocked task does not compete for CPU time.

This is one of the most important concepts in FreeRTOS.

Suppose the NGHam Task waits for a packet:

```c
xQueueReceive(
    packetQueue,
    &packet,
    portMAX_DELAY
);
```

If the queue is empty:

```text
NGHam Task
     |
     v
xQueueReceive()
     |
     | Queue is empty
     v
BLOCKED
```

The NGHam Task does not continuously use CPU time checking the queue.

Instead, FreeRTOS places it in the Blocked state.

Another Ready task can now use the CPU.

---

# 5. Blocking Because of `vTaskDelay()`

A task can deliberately block itself for a specific period.

Example:

```c
void StatusTask(void *pvParameters)
{
    while (1)
    {
        updateStatus();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

The state transition is:

```text
RUNNING
    |
    | vTaskDelay()
    v
BLOCKED
    |
    | Delay expires
    v
READY
    |
    | Scheduler selects task
    v
RUNNING
```

An important point is:

> When the delay expires, the task normally becomes Ready, not automatically Running.

Whether it immediately starts executing depends on:

- Its priority.
- The currently Running task's priority.
- Scheduler configuration.

---

# 6. Blocking While Waiting for Queue Data

This directly applies to my internship project.

Suppose the NGHam Task contains:

```c
void NGHamTask(void *pvParameters)
{
    Packet packet;

    while (1)
    {
        xQueueReceive(
            packetQueue,
            &packet,
            portMAX_DELAY
        );

        encodeNGHamFrame(&packet);
    }
}
```

Initially, the packet queue is empty:

```text
Packet Queue:

+------------------+
|      EMPTY       |
+------------------+
```

The NGHam Task executes:

```c
xQueueReceive(
    packetQueue,
    &packet,
    portMAX_DELAY
);
```

Because no packet is available:

```text
NGHam Task -> BLOCKED
```

The task remains blocked until packet data becomes available.

Now the Payload Task creates a packet:

```text
Packet:
Sequence Number = 17
Occultation ID  = 52
Payload Size    = 64 bytes
```

Then:

```c
xQueueSend(
    packetQueue,
    &packet,
    portMAX_DELAY
);
```

The state transition becomes:

```text
Before packet arrival:

Payload Task    -> RUNNING
NGHam Task      -> BLOCKED


Payload sends packet:

        |
        v

Packet Queue receives Packet 17

        |
        v

NGHam Task:

BLOCKED -> READY
```

If the NGHam Task has a higher priority than the currently Running Payload Task, it may immediately preempt it.

Then:

```text
Payload Task    -> READY
NGHam Task      -> RUNNING
```

This connects task states directly with queues and scheduling.

---

# 7. Blocking While Sending to a Full Queue

A producer can also become Blocked.

Suppose the packet queue has a maximum capacity of three packets:

```text
Queue Capacity = 3

+------------------+
|     Packet 1     |
+------------------+
|     Packet 2     |
+------------------+
|     Packet 3     |
+------------------+

QUEUE FULL
```

Now the Payload Task tries:

```c
xQueueSend(
    packetQueue,
    &packet4,
    portMAX_DELAY
);
```

There is no space available.

Because the task specified:

```c
portMAX_DELAY
```

it can wait until queue space becomes available.

The Payload Task becomes:

```text
BLOCKED
```

Later, the NGHam Task receives Packet 1:

```text
Before:

[Packet 1]
[Packet 2]
[Packet 3]

After NGHam receives Packet 1:

[Packet 2]
[Packet 3]
[  EMPTY ]
```

Now space is available.

The Payload Task can move:

```text
BLOCKED -> READY
```

When selected by the scheduler, it can continue sending the next packet.

Therefore, queues can block both:

```text
Consumer:
Waiting because queue is empty.

Producer:
Waiting because queue is full.
```

---

# 8. Blocking With a Timeout

A task does not always need to wait forever.

For example:

```c
xQueueReceive(
    packetQueue,
    &packet,
    pdMS_TO_TICKS(1000)
);
```

This means:

> Wait for a packet for up to approximately 1000 milliseconds.

Two outcomes are possible.

### Case 1: Packet arrives before timeout

```text
BLOCKED
    |
    | Packet arrives
    v
READY
```

### Case 2: Timeout expires first

```text
BLOCKED
    |
    | Timeout expires
    v
READY
```

The function's return value tells the application whether the operation succeeded.

For example:

```c
if (xQueueReceive(
        packetQueue,
        &packet,
        pdMS_TO_TICKS(1000)
    ) == pdPASS)
{
    processPacket(&packet);
}
else
{
    handleTimeout();
}
```

This is useful when a task should not wait forever.

---

# 9. Zero Blocking Time

A queue operation can also be configured not to wait.

For example:

```c
xQueueReceive(
    packetQueue,
    &packet,
    0
);
```

If a packet exists, it is received.

If the queue is empty, the function returns immediately.

The task does not enter the Blocked state because the waiting time is zero.

Conceptually:

```text
Queue has data?
      |
   +--+--+
   |     |
  YES    NO
   |     |
Receive  Return immediately
```

This can be useful in some situations, but repeatedly calling a non-blocking queue receive inside a tight loop can create unnecessary polling.

For example:

```c
while (1)
{
    xQueueReceive(
        packetQueue,
        &packet,
        0
    );
}
```

If the queue remains empty, this task may continuously consume CPU time.

For a task whose only purpose is to process incoming packets, blocking is often a better design.

---

# 10. Why Blocking Is Important

Consider this version:

```c
void NGHamTask(void *pvParameters)
{
    Packet packet;

    while (1)
    {
        if (packetAvailable())
        {
            getPacket(&packet);

            encodeNGHamFrame(&packet);
        }
    }
}
```

If no packet is available, the task repeatedly executes:

```text
Check
Check
Check
Check
Check
Check
```

The task remains Ready or Running and may consume CPU time.

Now compare:

```c
void NGHamTask(void *pvParameters)
{
    Packet packet;

    while (1)
    {
        xQueueReceive(
            packetQueue,
            &packet,
            portMAX_DELAY
        );

        encodeNGHamFrame(&packet);
    }
}
```

When no packet exists:

```text
NGHam Task -> BLOCKED
```

The task consumes no normal CPU execution time while waiting.

This allows other Ready tasks to execute.

For the internship pipeline, this is the desired general pattern:

```text
No packet available
        |
        v
NGHam Task BLOCKED
        |
        | Packet arrives
        v
NGHam Task READY
        |
        | Scheduler selects it
        v
NGHam Task RUNNING
```

---

# 11. Suspended State

A task can also enter the **Suspended** state.

For example:

```c
vTaskSuspend(taskHandle);
```

A Suspended task does not participate in normal scheduling.

Unlike a Blocked task, it does not automatically become Ready because:

- A delay expired.
- Queue data arrived.
- A timeout expired.

It must normally be explicitly resumed:

```c
vTaskResume(taskHandle);
```

The state transition is:

```text
RUNNING / READY
       |
       | vTaskSuspend()
       v
   SUSPENDED
       |
       | vTaskResume()
       v
      READY
```

A task can also suspend itself:

```c
vTaskSuspend(NULL);
```

Here, `NULL` refers to the currently Running task.

---

# 12. Blocked vs Suspended

These states should not be confused.

| Blocked | Suspended |
|---|---|
| Waiting for time, data, or event | Explicitly removed from normal scheduling |
| Can automatically become Ready | Does not normally become Ready automatically |
| Used frequently in normal task operation | Used when a task must be explicitly disabled |
| Example: Waiting for queue data | Example: Temporarily disabling a task |

For example:

```text
NGHam Task waiting for packet
        |
        v
BLOCKED
```

But:

```text
NGHam processing manually disabled
        |
        v
SUSPENDED
```

For normal producer-consumer communication, the Blocked state is usually the relevant one.

---

# 13. Deleted State

A task can be deleted using:

```c
vTaskDelete(taskHandle);
```

A task can delete itself:

```c
vTaskDelete(NULL);
```

After deletion, the task is removed from normal scheduling.

Conceptually:

```text
RUNNING
    |
    | vTaskDelete(NULL)
    v
DELETED
```

When dynamic task allocation is used, the Idle Task is responsible for reclaiming memory associated with a task that deletes itself.

Therefore, the Idle Task must be allowed to run.

For long-running embedded firmware, many application tasks are created during initialization and continue existing for the entire lifetime of the system.

For example:

```text
Payload Task
NGHam Task
Radio Task
```

These tasks may run indefinitely rather than being repeatedly created and deleted.

---

# 14. Complete Task-State Transition Model

A more complete simplified model is:

```text
                           Higher-priority task runs
                    +-----------------------------------+
                    |                                   |
                    v                                   |
               +---------+                              |
               |  READY  | --------------------+        |
               +----+----+                     |        |
                    |                          |        |
                    | Scheduler selects        |        |
                    v                          |        |
               +---------+                     |        |
               | RUNNING | --------------------+--------+
               +----+----+
                    |
          +---------+---------+
          |                   |
          | Wait for          | Suspend
          | time/data/event   |
          v                   v
      +---------+        +-----------+
      | BLOCKED |        | SUSPENDED |
      +----+----+        +-----+-----+
           |                   |
           | Event/timeout     | Resume
           | occurs            |
           v                   v
        +-------------------------+
        |          READY          |
        +-------------------------+
```

Deletion can remove a task from normal scheduling:

```text
RUNNING
   |
   | vTaskDelete()
   v
DELETED
```

This diagram is simplified but represents the main concepts needed for my current learning stage.

---

# 15. State Transitions in the Internship Pipeline

Now I can apply all of this directly to the payload packet-handling system.

Suppose:

```text
Payload Task    Priority 2
NGHam Task      Priority 3
```

Initially:

```text
Packet Queue    EMPTY

Payload Task    RUNNING
NGHam Task      BLOCKED
```

The NGHam Task is blocked here:

```c
xQueueReceive(
    packetQueue,
    &packet,
    portMAX_DELAY
);
```

The Payload Task reads simulated payload memory:

```text
Payload Data:

[Byte 0]
[Byte 1]
[Byte 2]
...
[Byte N]
```

It creates a packet:

```text
Sequence Number = 1
Occultation ID  = 42
Payload Size    = 64 bytes
```

Then:

```c
xQueueSend(
    packetQueue,
    &packet,
    portMAX_DELAY
);
```

Now:

```text
NGHam Task:

BLOCKED -> READY
```

Since the NGHam Task has higher priority:

```text
Payload Task    READY
NGHam Task      RUNNING
```

The NGHam Task receives the packet:

```text
Packet Queue:

EMPTY
```

It processes the packet and creates the NGHam-like frame.

Then it tries:

```c
xQueueReceive(
    packetQueue,
    &packet,
    portMAX_DELAY
);
```

If the queue remains empty:

```text
NGHam Task:

RUNNING -> BLOCKED
```

The Payload Task can become Running again.

The cycle becomes:

```text
Payload Task RUNNING
        |
        | Creates packet
        v
Packet Queue receives data
        |
        v
NGHam Task BLOCKED -> READY
        |
        v
Scheduler selects NGHam Task
        |
        v
NGHam Task RUNNING
        |
        | Processes packet
        v
Queue becomes empty
        |
        v
NGHam Task BLOCKED
        |
        v
Payload Task can run again
```

This is the basic behavior of a producer-consumer pipeline under FreeRTOS.

---

# 16. What Happens When Multiple Tasks Are Ready?

Suppose:

```text
Payload Task    Priority 2    READY
NGHam Task      Priority 3    READY
Radio Task      Priority 4    BLOCKED
```

The scheduler chooses:

```text
NGHam Task
```

because it is the highest-priority Ready task.

Now suppose the Radio Task becomes Ready:

```text
Payload Task    Priority 2    READY
NGHam Task      Priority 3    RUNNING
Radio Task      Priority 4    READY
```

In a preemptive configuration:

```text
NGHam Task -> READY
Radio Task -> RUNNING
```

The state of a task therefore depends on both:

```text
What the task is waiting for
```

and:

```text
What other tasks are currently Ready
```

---

# 17. A Task Should Usually Block When It Has Nothing to Do

A poorly designed task might be:

```c
void NGHamTask(void *pvParameters)
{
    while (1)
    {
        if (packetAvailable())
        {
            processPacket();
        }
    }
}
```

If no packet is available, this task may continue consuming CPU time.

A better FreeRTOS design is:

```c
void NGHamTask(void *pvParameters)
{
    Packet packet;

    while (1)
    {
        if (xQueueReceive(
                packetQueue,
                &packet,
                portMAX_DELAY
            ) == pdPASS)
        {
            processPacket(&packet);
        }
    }
}
```

Now:

```text
No packet
    |
    v
Task BLOCKED

Packet arrives
    |
    v
Task READY
```

This is generally more efficient and fits naturally with the FreeRTOS scheduler.

---

# 18. `portMAX_DELAY`

In several examples, I use:

```c
portMAX_DELAY
```

For example:

```c
xQueueReceive(
    packetQueue,
    &packet,
    portMAX_DELAY
);
```

Conceptually, this means:

> Wait for the maximum supported blocking time.

In many common FreeRTOS configurations, this is used to express waiting indefinitely for the required event.

However, exact behavior can depend on FreeRTOS configuration, including settings related to tick overflow and task suspension.

For my current learning stage, the important understanding is:

```text
xQueueReceive(..., 0)

Do not wait.


xQueueReceive(..., pdMS_TO_TICKS(1000))

Wait up to approximately one second.


xQueueReceive(..., portMAX_DELAY)

Wait for the maximum supported blocking period, commonly used as an indefinite wait.
```

---

# 19. State Does Not Equal Priority

Task state and task priority are separate concepts.

For example:

```text
Radio Task      Priority 5    BLOCKED

NGHam Task      Priority 3    READY

Payload Task    Priority 2    RUNNING
```

The Radio Task has the highest priority, but it cannot run because it is Blocked.

The NGHam Task is Ready and has higher priority than the currently Running Payload Task.

In a preemptive configuration, the scheduler can switch to NGHam.

The important rule remains:

> Priority matters among tasks that are Ready to execute.

---

# 20. Task State and CPU Usage

A simplified view is:

| Task State | Competes for CPU? |
|---|---|
| Running | Currently using CPU |
| Ready | Yes |
| Blocked | No |
| Suspended | No |
| Deleted | No longer normally scheduled |

This is why good FreeRTOS design often tries to keep tasks Blocked when they have no useful work.

For example:

```text
Payload Task:

Blocked until payload data is available.


NGHam Task:

Blocked until a packet is available.


Radio Task:

Blocked until a frame or transmission event is available.
```

This is more efficient than making every task continuously poll for work.

---

# 21. Important Misconceptions

## Misconception 1: A Blocked task is an error

No.

Blocking is a normal and important part of FreeRTOS task design.

---

## Misconception 2: A Ready task is currently executing

No.

A Ready task is capable of executing but does not currently have the CPU.

---

## Misconception 3: When a delay expires, the task immediately becomes Running

Not necessarily.

It first becomes Ready. The scheduler decides when it runs.

---

## Misconception 4: A Suspended task automatically wakes when data arrives

No.

A Suspended task normally requires an explicit resume operation.

---

## Misconception 5: A higher-priority task always runs

No.

It must first be in the Ready state.

---

## Misconception 6: Blocking wastes CPU time

No.

Blocking allows other Ready tasks to use the CPU.

---

# 22. What I Learned From This Lesson

After completing this lesson, my understanding is:

> A FreeRTOS task moves between different states during execution. A Running task currently owns the CPU. A Ready task can run but is waiting for CPU access. A Blocked task is waiting for time, data, or an event and does not compete for CPU time. A Suspended task is explicitly removed from normal scheduling until resumed. A Deleted task is removed from normal task execution.

For my internship pipeline:

```text
Payload Task
    |
    | Creates packet
    v
Packet Queue
    |
    | Wakes waiting consumer
    v
NGHam Task
```

The important state flow is:

```text
NGHam Task BLOCKED
        |
        | Packet arrives
        v
NGHam Task READY
        |
        | Scheduler selects task
        v
NGHam Task RUNNING
        |
        | Queue becomes empty
        v
NGHam Task BLOCKED
```

This allows the MCU to use CPU time efficiently.

---

# 23. Key Takeaways

- A task can be Running, Ready, Blocked, Suspended, or Deleted.
- A Running task currently owns the CPU.
- A Ready task can execute but is waiting for CPU access.
- A Blocked task waits for time, data, queue space, or another event.
- A Blocked task does not compete for CPU time.
- `vTaskDelay()` moves a task from Running to Blocked.
- `xQueueReceive()` can block a consumer when the queue is empty.
- `xQueueSend()` can block a producer when the queue is full.
- A timeout can move a task from Blocked back to Ready.
- A task becomes Ready before it becomes Running.
- A Suspended task normally requires an explicit resume operation.
- A Deleted task no longer participates in normal scheduling.
- Good FreeRTOS tasks usually block when they have no useful work.
- For the internship pipeline, queue operations naturally control when the Payload and NGHam tasks become Blocked or Ready.

---

# 24. Next Lesson

The next lesson is:

# Lesson 4: Tasks and the Task Control Block

I will study:

- What exactly a FreeRTOS task is.
- The structure of a task function.
- Why tasks commonly contain infinite loops.
- How tasks are created.
- What a Task Control Block is.
- What information FreeRTOS stores for each task.
- What a task handle is.
- How the scheduler keeps track of different tasks.
- How this applies to the Payload Task and NGHam Task in my internship pipeline.

The connected architecture remains:

```text
Payload Storage Memory
        |
        v
Payload Task
        |
        v
FIFO Packet Queue
        |
        v
NGHam Task
        |
        v
Transmitter
```

---

## Progress

| Lesson | Topic | Status |
|---|---|---|
| 1 | Introduction to FreeRTOS | Completed |
| 2 | Scheduler and Context Switching | Completed |
| 3 | Task States | Completed |
| 4 | Tasks and Task Control Block | Next |
| 5 | Stack | Upcoming |
| 6 | Queues | Upcoming |
| 7 | Core FreeRTOS APIs | Upcoming |
| 8 | Synchronization | Upcoming |
| 9 | Memory Management and Project Structure | Upcoming |

---

*These notes document my understanding while learning FreeRTOS for an embedded systems internship involving an MSP430-based payload data-handling and communication system.*
