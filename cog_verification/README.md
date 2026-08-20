## Cog Execution Verification

The code and supporting evidence used to verify execution on separate
Propeller cogs are available in a separate repository:

[Propeller Cog Execution Verification](https://github.com/maryclareok/propellercogproof)

The repository contains:

- `provecmm.c`: Main program used to launch and verify additional cogs.
- `provecmm.map`: Linker memory map showing the compiled worker images.
- `provecmm.side`: SimpleIDE project configuration.
- `cog_state_check.txt`: Simulator output showing active and inactive cog states.

These diagnostic tests were separate from the single-cog and multicog
execution-time experiments.
