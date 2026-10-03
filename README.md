# Mini-SoC Performance Model

A SystemC-based mini-SoC simulator for exploring how **memory latency, DMA behavior, accelerator parallelism, and interconnect contention affect end-to-end workload performance**.

The project models a simplified SoC containing:

```text
                    Mini SoC
┌──────────────────────────────────────────────┐
│                                              │
│   CPU / Test Driver                         │
│          │                                   │
│          ▼                                   │
│   ┌───────────────┐                          │
│   │ Interconnect  │                          │
│   └───────┬───────┘                          │
│           │                                  │
│      ┌────┴──────────────┐                   │
│      │                   │                   │
│      ▼                   ▼                   │
│  ┌────────┐          ┌─────────┐             │
│  │ Memory │          │   DMA   │             │
│  └────────┘          └────┬────┘             │
│                           │                  │
│                           ▼                  │
│                    ┌─────────────┐           │
│                    │ Convolution │           │
│                    │ Accelerator │           │
│                    └─────────────┘           │
│                                              │
└──────────────────────────────────────────────┘
```

The long-term goal is not to reproduce a production SoC cycle-for-cycle. Instead, the simulator provides a controllable environment for studying **hardware/software performance interactions and architectural tradeoffs**.

---

## Why I Built This

Modern accelerator performance depends on more than compute throughput.

An accelerator can have substantial compute capacity while still spending much of its execution time waiting for data because of:

- memory latency and bandwidth;
- DMA transfer behavior;
- interconnect congestion;
- limited bus width;
- accelerator parallelism;
- queueing and arbitration;
- communication overhead between SoC components.

This project is intended to explore those interactions quantitatively.

The central question is:

> **How do memory, data movement, interconnect behavior, and accelerator parallelism affect end-to-end convolution latency?**

The simulator is being developed incrementally so that each architectural effect can be introduced and measured independently.

---

# Current Architecture

## Memory

The memory module provides a byte-addressable main-memory abstraction used for input tensors, weights, and accelerator results.

The current model supports:

- bounded memory accesses;
- reads and writes through the system interconnect;
- configurable access timing;
- validation of invalid and out-of-range accesses.

The current timing model is intentionally simplified. It does not yet model DRAM banks, row-buffer behavior, memory controllers, cache hierarchies, or realistic bandwidth saturation.

Those effects can be introduced later as the performance model becomes more detailed.

---

## Interconnect

The interconnect provides address-based routing between initiators and memory-mapped SoC components.

Conceptually:

```text
request
   │
   ▼
Interconnect
   │
   ├── address in memory range ──────► Memory
   │
   └── address in accelerator range ─► Accelerator
```

The current implementation focuses on:

- address decoding;
- MMIO-style access;
- routing reads and writes;
- rejecting unmapped accesses;
- detecting accesses that cross mapped-region boundaries.

At this stage, the interconnect behaves primarily as a functional routing layer.

A major next step is turning it into a **performance-sensitive shared resource** with arbitration, bandwidth limits, queueing, and contention.

---

## DMA Engine

The DMA engine models data movement without requiring the CPU/test driver to manually transfer every word.

It is used to move data between:

```text
Main Memory
     │
     │ DMA
     ▼
Accelerator buffers
```

and later:

```text
Accelerator output
     │
     │ DMA
     ▼
Main Memory
```

This allows the model to separate:

- compute time;
- memory-access cost;
- transfer cost;
- accelerator execution.

Future iterations will extend the DMA timing model with parameters such as burst size and transfer bandwidth.

---

## Convolution Accelerator

The accelerator implements a simplified convolution engine with memory-mapped configuration and internal buffers.

The accelerator can be programmed through the interconnect, allowing the simulator to model a software-controlled hardware accelerator rather than directly invoking a convolution function.

The execution flow is approximately:

```text
configure accelerator
        │
        ▼
load input / weights
        │
        ▼
      START
        │
        ▼
 convolution execution
        │
        ▼
       DONE
        │
        ▼
read output
```

The model currently prioritizes correctness and architectural integration.

Later phases will expose parameters representing compute parallelism, such as the number of MAC units, so that compute throughput can be compared against memory and interconnect limitations.

---

# End-to-End SoC Flow

The current integration test exercises the complete path across the modeled components:

```text
CPU / Test Driver
        │
        │ prepare input + weights
        ▼
   Main Memory
        │
        │ DMA transfer
        ▼
   Interconnect
        │
        ▼
Accelerator Input / Weight Buffers
        │
        │ MMIO configuration
        ▼
Convolution Accelerator
        │
        │ compute
        ▼
Accelerator Output Buffer
        │
        │ DMA transfer
        ▼
   Interconnect
        │
        ▼
   Main Memory
        │
        ▼
Result Verification
```

This integration path is important because later performance experiments will measure the **entire workload**, rather than accelerator compute latency in isolation.

---

# Testing Strategy

The simulator is tested at both component and integration levels.

Current test targets include:

```text
MemoryTest
InterconnectTest
DmaTest
AcceleratorTest
SocTest
```

Run all tests with:

```bash
ctest --test-dir build --output-on-failure
```

The tests validate individual modules first and then verify that the components work together as a complete SoC data path.

The end-to-end test covers the sequence:

```text
Memory
   ↓
DMA
   ↓
Interconnect
   ↓
Accelerator
   ↓
Interconnect
   ↓
DMA
   ↓
Memory
```

and verifies the resulting convolution output.

---

# Current Project Status

### Phase 1 — Functional SoC Model

Implemented:

- [x] Main memory model
- [x] Address map
- [x] Interconnect routing
- [x] DMA engine
- [x] Memory-mapped convolution accelerator
- [x] Component-level tests
- [x] End-to-end SoC integration test
- [x] SystemC/CMake project structure

The current milestone establishes a functioning system model before introducing more complex performance behavior.

---

# Next: Performance Modeling

The next phase turns the functional simulator into an architectural performance experiment.

## 1. Configurable Simulation Parameters

The simulator will expose architectural parameters through a shared configuration object and command-line interface.

Planned parameters include:

```text
Memory latency:          20 / 50 / 100 cycles
Bus width:               32 / 64 / 128 bits
DMA burst size:          configurable
Accelerator MAC units:   configurable
```

Example:

```bash
./soc_sim \
    --memory-latency 50 \
    --bus-width 64 \
    --dma-burst 16 \
    --mac-units 8
```

This will allow experiments to run without recompiling the simulator.

---

## 2. Shared Performance Metrics

The simulator will collect metrics such as:

```text
total_cycles
memory_stall_cycles
dma_cycles
accelerator_compute_cycles
interconnect_wait_cycles
bytes_transferred
```

The goal is not only to answer:

> How long did the workload take?

but also:

> Where did the cycles go?

---

## 3. DMA Burst Modeling

The DMA model will be extended so transfer latency depends on factors such as:

```text
transfer size
      │
      ▼
number of bursts
      │
      ▼
bus transactions
      │
      ▼
transfer latency
```

This enables experiments showing when increasing DMA burst size improves performance and when another subsystem becomes the bottleneck.

---

## 4. Accelerator Parallelism

The convolution accelerator will expose configurable compute resources.

For example:

```text
MAC units = 1
MAC units = 2
MAC units = 4
MAC units = 8
MAC units = 16
```

This allows investigation of an important architectural question:

> At what point does adding compute stop improving performance because the workload becomes memory- or interconnect-bound?

---

# Next Architecture Phase: Interconnect Contention

The current interconnect performs functional address routing but does not yet represent a realistic shared communication fabric.

The next major extension will introduce a simplified shared-bus / NoC contention model.

Two traffic initiators will initially compete for the interconnect:

```text
                 ┌──────────────┐
DMA ────────────►│              │
                 │ Interconnect │────► Memory
Background ─────►│  Arbiter     │
Traffic          │              │
                 └──────────────┘
```

Planned arbitration policies:

```text
Fixed Priority
Round Robin
```

Additional parameters may include:

- packet size;
- link width;
- transfer latency;
- queue depth;
- arbitration delay;
- background traffic rate.

The simulator can then measure how competing traffic changes DMA latency and overall accelerator completion time.

This is deliberately a simplified interconnect model rather than a claim to reproduce a production Network-on-Chip implementation.

---

# Planned Experiments

Once the performance model is complete, experiments will sweep architectural parameters and compare end-to-end latency.

Example experiment matrix:

| Parameter | Values |
|---|---|
| Memory latency | 20, 50, 100 cycles |
| Bus width | 32, 64, 128 bits |
| DMA burst size | 4, 8, 16, 32 |
| MAC units | 1, 2, 4, 8, 16 |
| Background traffic | 0–high |
| Arbitration | fixed priority / round robin |

The results should reveal transitions between different bottleneck regimes.

For example:

```text
Low compute parallelism
        │
        ▼
Compute-bound
        │
        │ increase MAC units
        ▼
Memory / DMA-bound
        │
        │ add competing traffic
        ▼
Interconnect-bound
```

The important result is therefore not simply finding the configuration with the lowest latency.

The project aims to explain **why performance changes and which subsystem becomes the bottleneck.**

---

# Scope and Modeling Assumptions

This project is an educational architectural simulator, not a production RTL implementation or cycle-accurate model of a commercial SoC.

Several hardware behaviors are intentionally abstracted.

Current or planned simplifications include:

- simplified memory timing rather than a complete DRAM controller;
- no CPU cache hierarchy or cache coherence model;
- simplified DMA scheduling;
- abstract accelerator compute timing;
- simplified bus/NoC arbitration;
- no detailed physical power model;
- no RTL-level signal timing.

These simplifications keep the model understandable while still allowing architectural tradeoffs to be studied.

The model can be progressively refined as additional performance questions are introduced.

---

# Build

Requirements:

- C++17
- CMake
- SystemC

Example:

```bash
cmake -S . -B build
cmake --build build
```

Run the simulator:

```bash
./build/soc_sim
```

Run the tests:

```bash
ctest --test-dir build --output-on-failure
```

---

# Repository Structure

```text
mini-soc-model/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── memory.h
│   ├── interconnect.h
│   ├── dma.h
│   ├── accelerator.h
│   ├── address_mapping.h
│   └── common/
│
├── src/
│   ├── main.cpp
│   ├── memory.cpp
│   ├── interconnect.cpp
│   ├── dma.cpp
│   └── accelerator.cpp
│
└── tests/
    ├── test_memory.cpp
    ├── test_interconnect.cpp
    ├── test_dma.cpp
    ├── test_accelerator.cpp
    └── test_soc.cpp
```

---

# Project Roadmap

```text
Phase 1 — Functional Modeling
        │
        ├── Memory
        ├── Interconnect
        ├── DMA
        ├── Accelerator
        └── Integration tests
        │
        ▼
Phase 2 — Performance Instrumentation
        │
        ├── SimulationConfig
        ├── SimulationMetrics
        ├── configurable memory latency
        ├── DMA burst modeling
        └── accelerator parallelism
        │
        ▼
Phase 3 — Interconnect Contention
        │
        ├── multiple initiators
        ├── arbitration
        ├── queueing
        └── background traffic
        │
        ▼
Phase 4 — Architecture Experiments
        │
        ├── parameter sweeps
        ├── bottleneck analysis
        ├── latency breakdown
        └── performance plots
```

---

# What I Am Learning

This project connects concepts from several areas of computer systems:

- computer architecture;
- SoC design;
- hardware/software interfaces;
- memory-mapped I/O;
- DMA;
- accelerator architecture;
- SystemC modeling;
- interconnect arbitration;
- performance modeling;
- bottleneck analysis;
- C++ systems programming;
- integration testing.

More importantly, the project is an exercise in reasoning about the **system as a whole**.

Rather than optimizing an accelerator in isolation, the model explores how compute, memory, data movement, and communication interact to determine application-level performance.
