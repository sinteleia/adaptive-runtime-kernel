# Manifesto

> **Every protection adds complexity.**  
> **Every complexity adds new ways to fail.**  
> **And every failure tempts us to add more complexity.**

This project is built around a simple idea:

**complexity is a cost, not a feature.**

In concurrent systems there are no risk-free abstractions.  
Every additional mechanism — protections, hidden policies, automation layers, implicit behaviors, safety wrappers — may solve some problems while simultaneously introducing new states, new interactions, and new failure modes.

For this reason, this kernel does not pursue “absolute safety” through increasing architectural complexity.  
Instead, it prioritizes:

- simplicity;
- predictability;
- transparency;
- deterministic behavior;
- explicit control;
- full developer awareness.

The objective is to keep the kernel sufficiently small and understandable to be treated as part of the application itself rather than as a separate opaque subsystem.

This approach does not claim to eliminate risk.  
No non-trivial concurrent system can be exhaustively verified in practice: the reachable state space grows too quickly.

Instead, every mechanism is evaluated pragmatically in terms of:

- actual benefit;
- added complexity;
- verification cost;
- impact on predictability;
- runtime overhead;
- emergent behavior.

Some protections are essential.  
Some are worth their cost.  
Some merely move complexity somewhere else.

The philosophy of this project is therefore not “less safety at any cost”, but rather:

> introduce complexity only when its benefits clearly exceed the risks created by the complexity itself.

Simplicity here is not aesthetic minimalism.  
It is an engineering strategy for reliability.