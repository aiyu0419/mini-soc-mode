# Mini SoC Architecture

## Objective

Model the end-to-end execution time of a convolution workload on a
simplified SoC containing a CPU driver, DMA engine, shared interconnect,
memory, and convolution accelerator.

## Research question

How do DMA burst size, memory latency, accelerator parallelism, and
interconnect contention affect end-to-end workload latency?

## Modules

### CPU Driver

Configures the DMA and accelerator through memory-mapped registers,
starts execution, and waits for a completion interrupt.

### Interconnect

Routes read and write transactions based on address ranges. The first
version uses fixed transaction latency without contention.

### Memory

Stores input activations, weights, and convolution output.

### DMA

Transfers data between main memory and accelerator-local buffers.

### Convolution Accelerator

Performs a small functional convolution and models compute latency based
on configurable MAC parallelism.

### Interrupt Controller

Receives completion signals and notifies the CPU driver.