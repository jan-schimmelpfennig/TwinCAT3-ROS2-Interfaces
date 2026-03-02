# EtherCAT ROS2 ↔ TwinCAT3 Interface

> A ROS2-TwinCAT3 interface that links non-real time user-space robotics software to a deterministic, realtime environment over EtherCAT through the Beckhoff EL6695 bridge.

![Architecture Overview](graphics/ros3tc3_ethercat_interface.png)

---

## Table of Contents

- [About](#about)
- [Why EtherCAT?](#why-ethercat)
- [Hardware & Software Setup](#hardware--software-setup)
- [Architecture](#architecture)
- [Quick Start](#quick-start)
- [How to Add Variables](#how-to-add-variables)
- [Testing](#testing)
- [Further Work](#further-work)

---

## About

This repository implements a **ROS2 ↔ TwinCAT3 communication interface over EtherCAT** using the **Beckhoff EL6695 master-to-master bridge terminal**.

Although ROS2 runs an EtherCAT master (via the IgH EtherCAT driver), it is **conceptually treated as a slave** in the overall system architecture. The deterministic, cyclic real-time control remains fully inside TwinCAT3. ROS2 operates on the non-deterministic Linux side and exchanges data with the PLC via the EL6695 bridge.

The EL6695 enables symmetric PDO mapping between two independent EtherCAT masters. On the TwinCAT side, real-time control logic executes deterministically. On the ROS2 side, variables are exposed as ROS topics and can be consumed by higher-level, event-driven applications such as perception, planning, or monitoring.

### Architectural Premise

- The PLC (TwinCAT3) is the **real-time authority**.
- ROS2 is **not used for motion control or deterministic tasks**.
- The EtherCAT determinism boundary ends at the Linux NIC.
- The EL6695 provides safe, cyclic, hardware-level synchronization between both worlds.

> **Note:** If you want to use ROS2 directly for real-time control (e.g. motion control), do not use this repository. Instead use [ethercat_driver_ros2 by ICube Lab](https://github.com/ICube-Robotics/ethercat_driver_ros2), which integrates EtherLab with `ros2_control`.

---

## Why EtherCAT?

<details>
<summary>Click to expand</summary>

EtherCAT has become the standard fieldbus in both industrial robotics and research. Rather than sending individual Ethernet frames to each device (wasteful — the minimum Ethernet frame is 84 bytes), a fieldbus passes a single frame through all devices in a ring, with each subdevice reading and writing to its assigned location in the frame on the fly.

EtherCAT has several built-in reliability mechanisms that can be used for a reliable communication:

| Mechanism | Description |
|-----------|-------------|
| **Frame Integrity / CRC** | Every EtherCAT frame includes a 32-bit CRC checksum |
| **Working Counter (WC)** | Each slave increments a counter when it processes data; any mismatch is detected immediately by the master |
| **Timeout Watchdogs** | Hardware-based watchdogs ensure a safe state if communication is interrupted |
| **Distributed Clocks (DC)** | High-precision synchronisation across the entire network |


</details>

---

## Hardware & Software Setup

<details>
<summary>Hardware</summary>

| Component | Description |
|-----------|-------------|
| **EL6695** (Beckhoff) | EtherCAT bridge clamp — enables real-time data exchange between two buses with different masters |
| **CX2043** (Beckhoff) | Industrial PC running TwinCAT4026 in cyclic deterministic real-time |
| **Precision Tower 5820** | Linux workstation (Intel Xeon W-2135 @ 3.70 GHz, NIC: Intel I219-LM) |

> ⚠️ Make sure your NIC is EtherCAT compatible. Check the [Beckhoff compatibility list](https://infosys.beckhoff.com/english.php?content=../content/1033/tcsystemmanager/9810943371.html&id=8751857768543711394).

</details>

<details>
<summary>Software</summary>

| Component | Version |
|-----------|---------|
| TwinCAT | 4026 |
| ROS2 | Jazzy Jalisco |
| Ubuntu | 24.04 |
| EtherLab IgH EtherCAT Master | 1.6 |

</details>

---

## Architecture

The implementation follows a **layered architecture**:

```
TwinCAT3 (CX2043)
    │  via EtherCAT E-bus
    ▼
EL6695 (Primary ↔ Secondary)
    │  via RJ-45 / Standard Ethernet
    ▼
NIC (Linux)
    │
    ▼
Etherlab IgH EtherCAT Driver  [Kernel Space]
    │  ecrt.h API
    ▼
bridge_node  [User Space / ROS2]
    ├── ethercatLoop()  @ 1 kHz     ←→  EtherCAT frames
    └── ROS Executor thread @ 100 Hz ←→  ROS2 topics
```

### ROS2 Node Design

The `bridge_node` is split into two threads:

**`ethercatLoop()` — 1 kHz thread**

| Step | Function | Direction | Description |
|------|----------|-----------|-------------|
| 1 | `ecrt_master_receive` | EL6695 → Master | Pull raw data from NIC hardware buffer |
| 2 | `ecrt_domain_process` | Master → Memory | Validate WKC, map data into process data pointer |
| 3 | `std::memcpy` | Application | Read inputs / write outputs |
| 4 | `ecrt_domain_queue` | Memory → Master | Queue updated data for transmission |
| 5 | `ecrt_master_send` | Master → Hardware | Send frame onto the wire |

**ROS Executor thread — 100 Hz**

Handles all ROS2 publishers and subscribers. A `std::mutex` ensures the two threads never access the shared `tx_` / `rx_` structs simultaneously.

### PDO Layout (current)

```
RxPDO (ROS2 → TwinCAT)  SM2: 0x1608
  0x7000:01  int32_t  counter
  0x7000:02  bool     flag

TxPDO (TwinCAT → ROS2)  SM3: 0x1a08
  0x6000:01  int32_t  counter
  0x6000:02  bool     flag
```

### Data Structs (`el6695_slave.h`)

```cpp
#pragma pack(push, 1)
struct ToTwinCAT {
    int32_t counter;
    uint8_t flag;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct FromTwinCAT {
    int32_t counter;
    uint8_t flag;
};
#pragma pack(pop)
```

### ROS2 Topics

| Topic | Type | Direction |
|-------|------|-----------|
| `/to_twincat_counter` | `std_msgs/Int32` | ROS2 → TwinCAT |
| `/to_twincat_bool` | `std_msgs/Bool` | ROS2 → TwinCAT |
| `/from_twincat_counter` | `std_msgs/Int32` | TwinCAT → ROS2 |
| `/from_twincat_bool` | `std_msgs/Bool` | TwinCAT → ROS2 |

---

## Quick Start

### 1. Install EtherLab IgH EtherCAT Driver

<details>
<summary>Full installation steps</summary>

**Verify Secure Boot is disabled** (EtherLab is an unsigned kernel module):

```bash
sudo apt-get install mokutil
mokutil --sb-state
# Must print: SecureBoot disabled
# If enabled, disable it in BIOS before continuing.
```

**Install dependencies:**

```bash
sudo apt-get update && sudo apt-get upgrade
sudo apt-get install git autoconf libtool pkg-config make build-essential net-tools
```

**Clone and build:**

```bash
git clone https://gitlab.com/etherlab.org/ethercat.git
cd ethercat
git checkout stable-1.6
sudo rm /usr/bin/ethercat
sudo rm /etc/init.d/ethercat
./bootstrap
./configure --prefix=/usr/local/etherlab --disable-8139too --disable-eoe --enable-generic
make all modules
sudo make modules_install install
sudo depmod
```

**Configure system:**

```bash
sudo ln -s /usr/local/etherlab/bin/ethercat /usr/bin/
sudo ln -s /usr/local/etherlab/etc/init.d/ethercat /etc/init.d/ethercat
sudo mkdir -p /etc/sysconfig
sudo cp /usr/local/etherlab/etc/sysconfig/ethercat /etc/sysconfig/ethercat
```

**Create udev rule:**

```bash
sudo gedit /etc/udev/rules.d/99-EtherCAT.rules
# Add: KERNEL=="EtherCAT[0-9]*", MODE="0666"
```

**Configure your NIC** (find MAC with `ip link`):

```bash
sudo gedit /etc/sysconfig/ethercat
# Set:
# MASTER0_DEVICE="ff:ff:ff:ff:ff:ff"   ← your NIC MAC address
# DEVICE_MODULES="generic"
```

**Start the master:**

```bash
sudo /etc/init.d/ethercat start
# Expected: Starting EtherCAT master 1.6.x  done
```

</details>

### 2. Configure TwinCAT3

Follow section **6.4.2** of the [EL6695 documentation](https://download.beckhoff.com/download/Document/io/ethercat-terminals/el6695_en.pdf) to set up symmetric PDO mapping. Make sure TwinCAT is in **Run Mode** and all I/O are linked to variables before starting the Linux side. 

To verify your PDO mapping, export the ESI configuration file from TwinCAT: go to the device → EtherCAT → *Export Configuration File*. Open it and search for `txpdo` / `rxpdo` — only entries tagged `virtual` are relevant. 
![Configuration File](graphics/Extract_ESI_File.png)

### 3. Build and Run

```bash
# Start EtherLab driver
sudo /etc/init.d/ethercat start

# In your ROS2 workspace
source /opt/ros/jazzy/setup.bash
colcon build
source install/setup.bash

# Run the bridge
ros2 run el6695_ethercat_bridge ethercat_bridge
```

<details>
<summary>Useful EtherLab diagnostic commands</summary>

```bash
# Start / stop / restart the master
sudo /etc/init.d/ethercat start
sudo /etc/init.d/ethercat stop
sudo /etc/init.d/ethercat restart

# Check slave state (OP, PreOP, etc.)
ethercat slaves -v

# Check master state
ethercat master -v

# Show configured PDOs
ethercat pdos

# Show PDO layout as a C struct
ethercat cstruct
```

</details>

---

## Testing

Open separate terminals for each command (source ROS2 and `install/setup.bash` in each):

**Send test data to TwinCAT:**

```bash
ros2 run el6695_ethercat_bridge counter_publisher   # increments int32 counter at 10 Hz
ros2 run el6695_ethercat_bridge bool_toggler        # toggles bool at 1 Hz
```

**Monitor data received from TwinCAT:**

```bash
ros2 topic echo /from_twincat_counter
ros2 topic echo /from_twincat_bool
```

---

## How to Add Variables

Follow these steps whenever you add or modify variables in the PDO mapping.

> Always start from the TwinCAT side and work towards ROS2.

**Step 1 — TwinCAT3:** Add or modify I/O at the EL6695 clamp following section 6.4.2 of the [EL6695 documentation](https://download.beckhoff.com/download/Document/io/ethercat-terminals/el6695_en.pdf). Export the ESI file and check the PDO mapping (only `virtual` entries, not `fixed`).

**Step 2 — `el6695_slave.h`:** Add your variables to the `ToTwinCAT` and `FromTwinCAT` structs:

```cpp
#pragma pack(push, 1)
struct ToTwinCAT {
    int32_t counter;
    uint8_t flag;
    // add new fields here
};
#pragma pack(pop)
```

**Step 3 — `el6695_slave.h`:** Add a private offset member for each new variable:

```cpp
unsigned int offset_tx_counter_;
unsigned int offset_rx_counter_;
unsigned int offset_tx_bool_;
unsigned int offset_rx_bool_;
// unsigned int offset_tx_my_new_var_;
```

**Step 4 — `el6695_slave.cpp` `configure()`:** Add PDO entries. The bit length and subindex come from the exported ESI file. RxPDO (to TwinCAT) goes first:

```cpp
static ec_pdo_entry_info_t pdo_entries[] = {
    {0x7000, 0x01, 32},  // int32_t to TwinCAT
    {0x7000, 0x02,  8},  // bool to TwinCAT
    // {0x7000, 0x03, 16}, // new variable to TwinCAT
    {0x6000, 0x01, 32},  // int32_t from TwinCAT
    {0x6000, 0x02,  8},  // bool from TwinCAT
    // {0x6000, 0x03, 16}, // new variable from TwinCAT
};
```

**Step 5 — `el6695_slave.cpp` `configure()`:** Update `ec_pdo_info_t` entry counts to match:

```cpp
static ec_pdo_info_t pdos[] = {
    {0x1600, 2, &pdo_entries[0]},  // RxPDO — 2 entries
    {0x1A00, 2, &pdo_entries[2]},  // TxPDO — 2 entries
};
```

**Step 6 — `el6695_slave.cpp` `configure()`:** Register the new PDO entry in the domain registration table and link to your offset variable:

```cpp
static ec_pdo_entry_reg_t domain_regs[] = {
    {0, 0, VENDOR_ID, PRODUCT_ID, 0x7000, 0x01, &offset_tx_counter_, 0},
    {0, 0, VENDOR_ID, PRODUCT_ID, 0x7000, 0x02, &offset_tx_bool_,    0},
    // {0, 0, VENDOR_ID, PRODUCT_ID, 0x7000, 0x03, &offset_tx_my_new_var_, 0},
    {0, 0, VENDOR_ID, PRODUCT_ID, 0x6000, 0x01, &offset_rx_counter_, 0},
    {0, 0, VENDOR_ID, PRODUCT_ID, 0x6000, 0x02, &offset_rx_bool_,    0},
    {0, 0, 0, 0, 0, 0, nullptr, 0}
};
```

**Step 7 — `el6695_slave.cpp` `process()`:** Add `memcpy` calls for the new variable using the correct offset:

```cpp
// Write to TwinCAT
std::memcpy(domain_pd_ + offset_tx_counter_, &tx_.counter, sizeof(int32_t));
std::memcpy(domain_pd_ + offset_tx_bool_,    &tx_.flag,    sizeof(uint8_t));

// Read from TwinCAT
std::memcpy(&rx_.counter, domain_pd_ + offset_rx_counter_, sizeof(int32_t));
std::memcpy(&rx_.flag,    domain_pd_ + offset_rx_bool_,    sizeof(uint8_t));
```

**Step 8 — `bridge_node.h` / `bridge_node.cpp`:** Add publisher, subscriber, and callback for the new variable following the existing `counter` / `bool` pattern.

---

## Package Structure

```
el6695_ethercat_bridge/
├── CMakeLists.txt
├── package.xml
├── include/
│   └── el6695_ethercat_bridge/
│       ├── bridge_node.h       # ROS2 node — publishers, subscribers, EtherCAT thread
│       └── el6695_slave.h      # EtherCAT slave abstraction, PDO structs
└── src/
    ├── main.cpp                # Entry point
    ├── bridge_node.cpp         # Node implementation
    ├── el6695_slave.cpp        # EtherCAT PDO config and cyclic process
    ├── counter_publisher.cpp   # Test node: increments int32 counter → /to_twincat_counter
    └── bool_toggler.cpp        # Test node: toggles bool → /to_twincat_bool
```

---

## Further Work

- Define proper ROS2 message types (`ToTwinCAT.msg` / `FromTwinCAT.msg`) corresponding to the C structs, enabling typed communication across the ROS2 ecosystem.
- Automate PDO mapping generation from the C struct definitions (or a YAML config), including automatic offset calculation. Maybe: Helper tooling to parse an exported TwinCAT ESI file and generate the `ec_pdo_entry_info_t` / `ec_pdo_entry_reg_t` tables automatically.
