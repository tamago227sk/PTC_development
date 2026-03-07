<h1 align="center">PTC Development</h1>

<p align="center">
Software development repository for the <strong>PTC (Power and Timing Card)</strong>
</p>

<p align="center">
<img src="https://img.shields.io/badge/build-working-brightgreen">
<img src="https://img.shields.io/badge/language-C++-blue">
<img src="https://img.shields.io/badge/python-3.x-yellow">
</p>

---

## Table of Contents

- [Overview](#overview)
- [Repository Contents](#repository-contents)
- [Repository Layout](#repository-layout)
- [Dependencies](#dependencies)
- [Software Architecture](#software-architecture)
- [PTC Bring-Up Quickstart (Port 7820)](#ptc-bring-up-quickstart-port-7820)
  - [Assumptions](#assumptions)
  - [1. Identify the PTC IP Address](#1-identify-the-ptc-ip-address)
  - [2. Optional: Enable Passwordless SSH](#2-optional-enable-passwordless-ssh)
  - [3. Clone the Repository](#3-clone-the-repository)
  - [4. Build and Deploy](#4-build-and-deploy)
  - [5. Validation on the PTC](#5-validation-on-the-ptc)
  - [6. Validation from the DUNE Server](#6-validation-from-the-dune-server)
- [Example Successful Output](#example-successful-output)
- [Development Workflow](#development-workflow)
  - [Git Usage](#git-usage)
  - [Branching Strategy](#branching-strategy)
- [Roadmap](#roadmap)

---

## Overview

This repository provides a framework for developing and validating software for the **Power and Timing Card (PTC)**. <br>
The immediate objectives are:

1. Bring up a PTC-side service (`ptc_server`).
2. Validate basic register access (direct `peek`, followed by client → server → hardware access). **←Here Now!**
3. Establish the foundation for slow-control extensions such as I2C sensor reads, EEPROM identification, and WIB-facing monitoring once the full hardware test stand becomes available.

---

## Repository Contents

| Component        | Description                                                                 |
|------------------|-----------------------------------------------------------------------------|
| `src/`           | C++ source code for the PTC service, register access, and I²C utilities    |
| `deploy.sh`      | Builds the software locally and deploys binaries to the PTC via SSH/SCP    |
| `build.sh`       | Generates protobuf files and compiles the server (`ptc_server`) and tools  |
| `ptc_init.sh`    | Startup script used to launch the PTC service on the target environment    |
| Python utilities | Lightweight scripts for validating connectivity and register access        |

---

## Repository Layout

```
PTC_development/
├── deploy.sh
├── build.sh
├── ptc_init.sh
├── src/
│   ├── ptc.proto
│   ├── ptc.cc / ptc.h
│   ├── i2c.cc / i2c.h
│   ├── log.cc / log.h
│   ├── ptc_server.cxx
│   ├── peek.cc
│   ├── ptc.py
│   ├── ptc_client.py
│   ├── ptc_client_ping.py
│   └── (generated) ptc.pb.cc / ptc.pb.h / ptc_pb2.py
```
### Notes:

- Python utilities are located inside `src/` so they can directly import the generated protobuf module `ptc_pb2.py`.
- The protobuf files (`ptc.pb.cc`, `ptc.pb.h`, `ptc_pb2.py`) are generated automatically during `build.sh`.
- Compiled binaries such as `ptc_server` and `peek` are build artifacts and should not be committed.

---

## Dependencies

Typical build requirements include:

* C++ compiler 
* `protoc` (Protocol Buffers compiler)
* ZeroMQ 
* Python 3 


---

## Software Architecture

The control stack is structured as a lightweight client–server model.

<pre>
DUNE Server
    │
    │ ZMQ (REQ/REP)
    │
    ▼
PTC Server (ptc_server)
    │
    │ register access via /dev/mem
    │
    ▼
PTC FPGA registers
</pre>

Client utilities communicate with the PTC through a ZeroMQ REQ/REP socket.
Commands are serialized using Protocol Buffers.

Main layers:

1. **Client layer**
   - `ptc_client.py`
   - sends commands (`peek`, `poke`, `ping`)

2. **Transport layer**
   - ZeroMQ REQ/REP messaging

3. **Command serialization**
   - protobuf messages (`ptc.proto`)

4. **Server dispatch**
   - `ptc_server.cxx`

5. **Hardware access**
   - `ptc.cc` → `/dev/mem` register mapping
     
---

## PTC Bring-Up Quickstart (Port 7820)

This section describes the initial bring-up procedure for a **fresh PTC microSD image** and a **DUNE server machine** connected via Ethernet.

The goal is to validate the following:

1. Network connectivity to the PTC
2. Direct register access on the PTC
3. Operation of the `ptc_server` ZMQ service
4. Successful client → server → hardware register read


### Assumptions

* The PTC boots successfully from the microSD image.
* Login access to the PTC as `root` is available.
* `ptc_server` is configured to bind to **TCP port 7820**.
* PTC address is **127.0.0.1** (set as default).
* **Those need to be updated once address is confirmed in 'ptc_client.py', 'ptc.py'**



### 1. Identify the PTC IP Address

On the PTC console:

```bash
ip a
```

From the DUNE server, verify connectivity:

```bash
ping -c 2 <PTC_IP>
```

Successful execution should return ICMP responses from the PTC.



### 2. Optional: Enable Passwordless SSH

For convenience during deployment:

```bash
ssh-copy-id root@<PTC_IP>
```

Verify access:

```bash
ssh root@<PTC_IP>
```



### 3. Clone the Repository

On the DUNE server:

```bash
git clone https://github.com/tamago227sk/PTC_development.git
cd PTC_development
```

Ensure required build dependencies are available before proceeding.



### 4. Build and Deploy

From the repository root on the DUNE server:

```bash
chmod +x deploy.sh
./deploy.sh <PTC_IP>
```

#### What `deploy.sh` performs

1. Runs `build.sh` on the development machine
2. Builds `ptc_server` and `peek`
3. Transfers binaries to the PTC using `scp`
4. Installs binaries on the PTC
5. Launches `ptc_server`

Logs are written to:

```
/var/log/ptc_server.log
```

Successful completion indicates binaries were installed and the server started.



### 5. Validation on the PTC

#### SSH into the PTC:

```bash
ssh root@<PTC_IP>
```

#### Confirm `ptc_server` is running

```bash
pidof ptc_server
```

Expected output: the server process ID.

#### Confirm server port binding

```bash
ss -lntp | grep 7820
```

Expected output: a `LISTEN` entry for port `7820`.

#### Confirm direct register access

```bash
/usr/local/bin/peek 0x800201FC
```

Expected output:

```
0xDEADBEEF
```



### 6. Validation of PTC client

Execute the client command:

```bash
python3 src/ptc_client.py -s <PTC_IP> peek 0x800201FC
```

Expected output:

```
0xdeadbeef
```

This confirms the full stack:

* Ethernet communication
* ZMQ REQ/REP transport
* Protobuf command packing
* Server dispatch logic
* Register access through `/dev/mem`



### Example Successful Output

#### DUNE Server

```
$ ./deploy.sh <PTC_IP>
=== Building on dune-server ===
... build output ...
=== Copying binaries to PTC ===
ptc_server 100%
peek       100%
=== Installing ===
Installed: /usr/local/bin/ptc_server
Installed: /usr/local/bin/peek
ptc_server log: /var/log/ptc_server.log
=== Done ===

$ python3 src/ptc_client.py -s <PTC_IP> peek 0x800201FC
0xdeadbeef
```

#### PTC

```
# pidof ptc_server
7820

# ss -lntp | grep 7820
LISTEN ... 0.0.0.0:XX ... users:("ptc_server",pid=7820)

# /usr/local/bin/peek 0x800201FC
0xDEADBEEF
```

---


### Branching Strategy

| Branch      | Purpose                           |
| ----------- | --------------------------------- |
| `main`      | Stable, runnable repository state |
| `feature/*` | Development of new capabilities   |

Probably the next branch to develop is:

```
feature/i2c-sensors
```

---

## Roadmap

This repository is being developed alongside a hardware test stand consisting of:

* 1× PTC
* 1× WIB
* optional 1× FEMB
* a DUNE server on the same network

Planned milestones:

1. Communication baseline (server launch and register read validation)
2. Local PTC monitoring via onboard I2C sensors and EEPROM
3. WIB-side monitoring through the PTC PL I2C path
4. Improved operational robustness (logging, error handling, timeouts)
