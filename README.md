# ROS2 ↔ TwinCAT3 Interfaces

## Motivation

Medical robotic systems should rely on deterministic real-time control to ensure predictable and safe interaction with hardware. At the same time, there is a growing interest in incorporating advanced perception, planning, and learning-based capabilities into robotic systems.

Many of the software libraries used for computer vision, motion planning, and machine learning are developed for **general-purpose operating systems**, typically as user-space applications. When developers aim to integrate such capabilities into robotic systems, a flexible software environment becomes advantageous.

To combine deterministic control with a non-real-time system, we propose a **layered system architecture** that separates that separates:
- Safety layer (yellow): safety-instrumented logic (e.g. SIL-rated safety PLC)
- Control layer (grey): deterministic real-time control (PLC / RTOS)
- Deliberative layer (blue): high-level computation (ROS2 on Linux)

![reference architecture](Reference_Architecture.png)

The challenge lies in designing the interface between the non-real-time and real-time systems (shaded red in the figure). Why? Because failures in the non-real-time system could affect the hardware. 

This repository demonstrates two different bi-directional communication approaches for connecting a ROS2 running on Linux with a Beckhoff PLC running TwinCAT3.
Both presented interfaces enforce a strict separation between deliberative computation and deterministic control. In both architectures, safety authority remains entirely within the real-time domain. The ROS~2 system is treated as a supervised external component whose outputs are conditionally admitted to the real-time layer.

## [EtherCAT Interface](EtherCAT/README.md)

The EtherCAT implementation uses the **Beckhoff EL6695 master–master bridge** to exchange cyclic data between:

- A TwinCAT3 PLC (real-time authority)
- A Linux-based ROS2 system running the IgH EtherCAT master

The **communication supervision is handled at the industrial fieldbus level** (EtherCAT provides built-in supervision mechanisms such as working counter validation, hardware watchdogs, distributed clock synchronization, and frame integrity checks).


However, this comes with higher hardware requirements:

- An EtherCAT-compatible NIC on the Linux side  
- An EtherCAT master driver (e.g. IgH / EtherLab)  
- A master–master bridge terminal (EL6695)  
- A PLC capable of running EtherCAT  

---

## [UDP/IP Interface](UDP/README.md)

The UDP implementation demonstrates a lightweight and vendor-independent communication interface between ROS2 and TwinCAT3 using standard Ethernet.

Unlike EtherCAT, UDP does not provide intrinsic guarantees regarding delivery, ordering, or determinism. Therefore, the interface implements **supervision mechanisms at the application layer**.

The application-layer mechanisms are:

- Timestamp echoing for latency estimation  
- CRC8 checksum validation  
- Message keyword versioning  
- User-defined timeout logic  


## Architectural Recommendation
Independent of the chosen communication protocol, we highly recommend to **maintain a single determinism boundary/interface** between the ROS ecosystem and the real-time controller.

Within the ROS environment, multiple perception nodes, workstations, or planning modules can communicate freely using DDS without real-time guarantees. 


## Which interface to choose?

Both interfaces implement the same architectural concept but represent different engineering trade-offs. The EtherCAT-based approach is particularly suitable when industrial EtherCAT infrastructure is already available. The clamp provides in addition a galvanic isolation.

The UDP-based interface, in contrast, is advantageous when the interface should run on any real-time capable platform using standard Ethernet (completely vendor-independent). 

During early system development, the UDP interface was in our project more convenient because modifying the communication interface typically only requires adjusting the shared C-style data structures on both sides. EtherCAT, on the other hand, requires explicit configuration of Process Data Objects (PDOs), which makes the initial setup slightly more involved but results in a more tightly integrated industrial solution.

# Contributing
Contact jan.schimmelpfennig@unibas.ch from the BIROMED-Lab, University of Basel if you have questions.

