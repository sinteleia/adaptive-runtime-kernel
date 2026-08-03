# ARK - Traceability Matrix

## Purpose

This document provides traceability between RTK hazards, software requirements,
and validation tests.

The matrix is intended to support bidirectional review from:

- hazard to related requirements and tests;
- requirement to related hazards and tests;
- test evidence back to the covered safety and functional concerns.

Test traceability uses the stable short test ID, such as `SCHED-001`. The full
`RTK-TC-*` form is a reporting and logging alias used when a globally qualified
identifier is useful. Analysis documents may reference a test for the specific
verification aspect relevant to the analysis, even when the test plan describes
the test with a broader or different primary objective.

## Hazard To Requirements To Tests

```{=latex}
\begin{landscape}
\scriptsize
```

| \centering\makecell{Hazard\\ID\\RTK-HAZ-} | \centering\makecell{Related\\requirements\\RTK-REQ-} | \centering\makecell{Related\\tests} | \centering\makecell{Trace\\status} | \centering\makecell{Notes} |
|------|----------|------|------|------------------------------|
| \makecell{SCHED\\001} | `SCHED-001`, `SCHED-002`, `DIAG-001` | `SCHED-001`, `SCHED-002`, planned `DIAG-001` | Draft | Scheduler selection and same-priority coverage are implemented; task diagnostic coverage remains planned. |
| \makecell{SCHED\\002} | `SCHED-002`, `MM-002` | `SCHED-012` | Draft | Medium/low inversion policy covered by scheduler ratio test. |
| \makecell{SCHED\\003} | `SCHED-002`, `DIAG-001` | `SCHED-001`, `SCHED-002`, planned `DIAG-001` | Draft | Scheduler task rotation coverage is implemented; task stack diagnostic coverage remains planned. |
| \makecell{SCHED\\004} | `SCHED-004`, `CFG-001` | `SCHED-008`, planned `CFG-001` | Draft | PendSV re-pend evidence is operator-assisted and tied to validated target configuration. |
| \makecell{SCHED\\005} | `SCHED-005`, `SCHED-006`, `MM-001`, `MM-002` | `SCHED-009`, `SCHED-010` | Draft | Scheduler restart with and without repeated initialization, heap stability, and rejected start before first initialization covered by scheduler harness. |
| \makecell{WAIT\\001} | `WAIT-002`, `SEM-001`, `SEM-002` | `WAIT-003`, `WAIT-004`, `WAIT-005`, `WAIT-006`, `WAIT-007`, `SEM-001`, `SEM-002` | Draft | Wait primitive unblock coverage is implemented for flags, bits, semaphores, queues, masked bits, and semaphore primitives. |
| \makecell{WAIT\\002} | `WAIT-002` | `WAIT-002`, `WAIT-003`, `WAIT-004`, `WAIT-005`, `WAIT-006`, `WAIT-007` | Draft | Timeout and wait-result coverage is implemented across wait primitive tests. |
| \makecell{WAIT\\003} | `WAIT-001`, `WAIT-002` | `WAIT-001`, `WAIT-002` | Draft | Time wait and timeout-result coverage are implemented. |
| \makecell{TIMER\\001} | `TIMER-001`, `CFG-001` | `WAIT-001`, `TIMER-001` | Draft | Tick progress is covered by time-wait and ordered-timer functional tests; target clock configuration evidence remains planned. |
| \makecell{TIMER\\002} | `TIMER-001`, `DIAG-002`, `CFG-001` | `WAIT-001`, `TIMER-001`, `TIMER-002` | Draft | Timer expiration behavior is covered by wait/timer tests; timer queue and active-count consistency are checked at campaign end by `MainTask`. |
| \makecell{SEM\\001} | `SEM-001`, `WAIT-002` | `SEM-001`, `WAIT-005` | Draft | Binary semaphore and semaphore-wait coverage are implemented. |
| \makecell{SEM\\002} | `SEM-002` | `SEM-002` | Draft | Counting semaphore coverage is implemented. |
| \makecell{MM\\001} | `MM-001`, `MM-002` | `MM-001`, `MM-002` | Draft | Heap stress and post-cycle heap baseline coverage are implemented. |
| \makecell{MM\\002} | `MM-001`, `CFG-001` | `MM-001`, planned `CFG-001` | Draft | Allocation behavior is implemented; heap sizing/configuration evidence remains part of validated configuration capture. |
| \makecell{DIAG\\001} | `DIAG-001`, `CFG-001` | planned `DIAG-001`, planned `CFG-001` | Draft | Diagnostic completeness coverage remains planned. |
| \makecell{CFG\\001} | `CFG-001` | planned `CFG-001` | Draft | Validated configuration capture required. |
| \makecell{CFG\\002} | `CFG-001` | planned `CFG-001` | Draft | Compile-time option and API surface checks remain part of validated configuration capture. |

```{=latex}
\normalsize
\end{landscape}
```

## Open Items

- Complete planned diagnostic and configuration test cases.
- Define the expanded interactive console/stress test procedure and decide
  whether it shall receive a stable traceability test ID.
- Add implementation references where useful.
- Add result references after the first validation campaign.
- Replace campaign-level timer consistency evidence with an explicit summary row if
  the firmware test harness is later extended to emit `TIMER-002` PASS/FAIL.
