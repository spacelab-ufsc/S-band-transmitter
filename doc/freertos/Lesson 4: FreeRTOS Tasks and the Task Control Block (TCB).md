# Lesson 4: FreeRTOS Tasks and the Task Control Block (TCB)

## Introduction

In the previous lessons, I learned:

- What FreeRTOS is.
- How the scheduler manages CPU time.
- How context switching works.
- How tasks move between Running, Ready, Blocked, Suspended, and Deleted states.

Now the next important question is:

> What exactly is a FreeRTOS task, and how does the kernel keep track of multiple tasks?

In this lesson, I focus on:

- What a FreeRTOS task actually is.
- The structure of a task function.
- Why tasks usually contain infinite loops.
- How tasks are created using `xTaskCreate()`.
- What a Task Control Block (TCB) is.
- What information FreeRTOS stores for every task.
- What a task handle is.
- How the scheduler manages multiple tasks.

The connected internship architecture remains:

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

---

# 1. What Is a FreeRTOS Task?

A FreeRTOS task is an independently scheduled unit of execution.

In C, a task begins as a function.

For example:

```c
void PayloadTask(void *pvParameters)
{
    while (1)
    {
        readPayloadMemory();
    }
}
```

This function becomes a FreeRTOS task when it is created using a FreeRTOS task-creation API such as:

```c
xTaskCreate(
    PayloadTask,
    "Payload",
    1024,
    NULL,
    2,
    NULL
);
```

The important distinction is:

```text
Normal C function:

Called directly by another function.


FreeRTOS task:

Created and managed by the FreeRTOS kernel.
Its execution is controlled by the scheduler.
```

For example, normally I might write:

```c
int main(void)
{
    PayloadTask(NULL);
}
```

This directly calls the function.

With FreeRTOS, I instead create the task:

```c
xTaskCreate(
    PayloadTask,
    "Payload",
    1024,
    NULL,
    2,
    NULL
);
```

Then the scheduler manages when the task executes.

---

# 2. Basic Structure of a Task Function

A typical FreeRTOS task function has this form:

```c
void TaskName(void *pvParameters)
{
    while (1)
    {
        // Task work
    }
}
```

For example:

```c
void PayloadTask(void *pvParameters)
{
    while (1)
    {
        readPayloadMemory();

        fragmentPayload();

        createPacket();
    }
}
```

The function takes one parameter:

```c
void *pvParameters
```

This allows information to be passed into the task when it is created.

We will examine task parameters later in this lesson.

---

# 3. Why Do Tasks Usually Have Infinite Loops?

A FreeRTOS task normally represents an activity that may need to run throughout the lifetime of the system.

For example, the Payload Task may repeatedly:

```text
Wait for payload data
        |
        v
Read payload data
        |
        v
Fragment data
        |
        v
Create packets
        |
        v
Wait for more payload data
```

Therefore, its basic structure may be:

```c
void PayloadTask(void *pvParameters)
{
    while (1)
    {
        waitForPayloadData();

        readPayloadMemory();

        fragmentPayload();

        createPackets();
    }
}
```

Similarly, the NGHam Task may repeatedly wait for packets:

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

The task remains alive, but when no packet exists, it becomes Blocked inside `xQueueReceive()`.

Therefore:

```text
Infinite loop does not necessarily mean:

Consume CPU forever.
```

A correctly designed task usually blocks when it has nothing useful to do.

---

# 4. A Task Function Should Not Normally Return

Consider:

```c
void PayloadTask(void *pvParameters)
{
    readPayloadMemory();

    fragmentPayload();

    createPacket();

    return;
}
```

A FreeRTOS task should not normally return like an ordinary function.

If a task has finished permanently, it should delete itself explicitly:

```c
void OneTimeTask(void *pvParameters)
{
    performWork();

    vTaskDelete(NULL);
}
```

Here:

```c
vTaskDelete(NULL);
```

means:

> Delete the currently running task.

For long-running tasks such as:

```text
Payload Task
NGHam Task
Radio Task
```

the normal design is usually:

```c
while (1)
{
    // Wait for work
    // Process work
}
```

---

# 5. Creating a Task With `xTaskCreate()`

A dynamically allocated FreeRTOS task can be created using:

```c
xTaskCreate();
```

The function signature is conceptually:

```c
BaseType_t xTaskCreate(
    TaskFunction_t pvTaskCode,
    const char * const pcName,
    configSTACK_DEPTH_TYPE usStackDepth,
    void *pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t *pxCreatedTask
);
```

Example:

```c
xTaskCreate(
    PayloadTask,
    "Payload",
    1024,
    NULL,
    2,
    NULL
);
```

Each argument has a specific purpose.

---

# 6. Parameter 1: Task Function

The first parameter specifies the function that should become a task:

```c
PayloadTask
```

Example:

```c
xTaskCreate(
    PayloadTask,
    "Payload",
    1024,
    NULL,
    2,
    NULL
);
```

The function must have the expected task-function form:

```c
void PayloadTask(void *pvParameters)
{
    while (1)
    {
        // Work
    }
}
```

Conceptually:

```text
Function:

PayloadTask

        |

        v

Registered with FreeRTOS

        |

        v

Managed as a task by the scheduler
```

---

# 7. Parameter 2: Task Name

The second parameter is a human-readable task name:

```c
"Payload"
```

Example:

```c
xTaskCreate(
    PayloadTask,
    "Payload",
    1024,
    NULL,
    2,
    NULL
);
```

The task name is useful for:

- Debugging.
- Kernel-aware debugger views.
- Runtime statistics.
- Identifying tasks during development.

The task name does not control scheduling.

For example:

```text
Task Name = "Payload"
Priority  = 2
```

The scheduler uses the priority and task state, not the text name, when deciding what runs.

---

# 8. Parameter 3: Stack Depth

The third parameter specifies the task's stack depth:

```c
1024
```

Example:

```c
xTaskCreate(
    PayloadTask,
    "Payload",
    1024,
    NULL,
    2,
    NULL
);
```

Each task needs its own stack.

The stack stores information such as:

- Local variables.
- Function call information.
- Return addresses.
- Saved processor context.

Conceptually:

```text
Payload Task
    |
    +---- TCB
    |
    +---- Stack
```

An important technical detail is:

> The stack-depth parameter is generally specified in units of `StackType_t`, not necessarily bytes.

Therefore:

```c
1024
```

does not universally mean exactly 1024 bytes.

The actual number of bytes depends on:

```c
sizeof(StackType_t)
```

and the architecture/FreeRTOS port.

This is especially important when working with the actual MSP430 target.

We will study stack memory properly in Lesson 5.

---

# 9. Parameter 4: Task Parameters

The fourth argument allows data to be passed into the task:

```c
void *pvParameters
```

If no parameter is needed:

```c
NULL
```

can be passed.

Example:

```c
xTaskCreate(
    PayloadTask,
    "Payload",
    1024,
    NULL,
    2,
    NULL
);
```

Inside the task:

```c
void PayloadTask(void *pvParameters)
{
    while (1)
    {
        // Task work
    }
}
```

If configuration data needs to be passed, I could define:

```c
typedef struct
{
    uint16_t occultationID;
    uint16_t chunkSize;

} PayloadConfig;
```

Then:

```c
PayloadConfig config;
```

The address could be passed to the task:

```c
xTaskCreate(
    PayloadTask,
    "Payload",
    1024,
    &config,
    2,
    NULL
);
```

Inside the task:

```c
void PayloadTask(void *pvParameters)
{
    PayloadConfig *config;

    config = (PayloadConfig *)pvParameters;

    while (1)
    {
        processPayload(
            config->occultationID,
            config->chunkSize
        );
    }
}
```

One important requirement is that the object being pointed to must remain valid for as long as the task uses it.

For example, care must be taken before passing the address of a local variable that may go out of scope.

---

# 10. Parameter 5: Task Priority

The fifth parameter specifies task priority:

```c
2
```

Example:

```c
xTaskCreate(
    PayloadTask,
    "Payload",
    1024,
    NULL,
    2,
    NULL
);
```

Suppose:

```text
Payload Task    Priority 2

NGHam Task      Priority 3

Radio Task      Priority 4
```

Among tasks that are Ready, the scheduler selects a task at the highest Ready priority.

As studied previously:

```text
Highest priority does not mean always Running.
```

A higher-priority task may be Blocked.

For example:

```text
Radio Task      Priority 4    BLOCKED

NGHam Task      Priority 3    BLOCKED

Payload Task    Priority 2    READY
```

The Payload Task can run.

The number of available priorities depends on the FreeRTOS configuration, particularly:

```c
configMAX_PRIORITIES
```

---

# 11. Parameter 6: Task Handle

The final parameter can be used to obtain a **task handle**.

Example:

```c
TaskHandle_t payloadTaskHandle;
```

Then:

```c
xTaskCreate(
    PayloadTask,
    "Payload",
    1024,
    NULL,
    2,
    &payloadTaskHandle
);
```

After successful creation, `payloadTaskHandle` identifies the created task.

Conceptually:

```text
payloadTaskHandle
        |
        v
   Payload Task
```

The handle can later be used with APIs that operate on a particular task.

For example:

```c
vTaskSuspend(payloadTaskHandle);
```

or:

```c
vTaskResume(payloadTaskHandle);
```

If I do not need the handle:

```c
NULL
```

can be passed.

Example:

```c
xTaskCreate(
    PayloadTask,
    "Payload",
    1024,
    NULL,
    2,
    NULL
);
```

---

# 12. Complete `xTaskCreate()` Example

Suppose I create the Payload Task:

```c
TaskHandle_t payloadTaskHandle;

BaseType_t result;

result = xTaskCreate(
    PayloadTask,
    "Payload",
    1024,
    NULL,
    2,
    &payloadTaskHandle
);
```

The meaning is:

```text
Task Function:
PayloadTask

Task Name:
"Payload"

Stack Depth:
1024 StackType_t units

Parameter:
NULL

Priority:
2

Task Handle:
Stored in payloadTaskHandle
```

Task creation can fail, for example because sufficient memory is not available.

Therefore, the return value should be checked:

```c
if (result == pdPASS)
{
    // Task created successfully.
}
else
{
    // Task creation failed.
}
```

For an embedded system, silently assuming that every allocation succeeds is poor practice.

---

# 13. Creating Multiple Tasks

For the internship pipeline, a simplified setup might contain:

```c
TaskHandle_t payloadTaskHandle;
TaskHandle_t nghamTaskHandle;
```

Then:

```c
xTaskCreate(
    PayloadTask,
    "Payload",
    1024,
    NULL,
    2,
    &payloadTaskHandle
);

xTaskCreate(
    NGHamTask,
    "NGHam",
    1024,
    NULL,
    3,
    &nghamTaskHandle
);
```

Conceptually:

```text
+----------------------+
|     Payload Task     |
|                      |
| Priority: 2          |
| Own Stack            |
| Own TCB              |
+----------------------+

            |

            | Packet

            v

+----------------------+
|     Packet Queue     |
+----------------------+

            |

            v

+----------------------+
|      NGHam Task      |
|                      |
| Priority: 3          |
| Own Stack            |
| Own TCB              |
+----------------------+
```

The scheduler manages the two tasks independently.

---

# 14. What Happens Internally When a Task Is Created?

When a task is created dynamically, FreeRTOS needs memory for important task-related structures.

Conceptually:

```text
xTaskCreate()
      |
      v
Allocate Task Control Block
      |
      v
Allocate Task Stack
      |
      v
Initialize task information
      |
      v
Place task into scheduler structures
```

The two important pieces are:

```text
Task Control Block
        +
Task Stack
```

The stack belongs to the task's execution.

The TCB contains information that the kernel needs to manage the task.

---

# 15. What Is a Task Control Block?

TCB stands for:

> **Task Control Block**

Every FreeRTOS task has an associated TCB.

Conceptually:

```text
+--------------------------------+
|      Task Control Block        |
+--------------------------------+
| Stack pointer                  |
| Task priority                  |
| Task name                      |
| Scheduler list information     |
| Other task-management data     |
+--------------------------------+
```

The exact internal TCB structure depends on:

- FreeRTOS version.
- Configuration options.
- Target architecture.
- Enabled kernel features.

Therefore, I should not assume that every TCB contains exactly the same fields in every project.

The main concept is:

> The TCB is the kernel's internal record for managing a task.

---

# 16. Why Does Every Task Need a TCB?

Suppose the system contains:

```text
Payload Task
NGHam Task
Radio Task
```

FreeRTOS needs to know information such as:

```text
Which task is Running?

Which tasks are Ready?

Which tasks are Blocked?

What is each task's priority?

Where is each task's stack?

Which task should run next?
```

Therefore:

```text
Payload Task
    |
    +---- Payload TCB
    |
    +---- Payload Stack


NGHam Task
    |
    +---- NGHam TCB
    |
    +---- NGHam Stack


Radio Task
    |
    +---- Radio TCB
    |
    +---- Radio Stack
```

Without per-task information, the scheduler could not independently manage each task.

---

# 17. TCB and Context Switching

Suppose the Payload Task is Running:

```text
Payload Task    RUNNING

NGHam Task      BLOCKED
```

Then a packet becomes available and the NGHam Task becomes Ready.

If the scheduler decides to switch tasks:

```text
Save Payload Task context
        |
        v
Update scheduler state
        |
        v
Select NGHam Task
        |
        v
Restore NGHam Task context
        |
        v
NGHam Task RUNNING
```

The task's stack and TCB are central to maintaining the information needed for task management and context switching.

A simplified view is:

```text
Payload Task
    |
    +---- TCB
    |       |
    |       +---- Priority
    |       +---- Stack pointer
    |       +---- Scheduler information
    |
    +---- Stack
            |
            +---- Local variables
            +---- Function calls
            +---- Saved context
```

The exact location of saved processor context depends on the specific FreeRTOS port and architecture.

---

# 18. TCB vs Stack

The TCB and stack are related but not the same thing.

| TCB | Stack |
|---|---|
| Kernel task-management structure | Per-task execution memory |
| Contains task-management information | Contains local variables and call information |
| Includes information needed by scheduler | Helps preserve task execution context |
| One per task | One per task |

Conceptually:

```text
             TASK
              |
       +------+------+
       |             |
       v             v
      TCB           Stack
       |             |
       |             +---- Local variables
       |             +---- Function calls
       |             +---- Saved context
       |
       +---- Priority
       +---- Stack pointer
       +---- Scheduler metadata
```

---

# 19. What Is a Task Handle Really?

At the application level, a task handle is used to refer to a particular task.

Example:

```c
TaskHandle_t payloadTaskHandle;
```

After:

```c
xTaskCreate(
    PayloadTask,
    "Payload",
    1024,
    NULL,
    2,
    &payloadTaskHandle
);
```

I can use:

```c
payloadTaskHandle
```

to identify that task when calling supported FreeRTOS APIs.

Conceptually:

```text
Application Code

payloadTaskHandle
        |
        v
FreeRTOS-managed Payload Task
```

I should treat the task handle as an identifier managed through the FreeRTOS API rather than manually modifying internal task structures.

---

# 20. The Task Control Block Is Internal Kernel Data

Although understanding the TCB concept is important, application code normally does not directly modify the internal TCB.

For example, I should not try to manually do something like:

```c
taskTCB.priority = 5;
```

Instead, I should use FreeRTOS APIs designed for task management.

For example:

```c
vTaskPrioritySet(
    payloadTaskHandle,
    5
);
```

This allows the kernel to maintain its internal scheduling structures correctly.

The general rule is:

> Understand the TCB conceptually, but use the official FreeRTOS APIs to manage tasks.

---

# 21. Task Creation Before Starting the Scheduler

A simplified FreeRTOS application may look like:

```c
int main(void)
{
    initializeHardware();

    xTaskCreate(
        PayloadTask,
        "Payload",
        1024,
        NULL,
        2,
        NULL
    );

    xTaskCreate(
        NGHamTask,
        "NGHam",
        1024,
        NULL,
        3,
        NULL
    );

    vTaskStartScheduler();

    while (1)
    {
    }
}
```

The flow is:

```text
Program starts
      |
      v
Initialize hardware
      |
      v
Create Payload Task
      |
      v
Create NGHam Task
      |
      v
Start FreeRTOS scheduler
      |
      v
Scheduler manages task execution
```

Once the scheduler successfully starts, normal application execution generally should not return to the code after:

```c
vTaskStartScheduler();
```

If it does return, that may indicate a startup failure, such as insufficient memory for required kernel resources, depending on the port and configuration.

---

# 22. Connected Internship Task Design

A simplified task structure for the internship pipeline could be:

```c
void PayloadTask(void *pvParameters)
{
    Packet packet;

    while (1)
    {
        readPayloadMemory();

        fragmentPayload();

        createPacket(&packet);

        xQueueSend(
            packetQueue,
            &packet,
            portMAX_DELAY
        );
    }
}
```

And:

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
            encodeNGHamFrame(&packet);
        }
    }
}
```

The task relationship is:

```text
Payload Task
    |
    | Own TCB
    | Own Stack
    |
    | Creates Packet
    v
Packet Queue
    |
    | Packet becomes available
    v
NGHam Task
    |
    | Own TCB
    | Own Stack
    |
    v
NGHam Processing
```

FreeRTOS independently manages the state and execution context of both tasks.

---

# 23. Complete Example Structure

A simplified program may eventually look like:

```c
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

typedef struct
{
    uint16_t sequenceNumber;
    uint16_t occultationID;
    uint16_t payloadSize;
    uint8_t payload[64];

} Packet;

QueueHandle_t packetQueue;

void PayloadTask(void *pvParameters)
{
    Packet packet;

    while (1)
    {
        /*
         * Read simulated payload memory.
         * Fragment data.
         * Fill packet metadata and payload.
         */

        xQueueSend(
            packetQueue,
            &packet,
            portMAX_DELAY
        );
    }
}

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
            /*
             * Convert the received packet
             * into the required NGHam-like frame.
             */
        }
    }
}

int main(void)
{
    packetQueue = xQueueCreate(
        10,
        sizeof(Packet)
    );

    if (packetQueue == NULL)
    {
        /*
         * Queue creation failed.
         * Handle the error.
         */
        while (1)
        {
        }
    }

    if (xTaskCreate(
            PayloadTask,
            "Payload",
            1024,
            NULL,
            2,
            NULL
        ) != pdPASS)
    {
        while (1)
        {
        }
    }

    if (xTaskCreate(
            NGHamTask,
            "NGHam",
            1024,
            NULL,
            3,
            NULL
        ) != pdPASS)
    {
        while (1)
        {
        }
    }

    vTaskStartScheduler();

    /*
     * Normally reached only if the scheduler
     * could not start successfully.
     */

    while (1)
    {
    }
}
```

This is still a simplified learning example.

The actual internship implementation will depend on:

- The exact MSP430 model.
- Existing TT&C firmware architecture.
- Existing FreeRTOS configuration.
- Available RAM.
- Payload memory interface.
- Required packet structure.
- Existing NGHam implementation.
- Existing queue and communication architecture.

---

# 24. Important Design Point for MSP430

The project uses an MCU from the MSP430 family.

This makes memory usage particularly important.

Every task requires:

```text
Task
 |
 +---- TCB memory
 |
 +---- Stack memory
```

Therefore, creating unnecessary tasks wastes RAM.

For example:

```text
Payload Task
NGHam Task
Radio Task
Logging Task
Status Task
Debug Task
Monitoring Task
Another Debug Task
```

should not be created without considering whether each task is actually necessary.

For the actual target, I will need to inspect:

```text
Exact MSP430 model
        |
        v
Available RAM
        |
        v
Existing task count
        |
        v
Stack size of each task
        |
        v
Queue memory usage
```

The correct task architecture must fit within the real MCU's memory limits.

---

# 25. Important Misconceptions

## Misconception 1: Any C function automatically becomes a FreeRTOS task

No.

A task function must be created and registered with the kernel using an appropriate task-creation API.

---

## Misconception 2: Every task shares one stack

No.

Each task has its own stack.

---

## Misconception 3: A task should return when finished

A FreeRTOS task should not normally return like an ordinary function. A one-time task should explicitly delete itself when appropriate.

---

## Misconception 4: A task inside `while (1)` always consumes 100% CPU

No.

The task may become Blocked inside APIs such as:

```c
xQueueReceive()
vTaskDelay()
xSemaphoreTake()
```

---

## Misconception 5: The task handle and TCB are the same concept at the application level

Not exactly.

The handle is what application code uses to refer to a task through FreeRTOS APIs. The TCB is the kernel's internal task-management structure.

---

## Misconception 6: `1024` in `xTaskCreate()` always means 1024 bytes

No.

The stack depth is expressed in units related to `StackType_t`, not universally in bytes.

---

## Misconception 7: More tasks always create a better architecture

No.

Every task consumes RAM and introduces management overhead.

Tasks should represent meaningful independent activities.

---

# 26. What I Learned From This Lesson

After completing this lesson, my understanding is:

> A FreeRTOS task is an independently scheduled unit of execution represented by a task function and managed by the kernel. Each task has its own stack and an associated Task Control Block. The TCB contains information needed by FreeRTOS to manage the task, such as scheduling-related data, priority, and stack information. Application code can use a task handle to refer to a specific task through FreeRTOS APIs.

For my internship pipeline:

```text
Payload Task
    |
    +---- Own TCB
    |
    +---- Own Stack
    |
    +---- Reads and fragments payload data
    |
    v
Packet Queue
    |
    v
NGHam Task
    |
    +---- Own TCB
    |
    +---- Own Stack
    |
    +---- Receives and processes packets
```

The scheduler can independently manage these tasks because each task has its own execution context and task-management information.

---

# 27. Key Takeaways

- A FreeRTOS task is an independently scheduled unit of execution.
- A task begins as a C function with the required task-function signature.
- Tasks commonly use an infinite loop because they remain active throughout system lifetime.
- A correctly designed task blocks when it has nothing useful to do.
- A FreeRTOS task should not normally return like an ordinary C function.
- `xTaskCreate()` dynamically creates a task.
- Each task has its own stack.
- Each task has an associated Task Control Block.
- The TCB contains internal information required by the kernel to manage the task.
- A task handle allows application code to refer to a specific task.
- Task priority influences scheduling among Ready tasks.
- The stack-depth parameter is not universally measured in bytes.
- Task creation can fail and its return value should be checked.
- Creating unnecessary tasks wastes memory, which is especially important on resource-constrained MCUs such as MSP430 devices.
- Application code should use FreeRTOS APIs instead of directly modifying internal TCB structures.

---

# 28. Next Lesson

The next lesson is:

# Lesson 5: Task Stacks and Stack Memory

I will study:

- What a stack actually is.
- Why every FreeRTOS task needs its own stack.
- What gets stored on a task's stack.
- Local variables and function calls.
- How context switching relates to task stacks.
- Stack size versus stack depth.
- Stack overflow.
- Why stack size is important on an MSP430.
- How to estimate and monitor task stack usage.
- How the Payload Task and NGHam Task use independent stacks.

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
| 4 | Tasks and Task Control Block | Completed |
| 5 | Task Stacks and Stack Memory | Next |
| 6 | Queues | Upcoming |
| 7 | Core FreeRTOS APIs | Upcoming |
| 8 | Synchronization | Upcoming |
| 9 | Memory Management and Project Structure | Upcoming |

---

*These notes document my understanding while learning FreeRTOS for an embedded systems internship involving an MSP430-based payload data-handling and communication system.*
