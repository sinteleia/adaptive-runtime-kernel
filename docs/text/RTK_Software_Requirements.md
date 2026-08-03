# ARK - Software Requirements

## Purpose

This document collects the software requirements of the ARK kernel.

Requirements are used to:

- define the expected kernel behavior;
- trace the relationship between requirements, source code, and tests;
- support preparation of the certification documentation;
- guide implementation of the test application.

## Identification Rules

Each requirement has a stable identifier.

Format:

- `RTK-REQ-SCHED-xxx` for scheduler and task management;
- `RTK-REQ-WAIT-xxx` for wait primitives;
- `RTK-REQ-TIMER-xxx` for timers and SysTick;
- `RTK-REQ-SEM-xxx` for semaphores;
- `RTK-REQ-MM-xxx` for the memory manager;
- `RTK-REQ-DIAG-xxx` for diagnostics;
- `RTK-REQ-CFG-xxx` for configuration and compilation options.

## Requirement Status

Allowed values:

- `Draft`: requirement still to be completed;
- `Review`: requirement written but still to be reviewed;
- `Approved`: requirement approved;
- `Implemented`: requirement implemented in the source code;
- `Verified`: requirement covered by tests;
- `Deprecated`: requirement no longer applicable.

## Requirement Fields

Each requirement shall contain:

- ID;
- title;
- status;
- description;
- rationale;
- applicable configurations;
- affected source files;
- associated tests;
- notes.

## Scheduler and Task Management

### RTK-REQ-SCHED-001 - Scheduler Initialization

- Status: `Draft`
- Description: RTK shall initialize the scheduler internal structures before task execution starts.
- Rationale: without consistent initialization of task lists, idle task, and timers, runtime behavior is not deterministic.
- Applicable configurations: all.
- Affected sources: `Sched.h`, `Sched.c`, `TimerTic.c`, `Tic.c`.
- Associated tests: `RTK-TC-SCHED-010`.
- Notes: include verification of `SchedulerInit()` and of `SchedulerStart()` behavior when initialization is missing.

### RTK-REQ-SCHED-002 - Task Creation

- Status: `Draft`
- Description: RTK shall support task creation with assigned priority and dedicated stack.
- Rationale: the RTK execution model is based on independent tasks organized by priority.
- Applicable configurations: all.
- Affected sources: `Sched.h`, `Sched.c`.
- Associated tests: `RTK-TC-SCHED-001`, `RTK-TC-SCHED-002`, `RTK-TC-SCHED-005`.
- Notes: cover simple tasks, labeled tasks, parameterized tasks, and multi-parameter tasks.

### RTK-REQ-SCHED-003 - Task Termination

- Status: `Draft`
- Description: RTK shall support termination of the current task and removal of an existing task.
- Rationale: resources associated with a task shall be releasable in a controlled way. When the current task terminates, scheduling shall remain able to progress even if the task had previously masked PendSV through the scheduler lock.
- Applicable configurations: all.
- Affected sources: `Sched.c`.
- Associated tests: `RTK-TC-SCHED-006`, `RTK-TC-SCHED-011`.
- Notes: distinguish `Terminate()` from `KillTask()`. `Terminate()` shall re-enable scheduling interrupts masked through `BASEPRI` before requesting PendSV.

### RTK-REQ-SCHED-004 - Pending PendSV During PendSV Execution

- Status: `Draft`
- Description: if an interrupt requests scheduling while `PendSV_Handler` is already executing, the new PendSV request shall remain pending and shall be serviced before returning to thread execution, without waiting for the next system tick.
- Rationale: an ISR may make a higher-priority task ready while a context switch is in progress. The scheduling request must not be lost or delayed until the next tick.
- Applicable configurations: all.
- Affected sources: `Sched.h`, `Sched.c`, `SchedAsm.s`.
- Associated tests: `RTK-TC-SCHED-008`.
- Notes: test shall verify that writing `PENDSVSET` during active PendSV causes a subsequent PendSV entry immediately after the current handler exits.

### RTK-REQ-SCHED-005 - Scheduler Restart Without Memory Growth

- Status: `Draft`
- Description: RTK shall support repeated scheduler start/stop cycles without progressive heap consumption, provided that all application tasks terminate and release their resources correctly. After a successful initialization, repeated `SchedulerStart()` cycles shall be supported both with and without a repeated `SchedulerInit()` call.
- Rationale: `SchedulerStart()` is expected to return when all non-idle tasks have been destroyed. Repeated use shall not leave stale scheduler state or leak memory across cycles.
- Applicable configurations: validated scheduler and memory manager configuration.
- Affected sources: `Sched.h`, `Sched.c`, `MM.H`, `MM.c`.
- Associated tests: `RTK-TC-SCHED-009`.
- Notes: test shall run multiple task creation / `SchedulerStart()` cycles, alternating cycles that repeat `SchedulerInit()` with cycles that reuse the previous initialization, and compare heap status before and after each cycle.

### RTK-REQ-SCHED-006 - Scheduler Start Return Status

- Status: `Draft`
- Description: `SchedulerStart()` shall return `false` if the scheduler has not been initialized, and shall return `true` after a successful scheduler start followed by normal return when all non-idle tasks have been destroyed.
- Rationale: callers and validation tests need an explicit result to distinguish a rejected start request from a completed scheduler run.
- Applicable configurations: all.
- Affected sources: `Sched.h`, `Sched.c`.
- Associated tests: `RTK-TC-SCHED-004`, `RTK-TC-SCHED-010`.
- Notes: if `SchedulerStart()` rejects the call before starting, it shall not modify scheduler runtime state.

## Wait Subsystem

### RTK-REQ-WAIT-001 - Time-Based Wait

- Status: `Draft`
- Description: RTK shall allow a task to suspend until a timer expires.
- Rationale: embedded applications require repeatable time-based suspensions.
- Applicable configurations: all.
- Affected sources: `RTK_Wait.c`, `TimerTic.c`, `Sched.c`.
- Associated tests: `RTK-TC-WAIT-001`.
- Notes: include behavior when `Time == 0`.

### RTK-REQ-WAIT-002 - Timeout Result in Wait Primitives

- Status: `Draft`
- Description: wait primitives with timeout shall return an outcome that distinguishes condition satisfaction from timeout expiration.
- Rationale: the caller shall be able to determine why execution resumed.
- Applicable configurations: enabled `*TO` primitives.
- Affected sources: `RTK_Wait.c`, `Sched.c`.
- Associated tests: `RTK-TC-WAIT-002`, `RTK-TC-WAIT-003`, `RTK-TC-WAIT-004`, `RTK-TC-WAIT-005`, `RTK-TC-WAIT-006`, `RTK-TC-WAIT-007`.
- Notes: expected convention is `true` when the condition is satisfied and `false` on timeout.

## Timer

### RTK-REQ-TIMER-001 - Ordered Timer Management

- Status: `Draft`
- Description: RTK shall manage timers in an expiration-ordered list.
- Rationale: an ordered list limits periodic tick work and makes the first timer expiration check deterministic.
- Applicable configurations: all.
- Affected sources: `timer.h`, `Timer.c`, `TimerTic.h`, `TimerTic.c`.
- Associated tests: `RTK-TC-TIMER-001`, `RTK-TC-TIMER-002`.
- Notes: verify insertion, expiration, and disarming.

## Semaphores

### RTK-REQ-SEM-001 - Binary Semaphore

- Status: `Draft`
- Description: RTK shall provide a binary semaphore that can be acquired and released atomically.
- Rationale: tasks shall be able to synchronize access to shared resources.
- Applicable configurations: `WAIT_FOR_SEM` for semaphore waits.
- Affected sources: `Sem.h`, `SemAsm.s`, `RTK_Wait.c`, `Sched.c`.
- Associated tests: `RTK-TC-SEM-001`, `RTK-TC-WAIT-005`.
- Notes: distinguish atomic primitives from wait primitives.

### RTK-REQ-SEM-002 - Counting Semaphore

- Status: `Draft`
- Description: RTK shall provide a counting semaphore to represent a numeric availability of resources.
- Rationale: some resources are available as multiple equivalent instances.
- Applicable configurations: `WAIT_FOR_COUNTING_SEM`.
- Affected sources: `CountingSem.h`, `CountingSemAsm.s`, `RTK_Wait.c`, `Sched.c`.
- Associated tests: `RTK-TC-SEM-002`.
- Notes: verify decrement, increment, and task unblocking.

## Memory Manager

### RTK-REQ-MM-001 - Dynamic Allocation

- Status: `Draft`
- Description: RTK shall provide memory allocation and release primitives compatible with kernel use.
- Rationale: task creation and some subsystems require dynamic allocation.
- Applicable configurations: `MM.cfg`.
- Affected sources: `MM.H`, `MM.c`, `MM.cfg`.
- Associated tests: `RTK-TC-MM-001`.
- Notes: classify initialization-time use separately from runtime use.

### RTK-REQ-MM-002 - Heap Protection

- Status: `Draft`
- Description: heap access shall be protected according to exactly one configured strategy.
- Rationale: concurrent heap accesses may corrupt internal heap structures.
- Applicable configurations: `MALLOC_INTERRUPT_PROTECT`, `MALLOC_SCHEDULER_PROTECT`, `MALLOC_SEMAPHORE_PROTECT`.
- Affected sources: `MM.cfg`, `MM.c`.
- Associated tests: `RTK-TC-MM-002`.
- Notes: validation applies to the selected strategy.

## Diagnostics

### RTK-REQ-DIAG-001 - Task Diagnostics

- Status: `Draft`
- Description: RTK shall provide diagnostic state information for tasks.
- Rationale: runtime verification and diagnosis require observability of wait conditions and scheduling state.
- Applicable configurations: enabled diagnostic options.
- Affected sources: `TaskDiag.h`, `TaskDiag.c`, `Sched.h`.
- Associated tests: planned `RTK-TC-DIAG-001`.
- Notes: include label, waiting type, wait parameters, and idle time when available.

### RTK-REQ-DIAG-002 - Idle-Time Consistency Diagnostics

- Status: `Draft`
- Description: RTK shall support an optional idle task diagnostic routine that
  performs consistency checks on RTK runtime structures when no application task
  is ready to run.
- Rationale: some runtime corruptions, such as corrupted scheduler lists or
  inconsistent counters, may not be detected by functional tests during normal
  execution. Running diagnostic checks from the idle task allows non-urgent
  consistency verification without adding long scheduler-disabled sections to
  time-critical paths.
- Applicable configurations: diagnostic/debug configurations.
- Affected sources: `Sched.h`, `Sched.c`, `TimerTic.h`, `TimerTic.c`,
  `TaskDiag.h`, `TaskDiag.c`.
- Associated tests: planned `RTK-TC-DIAG-002`, `RTK-TC-TIMER-002`.
- Notes: the diagnostic routine should verify, as far as practical, closure and
  consistency of the per-priority circular task lists, task priority/list
  consistency, task counters maintained by creation, destruction, and priority
  change functions, timer-list consistency, and timer counters maintained by
  timer arm/disarm/expiration logic. Heap consistency may use the existing
  memory-manager diagnostics. The idle diagnostic shall avoid disabling
  scheduling for the full scan; if scheduler activity is detected during a
  scan, the diagnostic result shall be discarded and the scan restarted.

### RTK-REQ-DIAG-003 - Terminal General Failure Routine

- Status: `Draft`
- Description: RTK shall provide or support a terminal general failure routine
  that can be entered from processor fault traps, RTK fatal error paths, and
  runtime diagnostics when an unrecoverable or potentially unsafe condition is
  detected.
- Rationale: RTK cannot prevent all runtime corruptions, stack faults, hardware
  faults, or rare latent defects. When such a condition is detected, the system
  shall have a minimal and deterministic path intended to place outputs in a
  safe state and stop execution as far as possible.
- Applicable configurations: safety-related and diagnostic configurations.
- Affected sources: `Error.h`, `Error.c`, `RTK_Error.h`, `RTK_Error.c`,
  `TaskDiag.h`, `TaskDiag.c`, fault/trap integration sources, board safety
  adapter sources.
- Associated tests: planned `RTK-TC-DIAG-003`.
- Notes: the routine shall disable interrupts immediately. If it must call C or
  C++ code, it shall switch to a known reserved safe stack before doing so. It
  shall call application- or board-provided safe-output functions to place I/O
  in a safe state. It may report the failure cause using only mechanisms that do
  not depend on interrupts, scheduler services, dynamic memory, timers, or wait
  primitives. After safe-state handling and optional non-interrupt diagnostic
  signaling, it shall stop in a controlled terminal state or perform a reset
  only if the validated system policy requires it.

## Configuration

### RTK-REQ-CFG-001 - Validated Configuration

- Status: `Draft`
- Description: each validation campaign shall identify the exact RTK compilation option set used.
- Rationale: configuration changes may affect APIs, data layout, and executed code.
- Applicable configurations: all.
- Affected sources: `RTK_Config.h`, `MM.cfg`, compiler options.
- Associated tests: planned `RTK-TC-CFG-001`.
- Notes: this requirement is linked to the certification support plan.
