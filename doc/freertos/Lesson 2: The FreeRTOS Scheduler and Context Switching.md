# Lesson 2: The FreeRTOS Scheduler and Context Switching

## Continuing My FreeRTOS Learning Journey

In Lesson 1, I learned what FreeRTOS is, why embedded systems may need an RTOS, and how a complex application can be divided into separate tasks.

The connected example I am using throughout my FreeRTOS learning journey comes from my summer internship work. The basic architecture is:

```text
Payload Storage Memory
        |
        v
Payload Reader / Fragmentation Task
        |
        v
FIFO Packet Queue
        |
        v
NGHam Processing Task
        |
        v
Transmitter
```

The actual project uses a microcontroller from the **MSP430 family**.

In this lesson, I focus on one of the most important parts of FreeRTOS: the **scheduler**.

The main question is:

> If a microcontroller has only one CPU core, how can several FreeRTOS tasks make progress?

To answer this question, I need to understand:

- The scheduler
- Context switching
- Task priorities
- Preemption
- Time slicing
- Blocking
- The system tick
- The Idle Task

---

# 1. The Basic Problem: One CPU, Multiple Tasks

Suppose my embedded application contains these four tasks:

```text
Payload Task
NGHam Task
Radio Task
Battery Task
```

A common beginner misconception is that all four tasks must literally execute at exactly the same instant.

On a **single-core microcontroller**, that is generally not true.

At one exact instant, one CPU core normally executes only one stream of instructions.

So the actual execution may look something like this:

```text
Time ---------------------------------------------------->

Payload   | RUN |     |     | RUN |     |
NGHam     |     | RUN |     |     | RUN |
Radio     |     |     | RUN |     |     |
```

The CPU switches between tasks.

If this switching happens quickly and efficiently, several tasks appear to progress concurrently.

This is called **multitasking**.

The important point is:

> Multitasking does not necessarily mean that multiple tasks are physically executing at the exact same instant.

On a single-core processor, the CPU rapidly switches between different tasks.

---

# 2. What Is the Scheduler?

The **scheduler** is the part of the FreeRTOS kernel that decides:

> Which task should use the CPU now?

Imagine three tasks:

```text
+------------------+
|   Payload Task   |
+------------------+

+------------------+
|    NGHam Task    |
+------------------+

+------------------+
|    Radio Task    |
+------------------+
```

The scheduler manages their execution:

```text
                    +---------------------+
                    |  FreeRTOS Scheduler |
                    +----------+----------+
                               |
              +----------------+----------------+
              |                |                |
              v                v                v
        Payload Task      NGHam Task       Radio Task
```

The scheduler does not:

- Make the CPU faster
- Create additional CPU cores
- Execute every task simultaneously

Instead, it organizes access to the available CPU.

Its job is essentially:

```text
Which tasks are ready?

        |

        v

Which ready task has the highest priority?

        |

        v

Run that task.
```

---

# 3. A Real-Life Analogy

Imagine one person handling three activities:

```text
1. Cook food
2. Wash clothes
3. Reply to messages
```

The person starts cooking, but the food must stay in the oven for 20 minutes.

It would be wasteful to stand in front of the oven doing nothing for 20 minutes.

Instead:

```text
Start cooking
      |
      v
Food is waiting in oven
      |
      v
Wash clothes
      |
      v
Reply to messages
      |
      v
Return when food needs attention
```

The person is still only one person.

They are not physically performing all three CPU-intensive activities at exactly the same instant.

Instead, they intelligently use the time during which one activity is waiting.

This is similar to how tasks work in an RTOS.

A task that is waiting for:

- Time
- Data
- Hardware
- A queue item
- A signal
- An event

does not necessarily need to occupy the CPU.

The scheduler can give the CPU to another task that has useful work to perform.

---

# 4. Running and Ready Tasks

To understand scheduling, two task states are especially important:

```text
Running
Ready
```

We will study all task states properly in Lesson 3, but these two are necessary to understand the scheduler.

## Running State

A task is **Running** when the CPU is currently executing its instructions.

For example:

```text
CPU
 |
 v
Payload Task  <-- RUNNING
```

On a single-core processor, normally only one task can be in the Running state at one exact instant.

---

## Ready State

A task is **Ready** when:

- It is capable of running.
- It is not waiting for anything.
- But another task currently has the CPU.

Example:

```text
Running:

    Payload Task


Ready:

    NGHam Task
    Radio Task
```

The NGHam and Radio tasks are ready to execute immediately if selected by the scheduler.

A useful analogy is a barber shop.

```text
Person currently getting haircut = Running

People waiting for their turn = Ready
```

The people waiting are completely capable of getting a haircut. They are simply waiting for access to the barber.

Similarly, Ready tasks are waiting for CPU access.

---

# 5. What Is Context Switching?

Suppose the Payload Task is currently running:

```c
void PayloadTask(void *pvParameters)
{
    int sequence = 0;

    while (1)
    {
        sequence++;

        createPacket(sequence);
    }
}
```

At some point:

```text
sequence = 27
```

Now suppose the scheduler decides that another task should run.

The Payload Task might currently have:

- Local variables
- Function calls in progress
- Values stored in CPU registers
- A particular stack pointer
- A specific current execution location

FreeRTOS cannot simply forget all this information.

Otherwise, when the Payload Task runs again, it would have no idea where it stopped.

Therefore, the execution state must be saved.

The process of stopping one task and switching to another task is called a:

> **Context Switch**

Conceptually:

```text
Task A is running
      |
      v
Save Task A context
      |
      v
Select Task B
      |
      v
Restore Task B context
      |
      v
Task B continues running
```

Later:

```text
Task B stops
      |
      v
Save Task B context
      |
      v
Restore Task A context
      |
      v
Task A continues exactly where it stopped
```

---

# 6. What Is a Task Context?

A task's **context** is the execution information needed to pause and later resume that task correctly.

Depending on the CPU architecture and FreeRTOS port, this may include:

- CPU registers
- Stack pointer
- Program execution state
- Architecture-specific processor state

A useful analogy is pausing a video game.

Suppose I pause a game.

To resume later, the game needs to remember:

```text
Player position
Health
Weapons
Current mission
Inventory
Current location
```

Without this saved information, the game could not continue from where I stopped.

Similarly, when a task is switched out, its execution context must be preserved.

---

# 7. Example of Context Switching

Suppose we have:

```c
void PayloadTask(void *pvParameters)
{
    int packetNumber = 0;

    while (1)
    {
        packetNumber++;

        createPacket(packetNumber);
    }
}
```

The CPU executes:

```text
packetNumber = 0

packetNumber++

packetNumber = 1
```

Then the scheduler switches to another task.

The Payload Task is paused.

Later, the scheduler returns to it.

The task must continue knowing:

```text
packetNumber = 1
```

It should not restart from:

```text
packetNumber = 0
```

This is possible because the task's execution state is preserved.

---

# 8. Why Every Task Needs Its Own Stack

This connects directly to Lesson 5, but it is useful to introduce the idea here.

Suppose my application has:

```text
Payload Task
NGHam Task
Radio Task
```

Each task can:

- Call different functions
- Have different local variables
- Be paused at different points
- Have different execution states

For example:

```c
void PayloadTask(void *pvParameters)
{
    uint16_t sequenceNumber = 0;

    Packet packet;

    while (1)
    {
        createPacket(&packet, sequenceNumber);

        sequenceNumber++;
    }
}
```

Another task:

```c
void NGHamTask(void *pvParameters)
{
    NGHamFrame frame;

    while (1)
    {
        encodeFrame(&frame);
    }
}
```

Each task therefore needs its own stack.

Conceptually:

```text
Payload Task
    |
    +---- Payload Stack


NGHam Task
    |
    +---- NGHam Stack


Radio Task
    |
    +---- Radio Stack
```

The stack helps maintain:

- Local variables
- Function calls
- Return information
- Saved execution state

During a context switch, FreeRTOS moves from the execution context of one task to another.

---

# 9. When Does the Scheduler Switch Tasks?

There are several reasons why the currently Running task may stop running and another task may execute.

Important examples include:

1. The Running task blocks.
2. The Running task calls `vTaskDelay()`.
3. A higher-priority task becomes Ready.
4. Equal-priority tasks share CPU time when time slicing is enabled.
5. The Running task voluntarily yields.
6. An interrupt causes a higher-priority task to become Ready.

The exact behavior depends on:

- Task priorities
- Task states
- FreeRTOS configuration
- Scheduler configuration

---

# 10. Case 1: A Task Calls `vTaskDelay()`

Suppose we have a Battery Task:

```c
void BatteryTask(void *pvParameters)
{
    while (1)
    {
        checkBattery();

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

The task performs:

```text
Check battery

      |

      v

Wait 5 seconds

      |

      v

Check battery again
```

After checking the battery, the task does not need the CPU for the next five seconds.

So it calls:

```c
vTaskDelay(pdMS_TO_TICKS(5000));
```

The task becomes **Blocked**.

```text
Battery Task
    |
    | checkBattery()
    v
vTaskDelay(...)
    |
    v
BLOCKED
    |
    | Delay expires
    v
READY
    |
    | Scheduler selects it
    v
RUNNING
```

While the Battery Task is blocked, another Ready task can use the CPU.

---

# 11. Busy Waiting vs Blocking

Suppose I want to create a delay.

I could write:

```c
for (volatile long i = 0; i < 1000000; i++)
{
    // Do nothing
}
```

The CPU is actively executing this loop.

It is consuming CPU cycles without performing useful work.

This is called:

> **Busy Waiting**

Conceptually:

```text
CPU
 |
 v
Loop
Loop
Loop
Loop
Loop
Loop
```

The CPU remains busy.

Now compare this with:

```c
vTaskDelay(pdMS_TO_TICKS(1000));
```

The task becomes Blocked.

Now:

```text
Current Task
    |
    v
BLOCKED

CPU
    |
    v
Runs another Ready Task
```

This is one of the biggest advantages of RTOS-based programming.

The important distinction is:

```text
Busy waiting:
The task uses CPU while accomplishing no useful work.

Blocking:
The task stops competing for CPU until something relevant happens.
```

---

# 12. Case 2: Waiting for Queue Data

This directly connects to my internship.

Suppose the NGHam Task needs a packet:

```c
xQueueReceive(
    packetQueue,
    &packet,
    portMAX_DELAY
);
```

What if the queue is empty?

A bad approach would be:

```c
while (1)
{
    if (packetAvailable())
    {
        processPacket();
    }
}
```

The CPU repeatedly checks:

```text
Packet available?

No.

Packet available?

No.

Packet available?

No.
```

This is polling and may waste CPU time.

With a FreeRTOS queue, the NGHam Task can block.

```text
NGHam Task
    |
    v
xQueueReceive()
    |
    | Queue empty
    v
BLOCKED
```

Now the Payload Task creates a packet:

```c
xQueueSend(
    packetQueue,
    &packet,
    portMAX_DELAY
);
```

The waiting NGHam Task can become Ready.

```text
Payload Task
    |
    | Sends Packet 42
    v
Packet Queue
    |
    v
NGHam Task becomes READY
```

Whether it immediately becomes Running depends on:

- Its priority
- The priority of the currently Running task
- Scheduler configuration

---

# 13. Task Priorities

FreeRTOS tasks have priorities.

For example:

```text
Radio Task      Priority 4

NGHam Task      Priority 3

Payload Task    Priority 2

LED Task        Priority 1
```

The fundamental scheduling idea is:

> Among tasks that are Ready, the scheduler selects a task at the highest Ready priority.

This is extremely important.

Suppose:

```text
Radio Task      Priority 4    BLOCKED

NGHam Task      Priority 3    BLOCKED

Payload Task    Priority 2    READY

LED Task        Priority 1    READY
```

Which task runs?

The **Payload Task**.

Why?

Because the higher-priority Radio and NGHam tasks are Blocked.

Among the Ready tasks:

```text
Payload Priority = 2
LED Priority     = 1
```

Therefore, Payload is the highest-priority Ready task.

---

# 14. Higher Priority Does Not Mean Always Running

This is an important misconception.

Suppose:

```text
Radio Task      Priority 5
Payload Task    Priority 2
```

It does not mean the Radio Task permanently owns the CPU.

The Radio Task may be:

```text
BLOCKED
```

because it is waiting for:

- A frame
- A hardware event
- A queue item
- A notification

While it is blocked, the lower-priority Payload Task can run.

So the correct rule is:

> The scheduler selects the highest-priority task that is currently Ready.

Not simply:

> The task with the highest priority always runs.

---

# 15. Preemption

Suppose:

```text
Radio Task      Priority 4    BLOCKED

Payload Task    Priority 2    RUNNING
```

The Payload Task currently has the CPU.

Now an event occurs and the Radio Task becomes Ready:

```text
Radio Task      Priority 4    READY

Payload Task    Priority 2    RUNNING
```

In a **preemptive FreeRTOS configuration**, the higher-priority Radio Task can preempt the lower-priority Payload Task.

The process is:

```text
Payload Task running
        |
        v
Radio Task becomes Ready
        |
        v
Scheduler sees higher priority
        |
        v
Save Payload context
        |
        v
Restore Radio context
        |
        v
Radio Task runs
```

The Payload Task is not destroyed.

Its context is saved.

Later, when the Radio Task blocks again, the Payload Task may continue from exactly where it stopped.

---

# 16. Connected Internship Example: Payload and NGHam Tasks

Suppose my system contains:

```text
Payload Task      Priority 2

NGHam Task        Priority 3
```

Initially:

```text
Payload Task      RUNNING

NGHam Task        BLOCKED waiting for packet
```

The Payload Task creates:

```text
Packet:

Sequence Number = 17

Occultation ID  = 52

Payload Size    = 64 bytes
```

It sends the packet:

```c
xQueueSend(
    packetQueue,
    &packet,
    portMAX_DELAY
);
```

The NGHam Task was blocked waiting for queue data.

Now it becomes Ready.

The states become:

```text
Payload Task      RUNNING

NGHam Task        READY
```

Since the NGHam Task has higher priority:

```text
NGHam Priority   = 3

Payload Priority = 2
```

In a preemptive configuration, the scheduler may switch immediately.

```text
Before:

Payload Task      RUNNING
NGHam Task        BLOCKED


Payload sends Packet 17


After packet arrives:

Payload Task      RUNNING
NGHam Task        READY


Scheduler decision:

Payload Task      READY
NGHam Task        RUNNING
```

The NGHam Task processes Packet 17.

Then it requests another packet:

```c
xQueueReceive(
    packetQueue,
    &packet,
    portMAX_DELAY
);
```

If the queue is empty:

```text
NGHam Task -> BLOCKED
```

Now the Payload Task can run again.

This is a very important producer-consumer pattern.

---

# 17. Time Slicing

What happens if two tasks have the same priority?

For example:

```text
Task A    Priority 2

Task B    Priority 2
```

If both remain Ready and time slicing is enabled, FreeRTOS can share CPU time between them.

Conceptually:

```text
Time ------------------------------------------------>

Task A    | RUN |     | RUN |     | RUN |

Task B    |     | RUN |     | RUN |     |
```

This is often described as round-robin behavior among equal-priority Ready tasks.

However, an important detail is:

> Time slicing behavior depends on the FreeRTOS configuration.

For example:

```c
configUSE_TIME_SLICING
```

affects this behavior.

Therefore, I should not assume that every FreeRTOS project uses exactly the same scheduling configuration.

I should inspect:

```text
FreeRTOSConfig.h
```

in the actual project.

---

# 18. What Is a Tick?

FreeRTOS needs a way to track time.

Many FreeRTOS systems use a periodic interrupt called the:

> **Tick Interrupt**

Conceptually:

```text
Time ------------------------------------------------>

Tick 0

        Tick 1

                Tick 2

                        Tick 3

                                Tick 4
```

The tick helps the kernel manage operations such as:

- Task delays
- Timeouts
- Time slicing, depending on configuration

Suppose:

```c
vTaskDelay(pdMS_TO_TICKS(1000));
```

The task asks FreeRTOS to block it for the corresponding number of ticks.

The tick frequency is commonly configured using:

```c
configTICK_RATE_HZ
```

For example:

```text
configTICK_RATE_HZ = 1000
```

means approximately:

```text
1000 ticks per second

1 tick approximately equals 1 millisecond
```

But another system might use:

```text
configTICK_RATE_HZ = 100
```

Then:

```text
100 ticks per second

1 tick approximately equals 10 milliseconds
```

The exact configuration depends on the project.

---

# 19. Why Not Always Use a Very High Tick Rate?

Suppose:

```text
100 Hz
```

The tick interrupt occurs every:

```text
10 ms
```

Now suppose:

```text
1000 Hz
```

The tick interrupt occurs every:

```text
1 ms
```

A higher tick frequency provides finer timing resolution.

However, every tick interrupt requires CPU processing.

Therefore:

```text
Higher tick frequency
        |
        +---- Better timing resolution
        |
        +---- More interrupt overhead
```

This is an engineering trade-off.

For a resource-constrained MCU such as an MSP430-family device, unnecessary overhead may matter.

For my internship, the correct approach is not to randomly choose a tick rate.

Instead, I should inspect the existing TT&C firmware and understand why its configuration was selected.

---

# 20. The Idle Task

Suppose every application task is blocked:

```text
Payload Task      BLOCKED

NGHam Task        BLOCKED

Radio Task        BLOCKED

Battery Task      BLOCKED
```

What runs now?

FreeRTOS automatically creates a special task called the:

> **Idle Task**

Conceptually:

```text
No application task is Ready
          |
          v
      Idle Task runs
```

The Idle Task has the lowest priority.

It also performs some internal kernel responsibilities.

For example, when dynamic allocation is used, it is involved in cleaning up resources associated with tasks that delete themselves.

Therefore, the Idle Task must be allowed CPU time when required.

---

# 21. Does the Scheduler Check Every Task One by One?

As a beginner, I might imagine the scheduler doing this:

```c
while (1)
{
    checkTask1();

    checkTask2();

    checkTask3();

    checkTask4();
}
```

But that is not an accurate mental model of how FreeRTOS scheduling is organized.

Internally, FreeRTOS maintains lists for tasks.

Conceptually:

```text
Ready Priority 4:
    Radio Task


Ready Priority 3:
    NGHam Task


Ready Priority 2:
    Payload Task


Ready Priority 1:
    LED Task


Blocked:
    Battery Task
```

The scheduler uses internal data structures to manage tasks according to their:

- State
- Priority
- Delay status
- Event waiting status

The exact internal implementation is more advanced, but the important concept is:

> The scheduler manages task states and priority-based ready lists rather than treating every task as a manually checked function in one giant application loop.

---

# 22. Context Switching Has a Cost

Context switching is powerful, but it is not free.

During a context switch, the system must perform work such as:

```text
Save current task context
        |
        v
Update kernel scheduling information
        |
        v
Select another task
        |
        v
Restore the new task's context
```

This requires CPU cycles.

Therefore, creating unnecessary tasks or forcing excessive context switches can waste processing time.

A bad design would be:

```text
Task 1 -> Does one tiny operation

Task 2 -> Does another tiny operation

Task 3 -> Does another tiny operation

Task 4 -> Does another tiny operation

Task 5 -> Does another tiny operation
```

Tasks should represent meaningful independent activities.

For my internship, logical task boundaries might include:

```text
Payload handling

Packet processing

NGHam encoding

Transmission
```

The exact architecture should follow the actual requirements and existing firmware design.

---

# 23. Priority Does Not Mean Importance in a Human Sense

Suppose I think:

> Payload data is the most important part of my internship, so the Payload Task should have the highest priority.

That reasoning may be incorrect.

Priority should primarily reflect scheduling and timing requirements.

Consider:

```text
Task A:

Processes extremely valuable scientific data

Deadline = 10 seconds


Task B:

Handles a small communication event

Must respond within 2 milliseconds
```

Task B may need a higher scheduling priority even though Task A handles more valuable data.

Therefore, task priority should consider:

- Deadlines
- Latency requirements
- Response-time requirements
- Task dependencies
- Worst-case execution behavior
- Hardware timing requirements

Priority is an engineering decision.

---

# 24. Starvation

Consider:

```text
High Priority Task    Priority 5

Low Priority Task     Priority 1
```

Suppose the high-priority task is always Ready and never blocks.

For example:

```c
void HighPriorityTask(void *pvParameters)
{
    while (1)
    {
        doWork();

        // Never delays
        // Never blocks
        // Never waits
    }
}
```

Since this task permanently remains the highest-priority Ready task, lower-priority tasks may receive little or no CPU time.

This problem is called:

> **Starvation**

Conceptually:

```text
High Priority Task

RUN RUN RUN RUN RUN RUN RUN RUN


Low Priority Task

WAIT WAIT WAIT WAIT WAIT WAIT WAIT
```

This is why RTOS task design and priority selection must be done carefully.

---

# 25. A Complete Connected Internship Timeline

Suppose the system contains:

```text
Payload Task     Priority 2

NGHam Task       Priority 3

Radio Task       Priority 4
```

Initially:

```text
Payload Task     RUNNING

NGHam Task       BLOCKED waiting for packet

Radio Task       BLOCKED waiting for frame
```

## Step 1: Payload Task Creates Packet 1

```text
Packet 1

Sequence Number = 1

Occultation ID = 25

Payload Size = 64 bytes
```

The Payload Task sends the packet into the FIFO queue.

```text
Payload Task
      |
      v
+----------------+
|    Packet 1    |
+----------------+
```

---

## Step 2: NGHam Task Becomes Ready

The NGHam Task was waiting for a packet.

Now data is available.

```text
NGHam Task:

BLOCKED -> READY
```

Since:

```text
NGHam Priority   = 3

Payload Priority = 2
```

the scheduler can select the NGHam Task.

```text
Payload Task -> READY

NGHam Task   -> RUNNING
```

---

## Step 3: NGHam Task Creates a Frame

The NGHam Task receives Packet 1 and performs the required encoding or framing.

```text
Packet 1
    |
    v
NGHam Processing
    |
    v
Encoded Frame 1
```

The resulting frame becomes available for transmission.

---

## Step 4: Radio Task Becomes Ready

The Radio Task was waiting for a frame.

Now:

```text
Radio Task:

BLOCKED -> READY
```

Since:

```text
Radio Priority = 4

NGHam Priority = 3
```

the Radio Task can preempt the NGHam Task.

```text
NGHam Task -> READY

Radio Task -> RUNNING
```

---

## Step 5: Radio Task Handles Transmission

The Radio Task handles the frame.

Later, it may wait for another frame or hardware event.

Then:

```text
Radio Task -> BLOCKED
```

The scheduler selects the next highest-priority Ready task.

The complete pipeline is:

```text
+---------------------------+
|   Payload Storage Memory  |
+-------------+-------------+
              |
              v
+---------------------------+
|       Payload Task        |
|                           |
| - Read memory             |
| - Fragment data           |
| - Add packet metadata     |
+-------------+-------------+
              |
              | Packet
              v
+---------------------------+
|     FIFO Packet Queue     |
+-------------+-------------+
              |
              v
+---------------------------+
|        NGHam Task         |
|                           |
| - Receive packet          |
| - Create NGHam-like frame |
+-------------+-------------+
              |
              | Frame
              v
+---------------------------+
|        Radio Task         |
|                           |
| - Receive frame           |
| - Handle transmission     |
+---------------------------+
```

This example connects:

- Tasks
- Scheduler
- Priorities
- Context switching
- Blocking
- Queues
- Preemption

into one complete flow.

---

# 26. Cooperative vs Preemptive Scheduling

There are two general scheduling styles worth understanding.

## Preemptive Scheduling

A higher-priority task can take the CPU when it becomes Ready.

```text
Low-priority task running
        |
        v
Higher-priority task becomes Ready
        |
        v
Context switch
        |
        v
Higher-priority task runs
```

A FreeRTOS configuration may contain:

```c
#define configUSE_PREEMPTION 1
```

---

## Cooperative Scheduling

In cooperative scheduling, a running task keeps control until it voluntarily:

- Yields
- Blocks
- Delays

Conceptually:

```text
Task A runs
    |
    v
Task A voluntarily gives up CPU
    |
    v
Scheduler chooses another task
```

Preemptive scheduling is very common in FreeRTOS applications, but the actual configuration must always be checked.

---

# 27. Scheduler Configuration Matters

Not every FreeRTOS project behaves identically.

Important configuration options may include:

```text
configUSE_PREEMPTION

configUSE_TIME_SLICING

configTICK_RATE_HZ

configMAX_PRIORITIES
```

These are normally defined in:

```text
FreeRTOSConfig.h
```

Therefore, when I eventually inspect the actual TT&C firmware, one of the most important files to study will be:

```text
FreeRTOSConfig.h
```

It tells me how FreeRTOS has been configured for that particular system.

---

# 28. Common Misconceptions

## Misconception 1: The scheduler runs every task equally

No.

Task priority, task state, and scheduler configuration affect execution.

---

## Misconception 2: The highest-priority task always runs

No.

The highest-priority task may be Blocked.

The scheduler chooses the highest-priority **Ready** task.

---

## Misconception 3: More context switches are always better

No.

Context switching has CPU overhead.

---

## Misconception 4: `vTaskDelay()` wastes CPU time

No.

The delayed task becomes Blocked, allowing other Ready tasks to run.

---

## Misconception 5: Two equal-priority tasks always time-slice

Not necessarily.

This depends on FreeRTOS configuration.

---

## Misconception 6: The scheduler creates additional CPU cores

No.

On a single-core MCU, it rapidly switches between tasks.

---

## Misconception 7: Highest priority means most important task

Not necessarily.

Priority should reflect timing and response requirements.

---

# 29. What I Learned From This Lesson

My understanding after completing this lesson is:

> The FreeRTOS scheduler manages which Ready task gets CPU time. On a single-core MCU, normally only one task executes at one exact instant. Tasks can become Blocked while waiting for time, queue data, semaphores, notifications, or other events. When a higher-priority task becomes Ready in a preemptive configuration, it can preempt a lower-priority Running task. During a context switch, one task's execution state is saved and another task's state is restored.

For my internship, I can now imagine the packet pipeline dynamically:

```text
Payload Task creates packet
        |
        v
Packet enters FIFO queue
        |
        v
NGHam Task becomes Ready
        |
        v
Scheduler may perform context switch
        |
        v
NGHam Task creates frame
        |
        v
Radio Task becomes Ready
        |
        v
Frame is handled for transmission
```

The scheduler is the component coordinating CPU execution across these independent tasks.

---

# 30. Key Takeaways

- The scheduler decides which Ready task gets CPU time.
- On a single-core MCU, normally only one task executes at one exact instant.
- A Ready task can run but is waiting for CPU access.
- A Running task currently owns the CPU.
- A Blocked task is waiting for time, data, or an event.
- A context switch saves one task's execution state and restores another's.
- Higher-priority Ready tasks are favored over lower-priority Ready tasks.
- In preemptive scheduling, a higher-priority task can preempt a lower-priority Running task.
- Equal-priority tasks may time-slice depending on configuration.
- The system tick helps FreeRTOS manage time and delays.
- The Idle Task runs when no higher-priority application task is Ready.
- Excessive context switching creates overhead.
- Poor priority design can cause starvation.
- `FreeRTOSConfig.h` determines important scheduler behavior.
- Priority should be based on timing requirements rather than human perception of importance.

---

# 31. Next Lesson

The next lesson is:

# Lesson 3: FreeRTOS Task States

I will study:

- Running
- Ready
- Blocked
- Suspended
- Deleted
- State transitions
- Why blocking is better than polling
- How queues cause tasks to block and wake
- How task states apply to the payload packet-handling pipeline

The connected internship example continues:

```text
Payload Memory
      |
      v
Payload Task
      |
      v
Packet Queue
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
| 3 | Task States | Next |
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

