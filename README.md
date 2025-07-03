# Multicore Real-Time Safety-Critical Application Framework

## Overview

This project demonstrates a multicore real-time framework for safety-critical embedded applications, inspired by automotive and avionics domains. It implements four core processes—ABS, ECU, SENSOR, and MONITOR—each running on a dedicated CPU core and communicating via POSIX shared memory. The system showcases robust partitioning, real-time scheduling, fault detection, and graceful degradation, making it a strong foundation for research and industrial applications in safety-critical systems.

---

## Features

- **Multicore Partitioning:** Each core process (ABS, ECU, SENSOR, MONITOR) is pinned to a dedicated CPU core for spatial and temporal isolation.
- **Real-Time Scheduling:** Uses high-resolution timers and real-time Linux scheduling (SCHED_FIFO) for deterministic periodic execution.
- **Shared Memory Communication:** All processes communicate via a POSIX shared memory segment with mutex protection.
- **Fault Detection & Handling:** 
  - Heartbeat mechanism to detect monitor failures and trigger controlled halts.
  - Sensor failure detection with fallback to last known good values.
- **Graceful Degradation:** If a critical failure is detected (e.g., monitor or sensor failure), the system enters a safe halt or degraded mode to maintain safety.
- **Frequency Logging:** Each core can log its execution frequency for validation and analysis.
- **Easy Visualization:** Frequency logs can be visualized using included Python scripts (not shown here).

---

## Core Components

- **abs_core.c:** Implements anti-lock braking system logic, slip detection, and ABS activation/deactivation. Handles sensor failure and monitor heartbeat.
- **ecu_core.c:** Simulates throttle and brake control, interacts with ABS, and responds to system faults.
- **sensor_core.cpp:** Simulates wheel speed sensors, handles sensor failure, and provides last known good values when needed.
- **monitor_core.cpp:** Monitors system health, manages heartbeat, and initiates graceful halts on failure detection.
- **shared_data.c/h:** Sets up and manages shared memory, atomic variables, and mutexes for inter-process communication.

---

## Getting Started

### Prerequisites

- Linux system (preferably with PREEMPT_RT patch for best results)
- GCC and g++ compilers
- POSIX threads and real-time libraries (`-pthread`, `-lrt`)
- Python 3 (for visualization scripts)

### Build Instructions

1. **Initialize Shared Memory:**
gcc shared_data.c -o shared_data -pthread
./shared_data

text

2. **Build Each Core:**
gcc abs_core.c -o abs_core -pthread -lm -lrt
gcc ecu_core.c -o ecu_core -pthread -lm -lrt
g++ sensor_core.cpp -o sensor_core -pthread -lm -lrt
g++ monitor_core.cpp -o monitor_core -pthread -lm -lrt

text

3. **Run Each Core on a Dedicated CPU:**
sudo chrt -f 99 taskset -c 0 ./abs_core
sudo chrt -f 90 taskset -c 1 ./ecu_core
sudo chrt -f 85 taskset -c 2 ./sensor_core
sudo chrt -f 80 taskset -c 3 ./monitor_core

text

---

## How It Works

- **ABS Core:** Monitors wheel slip and modulates brake pressure. If sensor data fails, uses last valid values and disables ABS modulation.
- **ECU Core:** Adjusts throttle and brake, responds to ABS status, and simulates driver inputs.
- **Sensor Core:** Simulates wheel speed readings and handles sensor failure logic.
- **Monitor Core:** Checks for system health via heartbeats and initiates a controlled halt if a core fails.

---

## Fault Handling

- **Sensor Failure:** ABS uses last valid sensor values and disables active modulation.
- **Monitor Failure:** All cores detect the absence of a monitor heartbeat and initiate a gradual halt for safety.

---

## Visualization

- Each core can log its frequency to a CSV file (e.g., `abs_freq.csv`).
- Use a Python script to plot and analyze execution frequencies (see project scripts or ask for an example).

---

## Acknowledgments

Inspired by real-world safety-critical requirements in automotive and avionics domains, and informed by industry standards such as DO-178C and CAST-32A.

---

