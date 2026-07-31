# Lesson 5: Task Stacks and Stack Memory

## Introduction

In the previous lesson, I learned that every FreeRTOS task has:

- Its own Task Control Block (TCB)
- Its own Stack

The Task Control Block stores information required by the kernel to manage the task, while the stack stores information required during the execution of the task.

This lesson focuses entirely on stack memory.

Topics covered:

- What a stack is
- Why every task needs its own stack
- Stack growth
- Function calls
- Local variables
- Context switching
- Stack size
- Stack overflow
- Stack monitoring
- Stack sizing for MSP430
- Best practices

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
Transmitter
```

---

# 1. What is a Stack?

A stack is a dedicated memory region used during program execution.

It stores temporary execution information such as:

- Function calls
- Local variables
- Return addresses
- Saved CPU registers
- Temporary compiler-generated data

Every FreeRTOS task owns its own stack.

Conceptually:

```text
Payload Task
      |
      +------ Stack

NGHam Task
      |
      +------ Stack

Radio Task
      |
      +------ Stack
```

---

# 2. Why Does Every Task Need Its Own Stack?

Suppose two tasks execute independently.

Payload Task:

```c
void PayloadTask(void *pvParameters)
{
    Packet packet;

    while(1)
    {
        createPacket(&packet);
    }
}
```

NGHam Task:

```c
void NGHamTask(void *pvParameters)
{
    NGHamFrame frame;

    while(1)
    {
        encodeFrame(&frame);
    }
}
```

Each task has different:

- Local variables
- Function calls
- Return addresses
- Execution position

If they shared one stack, their data would overwrite each other.

Therefore:

```text
Task
    |
    +---- Own Stack
```

is mandatory.

---

# 3. Stack Memory Layout

A simplified stack looks like:

```text
High Address
+-------------------+
|                   |
|                   |
|                   |
|                   |
|                   |
+-------------------+
| Return Address    |
+-------------------+
| Local Variables   |
+-------------------+
| Saved Registers   |
+-------------------+
Low Address
```

Different CPUs may grow the stack upward or downward.

For MSP430 and most FreeRTOS ports, the stack grows downward.

Conceptually:

```text
High Address

Unused

↓

Local Variables

↓

Saved Registers

↓

Current Stack Pointer

↓

Low Address
```

---

# 4. What Happens During a Function Call?

Suppose:

```c
void createPacket()
{
    uint8_t data[64];
}
```

When called,

the CPU pushes information onto the stack.

Conceptually:

```text
Before call

Stack
+-----------+
|           |
+-----------+

After call

Stack
+-----------+
| Return    |
+-----------+
| data[64]  |
+-----------+
```

When the function returns,

everything added for that function is removed.

---

# 5. Local Variables Live on the Stack

Example:

```c
void PayloadTask(void *pvParameters)
{
    Packet packet;

    uint8_t buffer[128];

    while(1)
    {
    }
}
```

Both

```text
packet

buffer
```

occupy stack memory.

Larger local arrays require more stack.

Example:

```c
uint8_t buffer[1024];
```

immediately requires approximately 1 KB of stack.

On small MCUs this is significant.

---

# 6. Nested Function Calls

Suppose:

```c
PayloadTask()

↓

fragmentPayload()

↓

createPacket()

↓

calculateCRC()
```

Every function adds more data onto the stack.

Conceptually:

```text
Stack

calculateCRC()

↓

createPacket()

↓

fragmentPayload()

↓

PayloadTask()
```

The deeper the call chain,

the more stack is required.

---

# 7. Context Switching and the Stack

Suppose

Payload Task is running.

```text
Payload Task

↓

CPU
```

A higher priority task becomes Ready.

FreeRTOS performs a context switch.

During the switch,

the CPU registers are saved onto the current task's stack.

Conceptually:

```text
Payload Stack

-----------------
Registers
Return Address
Local Variables
-----------------

↓

Scheduler switches task

↓

NGHam Stack

-----------------
Registers
Return Address
Local Variables
-----------------
```

When Payload Task runs again,

its stack is restored,

allowing execution to continue exactly where it stopped.

---

# 8. Stack Pointer

Every task has a Stack Pointer (SP).

The Stack Pointer always points to the current top of the stack.

Conceptually:

```text
High Address

Unused

↓

Local Variables

↓

Saved Registers

↓

SP

↓

Low Address
```

During execution,

the SP changes continuously.

The current Stack Pointer value is stored in the Task Control Block during context switching.

---

# 9. Stack Depth

When creating a task:

```c
xTaskCreate(
    PayloadTask,
    "Payload",
    512,
    NULL,
    2,
    NULL
);
```

The third parameter specifies stack depth.

Important:

It is **not always bytes.**

It is measured in

```text
StackType_t
```

units.

Actual memory used is:

```text
Stack Depth × sizeof(StackType_t)
```

Different architectures have different StackType_t sizes.

---

# 10. Choosing Stack Size

Too small:

```text
↓

Stack Overflow

↓

Memory Corruption

↓

Crash
```

Too large:

```text
↓

Unused RAM

↓

Memory Waste
```

Goal:

Choose a stack size that is large enough,

but not excessively large.

---

# 11. Stack Overflow

Suppose the stack allocated is

```text
512 bytes
```

But execution requires

```text
700 bytes
```

Now:

```text
Stack

Allocated Area

↓↓↓↓↓↓↓↓↓

Overflow
```

The task writes outside its stack.

Possible consequences:

- Random crashes
- Corrupted variables
- Scheduler failure
- Hard faults
- System resets

Stack overflow is one of the most common embedded bugs.

---

# 12. Detecting Stack Overflow

FreeRTOS provides stack overflow detection.

Configuration:

```c
configCHECK_FOR_STACK_OVERFLOW
```

Example:

```c
#define configCHECK_FOR_STACK_OVERFLOW 2
```

If overflow is detected,

FreeRTOS calls:

```c
vApplicationStackOverflowHook()
```

The application can then:

- Log the error
- Blink an LED
- Halt execution
- Reset the MCU

---

# 13. Monitoring Remaining Stack

FreeRTOS provides:

```c
uxTaskGetStackHighWaterMark()
```

Example:

```c
UBaseType_t remaining;

remaining =
uxTaskGetStackHighWaterMark(NULL);
```

This returns the minimum unused stack ever remaining.

Example:

Task stack:

```text
1024 words
```

High Water Mark:

```text
700 words
```

Meaning:

Maximum usage was

```text
324 words
```

This helps determine appropriate stack sizes.

---

# 14. Common Causes of Stack Overflow

Large local arrays:

```c
uint8_t image[4096];
```

Deep recursion.

Recursive example:

```c
void func()
{
    func();
}
```

Infinite recursion eventually exhausts the stack.

Very deep function calls.

Large structures allocated locally.

Large printf buffers.

---

# 15. Best Practices

Instead of:

```c
void PayloadTask()
{
    uint8_t buffer[4096];
}
```

Prefer:

Allocate large buffers statically,

globally,

or dynamically if appropriate.

Avoid recursion in embedded systems unless carefully controlled.

Keep function call depth reasonable.

Use

```c
uxTaskGetStackHighWaterMark()
```

during testing.

Enable stack overflow checking.

---

# 16. MSP430 Considerations

The MSP430 family generally has limited RAM compared to desktop systems.

Example:

```text
RAM

↓

Task Stacks

↓

Queues

↓

Global Variables

↓

Heap
```

Every additional task requires:

- Stack memory
- Task Control Block memory

Creating unnecessary tasks reduces available RAM.

For the internship project,

stack sizing will be important.

---

# 17. Applying to the Internship

Payload Task:

Needs stack for:

- Packet
- Fragmentation variables
- Function calls

NGHam Task:

Needs stack for:

- Frame
- Encoding
- CRC
- Temporary buffers

Radio Task:

Needs stack for:

- Transmission buffers
- Driver calls

Each task should have only as much stack as required.

---

# 18. Common Misconceptions

## Misconception 1

Every task shares one stack.

False.

Every task owns its own stack.

---

## Misconception 2

Large local arrays are free.

False.

They consume stack memory.

---

## Misconception 3

More stack is always better.

False.

Unused stack wastes RAM.

---

## Misconception 4

Stack depth is always bytes.

False.

It is specified in StackType_t units.

---

## Misconception 5

Stack overflow always crashes immediately.

False.

Memory corruption may appear much later, making debugging difficult.

---

# 19. What I Learned

Every FreeRTOS task owns an independent stack.

The stack stores temporary execution information required while the task runs.

During context switching,

the current task's execution context is saved using its stack,

allowing the task to continue correctly later.

Choosing the correct stack size is critical.

Too little stack causes overflow.

Too much stack wastes valuable RAM.

For an MSP430-based system,

careful stack management is essential because RAM is limited.

---

# 20. Key Takeaways

- Every task has its own stack.
- The stack stores temporary execution data.
- Local variables consume stack memory.
- Function calls increase stack usage.
- Context switching saves task state using the stack.
- Stack size is chosen during task creation.
- Stack depth is measured in StackType_t units.
- Stack overflow is a major source of embedded bugs.
- FreeRTOS provides stack overflow detection.
- High Water Mark helps estimate required stack size.
- MSP430 systems require careful stack sizing because RAM is limited.

---

# 21. Next Lesson

Lesson 6:

**FreeRTOS Queues**

Topics:

- Queue architecture
- Queue creation
- xQueueCreate()
- xQueueSend()
- xQueueReceive()
- Queue blocking
- Queue timeouts
- FIFO behavior
- Producer-Consumer design
- Applying queues to the Payload → NGHam pipeline

---

## Progress

| Lesson | Topic | Status |
|---------|-----------------------------|-----------|
| 1 | Introduction | ✅ |
| 2 | Scheduler | ✅ |
| 3 | Task States | ✅ |
| 4 | Tasks & TCB | ✅ |
| 5 | Stack | ✅ |
| 6 | Queues | Next |
| 7 | Core APIs | Upcoming |
| 8 | Synchronization | Upcoming |
| 9 | Memory Management | Upcoming |
