ROS2 ↔ TwinCAT3 Interfaces

This repository demonstrates two different communication approaches for connecting a non-real-time ROS2 system running on Linux with a deterministic industrial PLC (TwinCAT3 on Beckhoff CX hardware).

Both approaches enable bidirectional data exchange between a ROS2 application and a hard real-time control system — but they differ fundamentally in architecture.


## EtherCAT Interface (`/EtherCAT`)

The EtherCAT implementation uses the **Beckhoff EL6695 master–master bridge** to exchange cyclic data between:

- A TwinCAT3 PLC (real-time authority)
- A Linux-based ROS2 system running the IgH EtherCAT master

EtherCAT provides inherently:

- Deterministic cyclic communication  
- Built-in working counter validation  
- Hardware watchdog mechanisms  
- Distributed clock synchronization  
- Frame integrity via CRC  

No additional application-layer safety mechanisms are required — EtherCAT natively guarantees data integrity and timing consistency.

However, this comes with higher hardware requirements:

- An EtherCAT-compatible NIC on the Linux side  
- An EtherCAT master driver (e.g. IgH / EtherLab)  
- A master–master bridge terminal (EL6695)  
- A PLC capable of running EtherCAT  


---

## UDP/IP Interface (`/UDP`)

The UDP implementation demonstrates a lightweight, vendor-agnostic communication layer between ROS2 and TwinCAT3 using real-time UDP sockets.

Unlike EtherCAT, UDP provides:

- Low latency  
- High throughput  
- Minimal protocol overhead  

But **no intrinsic safety or determinism guarantees**.

Therefore, the project implements application-layer mechanisms to compensate:

- Timestamp echoing for latency estimation  
- CRC8 checksum validation  
- Message keyword versioning  
- User-defined timeout logic  

This approach requires significantly less specialized hardware and is portable to nearly any PLC platform supporting Ethernet communication. It is particularly suitable when:

- Only two systems communicate  
- Hardware constraints limit EtherCAT usage  
