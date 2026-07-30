# Lesson 7: Core FreeRTOS APIs

# Introduction

In the previous lessons I learned:

- Tasks
- Scheduler
- Task States
- Task Control Block
- Task Stack
- Queues

This lesson focuses on the FreeRTOS APIs that are used most frequently when developing an embedded application.

These APIs allow the application to:

- Start the scheduler
- Delay tasks
- Suspend tasks
- Resume tasks
- Delete tasks
- Change priorities
- Get system tick information
- Manage task execution

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

# 1. What is a FreeRTOS API?

An API (Application Programming Interface) is simply a function provided by FreeRTOS.

Examples:

```c
xTaskCreate()

vTaskDelay()

xQueueSend()

xQueueReceive()

vTaskStartScheduler()
```

These functions allow the application to interact with the FreeRTOS kernel.

---

# 2. Starting the Scheduler

After creating all required tasks, the scheduler must be started.

API:

```c
vTaskStartScheduler();
```

Typical structure:

```c
int main(void)
{
    initializeHardware();

    xTaskCreate(...);

    xTaskCreate(...);

    vTaskStartScheduler();

    while(1)
    {

    }
}
```

Once the scheduler starts successfully:

- FreeRTOS begins scheduling tasks.
- Tasks start executing.
- Normally, execution does not return from `vTaskStartScheduler()`.

If execution returns, it usually indicates that the scheduler could not start (for example, insufficient memory).

---

# 3. Delaying a Task

API:

```c
vTaskDelay();
```

Example:

```c
vTaskDelay(pdMS_TO_TICKS(1000));
```

Meaning:

Block the current task for approximately:

```text
1000 milliseconds
```

State transition:

```text
RUNNING

↓

BLOCKED

↓

READY

↓

RUNNING
```

---

# 4. Converting Time to Ticks

FreeRTOS internally works with ticks.

Instead of writing:

```c
vTaskDelay(1000);
```

use:

```c
vTaskDelay(pdMS_TO_TICKS(1000));
```

Advantages:

- Independent of tick frequency
- More portable
- Easier to read

---

# 5. Yielding the CPU

API:

```c
taskYIELD();
```

Purpose:

Voluntarily give up the CPU.

The scheduler immediately decides which Ready task should execute next.

This is rarely needed in well-designed applications because blocking APIs naturally allow task switching.

---

# 6. Suspending a Task

API:

```c
vTaskSuspend();
```

Example:

```c
vTaskSuspend(payloadTaskHandle);
```

The specified task enters:

```text
SUSPENDED
```

A suspended task:

- Does not run.
- Does not automatically wake because time passed.
- Does not wake because queue data arrives.

It must be resumed explicitly.

---

# 7. Resuming a Task

API:

```c
vTaskResume();
```

Example:

```c
vTaskResume(payloadTaskHandle);
```

State transition:

```text
SUSPENDED

↓

READY

↓

RUNNING (when scheduled)
```

---

# 8. Deleting a Task

API:

```c
vTaskDelete();
```

Delete another task:

```c
vTaskDelete(payloadTaskHandle);
```

Delete current task:

```c
vTaskDelete(NULL);
```

The deleted task no longer participates in scheduling.

---

# 9. Changing Task Priority

API:

```c
vTaskPrioritySet();
```

Example:

```c
vTaskPrioritySet(
    payloadTaskHandle,
    4
);
```

The scheduler immediately evaluates whether another task should run.

---

# 10. Reading Task Priority

API:

```c
uxTaskPriorityGet();
```

Example:

```c
UBaseType_t priority;

priority =
uxTaskPriorityGet(
    payloadTaskHandle
);
```

Useful for:

- Debugging
- Diagnostics
- Runtime monitoring

---

# 11. Getting Tick Count

API:

```c
xTaskGetTickCount();
```

Example:

```c
TickType_t ticks;

ticks =
xTaskGetTickCount();
```

Returns:

Number of ticks since the scheduler started.

Useful for:

- Timing
- Logging
- Measuring execution time

---

# 12. Delaying Until a Fixed Time

API:

```c
vTaskDelayUntil();
```

Example:

```c
TickType_t lastWakeTime;

lastWakeTime =
xTaskGetTickCount();

while(1)
{
    performTask();

    vTaskDelayUntil(
        &lastWakeTime,
        pdMS_TO_TICKS(100)
    );
}
```

Unlike `vTaskDelay()`, this helps maintain a fixed execution period.

Useful for periodic tasks.

---

# 13. Getting Remaining Stack

API:

```c
uxTaskGetStackHighWaterMark();
```

Example:

```c
UBaseType_t remaining;

remaining =
uxTaskGetStackHighWaterMark(NULL);
```

Returns the minimum unused stack remaining.

Useful for:

- Stack tuning
- Detecting oversized stacks
- Detecting stack overflow risk

---

# 14. Getting Current Task Handle

API:

```c
xTaskGetCurrentTaskHandle();
```

Returns the handle of the currently running task.

Useful when a task needs to refer to itself.

---

# 15. Delaying vs Busy Waiting

Incorrect:

```c
while(counter < 1000000)
{
    counter++;
}
```

CPU remains busy.

Correct:

```c
vTaskDelay(
    pdMS_TO_TICKS(100)
);
```

The task blocks and allows other tasks to execute.

---

# 16. APIs Used in My Internship

Payload Task:

```text
xTaskCreate()

xQueueSend()

vTaskDelay()
```

NGHam Task:

```text
xQueueReceive()

vTaskDelay()

uxTaskGetStackHighWaterMark()
```

Main:

```text
xTaskCreate()

vTaskStartScheduler()
```

Radio Task:

```text
xQueueReceive()

vTaskDelay()
```

These APIs will likely cover most of the application.

---

# 17. Typical Application Flow

```text
main()

↓

Initialize Hardware

↓

Create Queue

↓

Create Payload Task

↓

Create NGHam Task

↓

Create Radio Task

↓

Start Scheduler

↓

Payload Task Runs

↓

Queue

↓

NGHam Task

↓

Queue

↓

Radio Task
```

---

# 18. APIs I Should Memorize

Task APIs:

```text
xTaskCreate()

vTaskDelete()

vTaskSuspend()

vTaskResume()

vTaskDelay()

vTaskDelayUntil()

taskYIELD()
```

Scheduler:

```text
vTaskStartScheduler()
```

Information:

```text
xTaskGetTickCount()

uxTaskPriorityGet()

uxTaskGetStackHighWaterMark()

xTaskGetCurrentTaskHandle()
```

These APIs appear frequently in real FreeRTOS applications.

---

# 19. Common Mistakes

Calling:

```c
vTaskDelay(1000);
```

instead of

```c
vTaskDelay(
    pdMS_TO_TICKS(1000)
);
```

Ignoring API return values.

Deleting tasks unnecessarily.

Suspending tasks when blocking is more appropriate.

Using busy waiting instead of delays.

---

# 20. What I Learned

FreeRTOS provides APIs that control task execution and scheduling.

The APIs most relevant to my internship are:

- Creating tasks
- Starting the scheduler
- Delaying tasks
- Sending queue data
- Receiving queue data
- Monitoring stack usage

These APIs form the basic programming interface used by most FreeRTOS applications.

---

# 21. Key Takeaways

- APIs are functions provided by FreeRTOS.
- `vTaskStartScheduler()` starts multitasking.
- `vTaskDelay()` blocks the current task.
- `pdMS_TO_TICKS()` converts milliseconds to ticks.
- `vTaskSuspend()` suspends a task.
- `vTaskResume()` resumes a suspended task.
- `vTaskDelete()` removes a task.
- `taskYIELD()` voluntarily gives up the CPU.
- `xTaskGetTickCount()` returns system uptime in ticks.
- `uxTaskGetStackHighWaterMark()` helps size task stacks.
- These APIs are sufficient for many FreeRTOS applications.

---

# 22. Next Lesson

# Lesson 8: FreeRTOS Synchronization

Topics:

- Why synchronization is needed
- Semaphores
- Binary Semaphores
- Counting Semaphores
- Mutexes
- Priority Inheritance
- Task Notifications
- Event Groups
- Which synchronization primitive should be used in different situations
- Applying synchronization to the Payload → Queue → NGHam pipeline

---

## Progress

| Lesson | Topic | Status |
|---------|------------------------------------|-----------|
| 1 | Introduction | ✅ |
| 2 | Scheduler | ✅ |
| 3 | Task States | ✅ |
| 4 | Tasks & TCB | ✅ |
| 5 | Stack | ✅ |
| 6 | Queues | ✅ |
| 7 | Core APIs | ✅ |
| 8 | Synchronization | Next |
| 9 | Memory Management & Project Structure | Upcoming |

---

*These notes document my understanding while learning FreeRTOS for an embedded systems internship involving an MSP430-based payload data-handling and communication system.*
