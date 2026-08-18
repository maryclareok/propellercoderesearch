# Propeller P8X32A Single-Cog and Multicog Runtime Verification

This project compares the execution time of single-cog and multicog runtime
verification on the Parallax Propeller P8X32A microcontroller.

The application reads pressure and temperature from an MS5540C barometer,
checks the pressure measurements using R2U2 runtime verification, and stores
sensor data on a Parallax SD card.

## Project Objective

Determine whether running R2U2 on a separate Propeller cog reduces application
execution time compared with running sensor processing, R2U2 verification, and
SD-card logging sequentially on one cog.

## Hardware

- Parallax Propeller P8X32A microcontroller.
- MS5540C barometer.
- Parallax SD-card module.
- Indicator LEDs.
- Oscilloscope.

### Pin Assignments

| Pin | Function |
|-----|----------|
| P0 | Oscilloscope timing output |
| P1 | SD-card chip select |
| P2 | SD-card data input |
| P3 | SD-card data output |
| P4 | SD-card clock |
| P16 | Error indicator LED |
| P21 | R2U2 verdict LED |
| P23 | Application-running LED |

The Propeller system clock was configured at 80 MHz.

## Software

- SimpleIDE.
- Propeller GCC.
- R2U2 runtime-verification software.
- Python and Matplotlib for timing analysis and graphs.

The main application was compiled using the Compact Memory Model (CMM).

## Monitored Property

R2U2 checks whether the current pressure measurement is more than 10 greater
than the previous pressure measurement.

```text
!has_prev || pressure <= previous_pressure + 10
