# 5G-NR FPGA Interleaver Accelerator using Vivado HLS

Hardware acceleration of key 5G-NR interleaving algorithms using
Vivado HLS and AXI-Stream interfaces, developed and prototyped on
a Xilinx Spartan-7 FPGA platform.

The project implements three 3GPP-defined interleaving blocks used
in different 5G-NR physical channels:

- Bit Interleaver for PDSCH
- Channel Interleaver for PUCCH
- Sub-block Interleaver for PDCCH

The implementations are optimized for FPGA execution using
pipelining, loop unrolling, hardware-aware data structures and
direct index-based bit remapping.

---

## Project Overview

5G-NR relies on interleaving to rearrange coded bits before
transmission, improving resilience against burst errors and
frequency-selective fading.

This project translates the corresponding 3GPP algorithms from
TS 38.212 into synthesizable hardware-oriented C++ using Vivado HLS.

### Implemented Blocks

| Block | 5G-NR Channel | 3GPP Reference | Interface |
|---|---|---|---|
| Bit Interleaver | PDSCH | TS 38.212 §5.4.2.2 | AXI-Stream |
| Channel Interleaver | PUCCH | TS 38.212 §5.4.1.3 | AXI-Stream |
| Sub-block Interleaver | PDCCH | TS 38.212 §5.4.1.1 | AXI-Stream |

---

## Architecture

```text
                    +----------------------+
                    |   Configuration      |
                    |      Stream          |
                    +----------+-----------+
                               |
                               v
+-------------+       +--------------------+       +-------------+
| AXI-Stream  | ----> |  Interleaver IP    | ----> | AXI-Stream  |
| Input Data  |       |                    |       | Output Data |
+-------------+       +--------------------+       +-------------+
                               |
                               v
                       Reordered 5G-NR
                           coded bits
