# Housekeeping / telemetry module (Task 4)

## Objective

Following completion of the encoding pipeline, project feedback specified that this module is responsible only for transmitting payload data; housekeeping (system health) data should be maintained continuously, but transmitted periodically by the TT&C module — not by this module. The suggested approach was a shared struct, similar to `radio_data_t` in the TTC 2.0 firmware (`firmware/devices/radio/radio_data.h`), where each field holds one parameter — buffer occupancy, generation rate, transmission rate, lost packets — and the TT&C module reads it whenever it assembles a housekeeping packet.

The deliverable for this task is therefore not "transmit housekeeping packets," but "maintain an accurate, thread-safe status structure that another module can read at any time."

## Design rationale: module separation

This follows the same pattern as `app/structs/payload_chunk.h` and `drivers/ngham/ngham.h`: a `.h` file declaring the interface, and a `.c` file owning the implementation and state. External modules do not need visibility into how the data is stored — every other file interacts only through `Housekeeping_Init()`, `vTask4_HousekeepingGenerator()`, or `Housekeeping_GetSnapshot()`. This separation is relevant because the eventual reader (TT&C) is a module whose internals are not controlled here — the interface constitutes the actual contract between the two.

## Folder placement
```
firmware/
└── app/
└── structs/
├── payload_chunk.h
└── housekeeping.h (+ housekeeping.c)
```
`HousekeepingData_t` is an application-level shared struct, same category as `PayloadChunk_t`, so it lives alongside it in `app/structs/` rather than under `drivers/`.

## Shared files

This task builds directly on the encoding pipeline and reuses two files unmodified:

- `drivers/ngham/ngham.h` — the NGHam packet encoder. Unmodified; `main.c` still includes it because the encoder task is unchanged.
- `app/structs/payload_chunk.h` — the `PayloadChunk_t` struct definition. Unmodified for the same reason.

Only `housekeeping.h`, `housekeeping.c`, and the updated `main.c` are new.

## Data structure design

```c
typedef struct
{
    uint8_t  raw_fifo_occupancy;
    uint8_t  ordered_fifo_occupancy;
    uint32_t chunk_generation_rate;
    uint32_t chunk_tx_rate;
    uint32_t chunk_overflow_count;
} HousekeepingData_t;
```

- Occupancy fields are `uint8_t` because both queues are created with a fixed depth of 10, so occupancy cannot exceed that value; a full byte is already sufficient. `radio_data_t` applies the same reasoning to its FIFO counters.
- Rate and overflow fields are `uint32_t`, since these can reach into the thousands over a mission's lifetime — consistent with `radio_data_t`'s packet counters.
- Units are documented per field: occupancy and overflow are plain chunk counts; generation/transmission rate are chunks/s, since Task 4 samples on a fixed 1-second period, making "per period" and "per second" numerically equivalent.

## Housekeeping data publication: length-1 mailbox queue

What's needed here is: one task publishes a value, other tasks read the latest value. FreeRTOS has a purpose-built pattern for exactly that — a queue of length 1 (a "mailbox"). `xQueueCreate(1, sizeof(HousekeepingData_t))` creates a queue that holds exactly one item, and all the locking needed to make that safe across tasks happens inside the queue's own implementation.

- `xQueueOverwrite()` replaces whatever's in the single slot instead of requiring it to be empty first — a normal `xQueueSend()` would only succeed once on a length-1 queue.
- `xQueuePeek()` copies the item out without removing it, so the same reading stays available for every future reader until Task 4 overwrites it with the next period's data — unlike `xQueueReceive()`, which would empty the mailbox after the first read.
- A 0-tick timeout on the peek — if TT&C reads before Task 4's first period completes, the mailbox is genuinely empty; returning `pdFALSE` immediately (instead of blocking) lets the caller decide to skip that field this cycle rather than stall waiting for data that isn't there yet.

## Sampling loop design

```c
vTaskDelayUntil(&xLastWakeTime, xPeriod);
```

`vTaskDelayUntil()` targets a fixed point in time rather than a delay relative to when the call occurs, which is relevant here because `chunk_generation_rate` is a delta computed against a fixed period length — if the period drifted, the rate would not represent a consistent measurement.

```c
data.chunk_generation_rate = generated - prev_generated;
```

`g_chunks_generated` is monotonically increasing, so subtracting the previous period's value from the current one gives chunks generated in the last second.

## Idle detection and sampling termination

The initial implementation printed a status line every second indefinitely, including once the mock pipeline had no further work — producing console output with no new information, since occupancy, rate, and overflow all settle to a fixed value once a run completes. The correction addresses this at the sampling level rather than the print level: two consecutive unchanged periods are treated as confirmation that the pipeline is idle, at which point the sampling loop exits entirely.

```c
if (have_sample && memcmp(&data, &prev_data, sizeof(data)) == 0)
{
    idle_periods++;
}
...
if (idle_periods >= IDLE_PERIODS_BEFORE_STOP)
{
    break;
}
```

Two consecutive unchanged periods (rather than one) are required before the pipeline is declared idle, as a margin against a brief pause between mock events being mistaken for completion. After `break`, the task enters its own idle loop — consistent with the pattern already used by the other tasks — since a FreeRTOS task function must never return.

**Design tradeoff:** this permanently retires housekeeping sampling once the mock event array is exhausted, which is correct for this fixed, finite test dataset. A flight implementation would more likely reset `idle_periods` and resume sampling upon new activity, since payload events could arrive at any time in operation.

## Dependency: Task 1 send behavior

`chunk_overflow_count` could never become non-zero as long as Task 1 sent with `portMAX_DELAY`, since that blocks forever instead of ever failing. Task 1's send is now bounded (`pdMS_TO_TICKS(50)`), and a failed send increments a lost-chunk counter and drops the chunk rather than stalling the whole pipeline waiting for space that might not come — a more realistic model of what a congested onboard buffer does on real hardware.

## Key takeaways

- A shared status structure functions well as a pull-based contract between two modules that do not otherwise need mutual awareness — the writer does not need to know whether or by whom it is being read, and the reader never blocks waiting on the writer.
- FreeRTOS's length-1 mailbox queue pattern is well suited to "always expose the latest value," removing an entire class of manual-locking errors by construction.
- Sampling and reporting are distinct responsibilities; throttling only the print statement would leave the task performing unnecessary work every period. Idle detection addresses both.

---


