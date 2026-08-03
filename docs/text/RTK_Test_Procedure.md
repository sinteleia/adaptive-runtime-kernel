# ARK - Test Procedure

## Preface

When RTK is used in contexts where a guarantee of reliability is required, the
RTK test procedure should be executed on the same hardware on which RTK will be
used.

If this is not practical, the test procedure should be executed at least on a
board with an analogous microprocessor and an analogous memory configuration.
The test shall also use the same RTK configuration, including the compilation
switches, that will be used in the final firmware.

For this reason, the RTK test firmware is separated from its hardware hosting
layer. The test logic is kept independent from the board project so
that it can be ported with relative ease to different hardware platforms while
preserving the same validation intent.

## Purpose

This document defines the operational procedure for executing the RTK validation
test campaign.

The test strategy, test groups, test case identifiers, and expected result
format are defined in `RTK_Test_Plan.md`.

The results of each executed campaign shall be recorded in `RTK_Test_Report.md`
or in a campaign-specific report derived from it.

## Test Behavior

The RTK test firmware runs a predefined number of test cycles.

During each cycle, the firmware executes the automatic tests used to verify the
individual RTK functions covered by the campaign:

- scheduler behavior;
- memory-manager behavior;
- timer behavior;
- RTK wait mechanism behavior.

The tests that require operator intervention are executed at the end of the
automatic cycles.

These operator-assisted tests cover, in particular:

- concurrent rescheduling behavior;
- system behavior under stress.

For these tests, the operator shall be able to monitor selected digital outputs
from the board with suitable instrumentation, such as an oscilloscope or logic
analyzer.

## Test Usage

1. Prepare the target board, diagnostic console, and any oscilloscope or logic
   analyzer required by the campaign.

2. Start log capture before launching the firmware. The captured log shall cover
   the complete campaign, from startup messages to the final summary or fatal
   error.

3. Build and launch the test firmware from the selected IDE, debugger, or
   programming environment. If the IDE rebuilds automatically before launch, do
   not run a separate build for the same campaign.

4. Let the automatic test cycles run without resetting the target. During this
   phase, the operator shall monitor the console for `START`, `PASS`, `FAIL`,
   `SKIP`, and fatal diagnostic messages.

5. If the firmware enters an operator-assisted evidence step, follow the console
   prompt. For PendSV re-pend evidence, observe the configured diagnostic pins
   and answer `Y` only if the expected timing relationship is visible. Answer
   `N` if the evidence is missing, ambiguous, or inconsistent.

6. After the automated and operator-assisted tests, the firmware may enter the
   interactive console or stress-test phase. Use the console only as described
   by the campaign plan and record any command used as operator evidence.

7. Preserve the full log and any captured oscilloscope or logic-analyzer files.
   Reference them from the test report together with the source revision,
   configuration, target board, operator, and campaign result.

Expected result:

- every mandatory automatic test emits `PASS`;
- no mandatory test emits `FAIL`;
- no unexpected fatal error is printed;
- all required operator-assisted checks are explicitly accepted or rejected by
  the operator;
- the final report can be traced back to the complete captured log and physical
  evidence.

If any result is unclear, incomplete, or not reproducible, the campaign shall be
recorded as invalid or failed according to the test report rules.

## Supplied Reference Example

The ARK repository includes a reference RTK test implementation based on an
ST NUCLEO-F446RE target board and on STM32CubeIDE.

This reference implementation is supplied as an example of one possible project
organization. It is not the only possible structure. Other IDEs, compilers, and
board projects may be used, provided that they compile and link the same logical
components described by this procedure.

In the supplied STM32CubeIDE project, the IDE project is:

```text
firmware/boards/NUCLEO-F446-RE/BasicDemo
```

The project sees the following source directories:

```text
BasicDemo/
|-- Core/
|   |-- Inc/             board-specific headers
|   |-- Src/             board-specific source files and test board adapter
|   `-- Startup/         target startup code
|-- Drivers/             STM32 CMSIS and HAL drivers
|-- ark/                 linked RTK/ARK sources
|   |-- asm/
|   |-- cfg/             RTK compilation options
|   |-- cpp/
|   |-- inc/
|   `-- src/
|-- mylib/               linked support library sources
|   |-- asm/
|   |-- cpp/
|   |-- inc/
|   `-- src/
`-- rtk_test/            linked RTK test firmware
    |-- inc/
    `-- src/
```

`ark`, `mylib`, and `rtk_test` are linked resources in the STM32CubeIDE project.
They point respectively to:

- `firmware/ark`;
- `firmware/mylib`;
- `firmware/rtk_test`.

Generated build-output directories are not part of this reference source
structure.

## STM32F446 Reference Test Configuration

For the supplied reference project, the validated host configuration is the
`Debug` STM32CubeIDE configuration of
`firmware/boards/NUCLEO-F446-RE/BasicDemo`.

The target and build configuration are:

| Item | Reference value |
|---|---|
| Board | `NUCLEO-F446RE` |
| MCU | `STM32F446RETx` / `STM32F446xx` |
| Toolchain | STM32CubeIDE MCU ARM GCC |
| Build configuration | `Debug` |
| C debug level | `-g3` |
| C optimization | `-O3` |
| C++ debug level | `-g3` |
| C++ optimization | `-O3` |
| Linker script | `STM32F446RETX_FLASH.ld` |
| Main build symbols | `DEBUG`, `USE_HAL_DRIVER`, `STM32F446xx` |

The reference include paths shall expose the board headers, STM32 HAL/CMSIS
headers, and the ARK support modules:

- `../Core/Inc`;
- `../Drivers/STM32F4xx_HAL_Driver/Inc`;
- `../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy`;
- `../Drivers/CMSIS/Device/ST/STM32F4xx/Include`;
- `../Drivers/CMSIS/Include`;
- `../../../ark/inc`;
- `../../../ark/cfg`;
- `../../../mylib/inc`;
- `../../../rtk_test/inc`.

The diagnostic console uses `USART2` through the board `Com` driver, configured
by `RTK_TestBoardConsoleInit()` at `115200`, `8N1`, without RTS/CTS flow
control.

Physical test outputs are configured as GPIO outputs by `RTK_TestBoardInit()`:

| Logical output | Board signal |
|---|---|
| Test output 0 | `CN9_PIN1`, `PA0` |
| Test output 1 | `CN9_PIN2`, `PA1` |
| Test output 2 | `CN9_PIN3`, `PA4` |
| Test output 3 | `CN9_PIN4`, `PB0` |

The physical evidence macros used by the build define direct GPIOA BSRR writes:
`OUT_SYSTIC(VALUE)` drives `PA0` / `CN9_PIN1`, and `OUT_PENDVS(VALUE)` drives
`PA1` / `CN9_PIN2`.

The reference Debug build defines these hooks as build symbols rather than as
source-code changes in `ARK_UsrOpt.h`.

`OUT_SYSTIC(VALUE)` is expanded from C code and writes GPIOB `BSRR` at
`0x40020018`:

```text
OUT_SYSTIC(VALUE)=(*((volatile unsigned int *)0x40020018)=((((VALUE)&1U)<<0)|(((((VALUE)&1U)^1U)<<16))))
```

`OUT_PENDVS(VALUE)` is expanded inside the assembler context-switch path and
therefore must be valid assembler code. The reference definition writes GPIOB
`BSRR` using scratch registers:

```text
OUT_PENDVS(VALUE)=LDR R12,=0x40020018; LDR R2,=((((VALUE)&1)<<1)|(((((VALUE)&1)^1)<<17))); STR R2,[R12]
```

For physical evidence, connect the oscilloscope or logic analyzer common ground
to the target ground, channel 1 to `CN9_PIN1` / `PA0` for SysTick, channel 2 to
`CN9_PIN2` / `PA1` for PendSV, and, when observing asynchronous re-pend
behavior, another channel to `CN9_PIN3` / `PA4` for the TIM2 test pulse.

The operator shall capture enough samples to show interrupt entry and exit
pulses. For PendSV evidence, the observed pulse shall correspond to the
assembler hook, not to a C wrapper or HAL GPIO call.

`TIM2` is the asynchronous timer used by the operator-assisted PendSV evidence
test. The board adapter configures it with prescaler `83`, initial period
`1998`, internal clock source, and `TIM2_IRQn` priority `0`.

## STM32F446 RTK Target Adapter

The STM32F446 reference project provides the RTK target adapter in:

```text
firmware/boards/NUCLEO-F446-RE/BasicDemo/Core/Src/RTK_Interface.c
```

This file is part of the validated target-specific configuration. It overrides
`HAL_InitTick()` so the STM32 HAL startup does not configure and start SysTick
before RTK takes ownership of the system tick.

The adapter derives the RTK interrupt priorities from `__NVIC_PRIO_BITS`:

| Item | Reference behavior |
|---|---|
| `RTK_PENDSV_PRIO_LOGICAL` | Lowest logical interrupt priority available on the target. |
| `RTK_SYSTICK_PRIO_LOGICAL` | One priority level above PendSV. |
| `RTK_GetSchedulerBasepri()` | Returns the `BASEPRI` threshold used to mask PendSV and lower-priority interrupts. |
| `RTK_GetSysTicBasepri()` | Returns the `BASEPRI` threshold used to mask SysTick and lower-priority interrupts. |

`SetPriorityPENDVS()` programs `PendSV_IRQn` to the RTK PendSV priority.
`SetPrioritySysTic()` programs `SysTick_IRQn` to the RTK SysTick priority.
`ResetPriorityPENDVS()` and `ResetPrioritySysTic()` restore both exceptions to
priority `0` when the scheduler is stopped.

`AttivaIlTic()` starts SysTick with:

```c
SysTick_Config(SystemCoreClock / 1000U)
```

This makes the reference RTK system tick period 1 ms. `DisattivaIlTic()` stops
SysTick by leaving only `SysTick_CTRL_CLKSOURCE_Msk` in `SysTick->CTRL`.

The validation report shall record this adapter file revision together with the
RTK configuration, because changing these functions changes the interrupt
masking, tick period, and scheduler timing assumptions of the campaign.

## Interactive Console Evidence

After the automatic campaign and operator-assisted PendSV evidence, the
reference firmware starts the interactive console stress phase through
`RTK_TestConsolle()`. During this phase, memory-noise and timer-noise tasks run
concurrently while the operator requests diagnostic pages from the console.

The RTK validation tests were executed using PuTTY as the serial terminal. PuTTY
is not a requirement: any simple terminal emulator can be used if it can open
the configured serial port and display the console output without altering the
characters sent by the firmware.

For the reference campaign, PuTTY was used with the USART2 serial settings
`115200`, `8N1`, no RTS/CTS flow control, and the following terminal-emulation
settings:

| PuTTY terminal option | Reference setting |
|---|---|
| Auto wrap mode initially on | Enabled |
| DEC Origin Mode initially on | Disabled |
| Implicit CR in every LF | Enabled |
| Implicit LF in every CR | Disabled |
| Use background colour to erase screen | Enabled |
| Enable blinking text | Disabled |
| Answerback to `^E` | `PuTTY` |
| Local echo | Auto |
| Local line editing | Auto |

Equivalent settings may be used in another terminal emulator. The important
requirements are that carriage-return/line-feed handling keeps the log readable
and that ANSI clear-screen/home sequences emitted by the console do not corrupt
the captured evidence.

The console accepts the following commands:

| Command | Meaning | Expected evidence |
|---|---|---|
| `?` or `H` | Print the help page. | The command list is displayed. |
| `M` | Print memory-manager status. | `Heap status is: HeapOk`; free memory and block counters are printed. |
| `T` | Print scheduler task status. | Task lists are printed by priority, including labels, wait causes, run counters, and free stack. |
| `D` | Print timer queue status. | `Timer status: ok.`; when enabled, the number of running timers is printed. |
| `Q` | Terminate the interactive console phase. | The console task exits and the noise tasks are requested to stop. |

The operator shall execute at least `M`, `T`, and `D` during the stress phase
before pressing `Q`. The captured log shall include the command outputs or the
operator shall attach screenshots/terminal captures showing them.

The interactive phase is acceptable only if:

- the console remains responsive while the noise tasks are active;
- `M` reports `HeapOk`;
- `D` reports `Timer status: ok.`;
- `T` prints coherent task lists without console corruption or unexpected fatal
  diagnostics;
- no fatal error, heap integrity error, timer sequence error, or timer number
  error appears before `Q`.

If any command reports an error, produces incomplete output, or the console
stops responding, record the campaign as failed or invalid and preserve the
complete log.

## Preconditions

Before executing the RTK test procedure, verify that:

- the target board is available and identified;
- the target MCU is identified;
- the project used to host the RTK test firmware is available;
- the selected IDE or build environment can compile the project for the target
  microprocessor;
- the ARK/RTK source revision under test is known;
- the RTK configuration files and compilation switches are those intended for
  the firmware being validated;
- the diagnostic output channel is available;
- any oscilloscope or logic analyzer required for timing evidence is connected;
- the operator knows where the test log will be captured.

The selected board, MCU, RTK revision, configuration, and diagnostic backend
shall be recorded in the test report.

## Hardware Selection

The preferred validation target is the final hardware on which RTK will be used.

If the final hardware is not available, the selected validation board shall have:

- the same MCU family, or a MCU with equivalent Cortex-M architecture and
  interrupt behavior;
- a comparable clock configuration;
- a comparable RAM organization and size for the tested memory layout;
- an equivalent or more restrictive interrupt priority configuration;
- diagnostic interfaces sufficient to observe test execution;
- digital outputs that can be monitored with external instrumentation;
- a communication channel connected to a terminal for the test log.

Any difference between the validation board and the final hardware shall be
recorded in the test report.

## Firmware Configuration

Before launching the campaign, verify that the RTK configuration matches the
configuration intended for the final firmware.

At minimum, record:

- `ARK_UsrOpt.h` revision and relevant RTK options;
- scheduler parameters used by `SchedulerInit()`;
- memory-manager configuration;
- diagnostic options enabled at compile time;
- wait, timer, semaphore, queue, and task-management options;
- any debug-only option used during the campaign.

The configuration shall not be changed during a campaign unless the campaign is
aborted and restarted from the beginning.

## Test Firmware Hosting

RTK hosting and RTK compilation options are described in `RTK_TechnicalManual.md`.
The selected validation project shall apply those rules when integrating RTK on
the target microprocessor.

This document describes only the additional hosting layer required by the RTK
test firmware.

The IDE or build project used for the selected target provides the
board-specific hosting layer:

- startup code;
- linker script;
- clock configuration;
- HAL initialization;
- pin and peripheral initialization;
- firmware loading or run configuration, when required by the selected
  environment.

The RTK test firmware provides the validation logic:

- RTK test harness entry point;
- RTK initial task;
- test groups;
- result logging;
- diagnostic abstraction;
- board adapter interface.

The hardware-dependent services required by the test firmware are declared in
`firmware/rtk_test/inc/RTK_TestBoard.h`. Each board used for validation shall
provide an implementation of those prototypes.

This interface includes board initialization, console initialization and output,
test digital outputs, optional digital inputs, LED control, timestamping,
asynchronous timer control, and console input/status functions.

Board-specific code shall remain isolated in the hosting layer or in the
`RTK_TestBoard.h` implementation. The test logic shall not directly depend on
STM32 HAL headers or board pin names.

## Test Code Generation

The executable test image shall be generated by linking together all components
needed to run RTK on the selected target:

- the RTK sources;
- the RTK compilation options used for the validation campaign;
- the RTK target adapter for the selected microprocessor and board environment;
- the RTK test firmware;
- the test target adapter used by the test firmware;
- the board-specific hosting project.

The RTK compilation options are part of the validation procedure. They define
the RTK configuration being validated and shall match the configuration recorded
for the campaign.

The RTK target adapter provides the target-dependent integration required by the
kernel. The test target adapter provides the target-dependent services required
only by the test firmware, such as diagnostic output, GPIO pulses, timestamping,
or board LEDs.

The preferred generation method is to compile the RTK sources, with their
validated options, together with the RTK test firmware, the RTK target adapter,
and the test target adapter in the same build.

Using a precompiled RTK library is possible, but it is not the preferred form
for validation. If this option is used, the library shall be built from the same
RTK sources, with the same compilation options and target adapter, that are
recorded for the validation campaign. The report shall make clear which library
revision and configuration were used.

The build shall therefore provide evidence that the executed test image contains
the intended RTK configuration and that the tested RTK code is the same code
identified by the campaign source revision.

## Procedure

### 1. Identify the Campaign

Assign or record the campaign identifier.

Record:

- date and operator;
- target board;
- target MCU;
- IDE or build project name and location;
- ARK/RTK source commit;
- RTK configuration identifier;
- diagnostic backend;
- instrumentation used for timing evidence.

### 2. Inspect the Configuration

Verify that the RTK configuration and compilation switches are the intended
ones.

If the campaign is intended to validate a final firmware configuration, the test
firmware shall use the same relevant RTK switches and memory configuration.

### 3. Build and Launch

Build the test firmware with the selected IDE or build environment for the
target microprocessor.

Launch, load, or start the test using the selected run, programming, or debug
configuration.

If the selected IDE automatically rebuilds before launch, no separate
command-line build is required.

### 4. Capture the Log

Capture the complete diagnostic output from the selected backend.

The log shall include:

- campaign start;
- individual test `START` rows;
- individual test `PASS`, `FAIL`, or `SKIP` rows;
- fatal error messages, if any;
- final summary;
- configuration or commit information emitted by the test firmware.

The expected logical format is defined in `RTK_Test_Plan.md`.

### 5. Observe Timing Evidence

For tests requiring physical timing evidence, connect the oscilloscope or logic
analyzer to the configured GPIO pulse outputs before entering the evidence mode.

When required by the campaign, verify:

- SysTick timing evidence;
- PendSV timing evidence;
- PendSV re-pend behavior;
- absence of unexpected long interrupt masking during the observed window.

The captured evidence shall be referenced or attached in the test report.

### 6. Evaluate the Result

The campaign passes only if:

- every mandatory test case reports `PASS`;
- no mandatory test case reports `FAIL`;
- no unexpected terminal failure occurs;
- the final summary is present and consistent with the individual rows;
- required physical evidence has been collected and reviewed;
- the tested configuration matches the recorded configuration.

If a test fails, preserve the log and record the failure in the test report.

### 7. Record the Campaign

Update the test report with:

- campaign identification data;
- tested source revision;
- tested RTK configuration;
- hardware and hosting project data;
- diagnostic backend;
- result summary;
- failed or skipped tests;
- references to captured logs and timing evidence;
- operator notes.

## Abort Conditions

Abort the campaign and restart it from the beginning if:

- the RTK configuration is changed;
- the IDE or build project configuration is changed in a way that affects RTK;
- the target board is reset unexpectedly;
- the test execution or diagnostic capture is interrupted during a mandatory test;
- the diagnostic output is incomplete;
- the operator cannot determine whether a mandatory test passed or failed.

The aborted campaign shall be recorded as invalid or incomplete.
