# ARK — Adaptive Runtime Kernel

ARK is a small real-time kernel for embedded systems, designed around simplicity, predictability, transparency, and explicit developer control.

The internal kernel implementation retains the historical name RTK.

## Why ARK

ARK is aimed at embedded systems where predictable behavior, explicit control, and inspectable implementation are more important than a large middleware ecosystem.

Its design emphasizes:

* predictable, priority-based scheduling;
* static integration with the application firmware;
* explicit wait and synchronization mechanisms;
* small and inspectable kernel code;
* traceable requirements and verification documentation;
* direct control over memory, timing, and scheduling behavior.

ARK is not intended to replace feature-rich operating systems such as FreeRTOS or Zephyr in every application. It is intended for systems where kernel behavior must remain simple, understandable, and under the developer's control.

## Project status

ARK v1.0.0 is the first public release of the Adaptive Runtime Kernel.

The repository currently provides:

* the ARK/RTK kernel sources;
* memory management, scheduling, waits, semaphores, timers, and diagnostics;
* a test and demonstration firmware;
* a reference STM32CubeIDE project for the ST NUCLEO-F446RE board;
* technical, requirements, test, and traceability documentation.

The supplied board project has been built and tested with STM32CubeIDE.

## Design philosophy

ARK treats complexity as an engineering cost. Its design favors mechanisms that remain understandable, deterministic, and under the developer's explicit control.

ARK is intended for statically integrated embedded firmware. The kernel, application tasks, and RTK objects are linked into a single firmware image, rather than discovered through a runtime symbolic object registry.

Read the complete [ARK Manifesto](MANIFESTO.md).

## Supported reference platform

* Board: ST NUCLEO-F446RE
* MCU: STM32F446RE
* Architecture: Arm Cortex-M4
* Development environment: STM32CubeIDE
* Toolchain: GNU Arm Embedded Toolchain

## Getting started

The reference STM32CubeIDE project is located at:

`firmware/boards/NUCLEO-F446-RE/BasicDemo`

Follow the [Out-of-the-box guide](docs/text/outofthebox.md) to import, build, run, and test the project.

## Documentation

* [Technical manual](docs/text/RTK_Technical_Manual.md)
* [Software requirements](docs/text/RTK_Software_Requirements.md)
* [Test plan](docs/text/RTK_Test_Plan.md)
* [Test procedure](docs/text/RTK_Test_Procedure.md)
* [Test report](docs/text/RTK_Test_Report.md)
* [Traceability matrix](docs/text/RTK_Traceability_Matrix.md)

## Intended use and limitations

This release has not been certified for safety-critical, medical, automotive, aerospace, industrial protection, or other regulated applications.

Users are responsible for validating the kernel, its configuration, timing, memory use, hardware integration, and failure behavior for their intended application.

## License

ARK is licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE).

Third-party components remain subject to their respective licenses.

## Maintainer

ARK is maintained by Sintéleia S.r.l.
