# ARK - Test Report

## Purpose

This document records RTK validation campaign results and links them to the
test cases defined in `RTK_Test_Plan.md`.

## Campaign Status

- Status: `Draft`
- Execution: not run by Codex in this documentation update.
- Evidence source: STM32CubeIDE test run logs or equivalent target diagnostic
  output shall be pasted or referenced here after execution.

## Campaign Identification

| Item | Value |
|---|---|
| Campaign ID | To be recorded |
| Date | To be recorded |
| Operator | To be recorded |
| Target board | To be recorded |
| MCU | To be recorded |
| Source commit | To be recorded |
| Build configuration | To be recorded |
| RTK configuration | To be recorded |
| Diagnostic console | To be recorded |
| Physical evidence files | To be recorded |

## Scheduler Results

| Test ID | Description | Evidence | Result | Notes |
|---------|-------------|----------|--------|-------|
| `RTK-TC-SCHED-001` | Priority order. | `SCHED-001` diagnostic result. | To be recorded | Automated scheduler group. |
| `RTK-TC-SCHED-002` | Same-priority task coverage. | `SCHED-002` diagnostic result. | To be recorded | Automated scheduler group. |
| `RTK-TC-SCHED-003` | Idle task runs while application tasks wait. | `SCHED-003` diagnostic result. | To be recorded | Automated scheduler group. |
| `RTK-TC-SCHED-004` | Scheduler returns after non-idle tasks terminate. | `SCHED-004` diagnostic result. | To be recorded | Checked after each scheduler cycle. |
| `RTK-TC-SCHED-005` | Waiting high-priority task preempts lower-priority work. | `SCHED-005` diagnostic result. | To be recorded | Automated scheduler group. |
| `RTK-TC-SCHED-006` | Current task termination releases heap state. | `SCHED-006` diagnostic result. | To be recorded | Automated scheduler group. |
| `RTK-TC-SCHED-007` | PendSV requested while locked is served after unlock. | `SCHED-007` diagnostic result. | To be recorded | Automated scheduler group. |
| `RTK-TC-SCHED-008` | PendSV re-pend evidence. | `SCHED-008` diagnostic result plus oscilloscope or logic-analyzer capture. | To be recorded | Operator-assisted. |
| `RTK-TC-SCHED-009` | Scheduler restart with and without repeated `SchedulerInit()`. | `SCHED-009` diagnostic result. | To be recorded | Reported after restart without repeated init. |
| `RTK-TC-SCHED-010` | `SchedulerStart()` rejected before first `SchedulerInit()`. | `SCHED-010` diagnostic result. | To be recorded | Executed before first scheduler init. |
| `RTK-TC-SCHED-011` | `Terminate()` restores scheduling after scheduler lock. | `SCHED-011` diagnostic result. | To be recorded | Automated scheduler group. |
| `RTK-TC-SCHED-012` | Medium/low priority ratio. | `SCHED-012` diagnostic result. | To be recorded | Uses `RTK_TEST_MEDIUM_FOR_LOW`. |

## Wait Results

| Test ID | Description | Evidence | Result | Notes |
|---------|-------------|----------|--------|-------|
| `RTK-TC-WAIT-001` | Time wait. | `WAIT-001` diagnostic result. | To be recorded | Automated wait group. |
| `RTK-TC-WAIT-002` | Timeout result in wait primitives. | `WAIT-002` diagnostic result. | To be recorded | Automated wait group. |
| `RTK-TC-WAIT-003` | Flag waits. | `WAIT-003` diagnostic result. | To be recorded | Automated wait group. |
| `RTK-TC-WAIT-004` | Bit waits. | `WAIT-004` diagnostic result. | To be recorded | Automated wait group. |
| `RTK-TC-WAIT-005` | Semaphore waits. | `WAIT-005` diagnostic result. | To be recorded | Automated wait group. |
| `RTK-TC-WAIT-006` | Queue waits. | `WAIT-006` diagnostic result. | To be recorded | Automated wait group. |
| `RTK-TC-WAIT-007` | Masked bit waits. | `WAIT-007` diagnostic result. | To be recorded | Automated wait group. |

## Timer Results

| Test ID | Description | Evidence | Result | Notes |
|---------|-------------|----------|--------|-------|
| `RTK-TC-TIMER-001` | Ordered timer insertion, disarm, and expiration behavior. | `RTK_RunTimerTests()` / `TIMER-001` diagnostic result. | To be recorded | Existing timer group functional test. |
| `RTK-TC-TIMER-002` | Timer queue consistency and active timer count stability across one `MainTask` test cycle. | `MainTask.cpp` captures `NumberOfActiveTimers`, runs all test groups, kills the auxiliary LED task, calls `CheckTimerStatus()`, and verifies the final active timer count matches the initial value. | To be recorded | Failure is reported as terminal `RTK_TestFatal()` rather than as a normal PASS/FAIL summary row. Absence of timer fatal error before `MainTask` exit is the campaign-level pass evidence. |

## Semaphore Results

| Test ID | Description | Evidence | Result | Notes |
|---------|-------------|----------|--------|-------|
| `RTK-TC-SEM-001` | Binary semaphore. | `SEM-001` diagnostic result. | To be recorded | Automated semaphore group. |
| `RTK-TC-SEM-002` | Counting semaphore. | `SEM-002` diagnostic result. | To be recorded | Automated semaphore group. |

## Memory Manager Results

| Test ID | Description | Evidence | Result | Notes |
|---------|-------------|----------|--------|-------|
| `RTK-TC-MM-001` | Dynamic allocation stress. | `MM-001` diagnostic result. | To be recorded | Automated memory-manager group. |
| `RTK-TC-MM-002` | Heap protection and post-cycle baseline stability. | `MM-002` diagnostic result and heap baseline messages. | To be recorded | Warnings shall be classified in the campaign notes. |

## Diagnostic And Configuration Results

| Test ID | Description | Evidence | Result | Notes |
|---------|-------------|----------|--------|-------|
| `RTK-TC-DIAG-001` | Task diagnostics. | Planned diagnostic result. | Not yet executed | Firmware table entry exists; test still to be completed. |
| `RTK-TC-DIAG-002` | Idle diagnostics. | Planned diagnostic result. | Not yet executed | Firmware table entry exists; test still to be completed. |
| `RTK-TC-DIAG-003` | Terminal failure path. | Planned diagnostic result. | Not yet executed | Firmware table entry exists; test still to be completed. |
| `RTK-TC-CFG-001` | Validated configuration capture. | Manual evidence package and planned diagnostic result. | Not yet executed | Configuration evidence is currently collected manually. |

## Interactive Console And Stress Evidence

The interactive console/stress phase is currently recorded as operator evidence,
not as a separate PASS/FAIL test ID. The phase is present in the test firmware
and is being expanded; its final procedure and acceptance criteria shall be
documented before assigning a stable traceability test ID.

| Evidence | Expected result | Result | Notes |
|----------|-----------------|--------|-------|
| Console command `M` | `Heap status is: HeapOk`. | To be recorded | Attach terminal capture or log excerpt. |
| Console command `T` | Coherent task lists by priority. | To be recorded | Attach terminal capture or log excerpt. |
| Console command `D` | `Timer status: ok.` | To be recorded | Attach terminal capture or log excerpt. |
| Console command `Q` | Console phase terminates and noise tasks are requested to stop. | To be recorded | Attach final log excerpt. |

## Final Summary

Paste or reference the final `TEST SUMMARY TABLE` and `SUMMARY PASS=<n> FAIL=<n>`
lines here.

```text
To be recorded.
```

## Open Items

- Record the target board, firmware commit, build configuration, and execution
  date for the next validation run.
- Paste the final test summary table and any fatal-error output, if present.
- Decide whether `RTK-TC-TIMER-002` shall remain a campaign-level acceptance
  guard or be converted into an explicit diagnostic summary row.
