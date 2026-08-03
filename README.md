# ARK — Adaptive Runtime Kernel

ARK is a small real-time kernel for embedded systems, designed around simplicity, predictability, transparency, and explicit developer control.

The internal kernel implementation retains the historical name RTK.

## Project status

This is the first public ARK release.

The repository currently provides:

- the ARK/RTK kernel sources;
- memory management, scheduling, waits, semaphores, timers, and diagnostics;
- a test and demonstration firmware;
- a reference STM32CubeIDE project for the ST NUCLEO-F446RE board;
- technical, requirements, test, and traceability documentation.

The supplied board project has been built and tested with STM32CubeIDE.

## Design philosophy

ARK treats complexity as an engineering cost. Its design favors mechanisms that remain understandable, deterministic, and under the developer's explicit
control.

Read the complete [ARK Manifesto](MANIFESTO.md).
