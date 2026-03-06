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

## Overview

This repository provides a minimal and reproducible framework for developing and validating software for the **Power and Timing Card (PTC)**. The immediate objectives are:

1. Bring up a PTC-side service (`ptc_server`).
2. Validate basic register access (direct `peek`, followed by client → server → hardware access).
3. Establish the foundation for slow-control extensions such as I2C sensor reads, EEPROM identification, and WIB-facing monitoring once the full hardware test stand becomes available.

The emphasis of this repository is **clarity, reproducibility, and incremental bring-up**, enabling a clean path from basic connectivity checks to full slow-control functionality.

---

## Repository Contents

| Component        | Description                                                             |
| ---------------- | ----------------------------------------------------------------------- |
| `src/`           | C++ source code for the PTC service and supporting utilities            |
| `build.sh`       | Generates protobuf files and compiles the server and utilities          |
| `ptc_init.sh`    | Startup script used to launch the PTC service on the target environment |
| Python utilities | Lightweight scripts for validating connectivity and register access     |

---

## Repository Layout

```
PTC_development/
├── build.sh
├── ptc_init.sh
├── src/
│   ├── ptc.proto
│   ├── ptc.cc / ptc.h
│   ├── i2c.cc / i2c.h
│   ├── log.cc / log.h
│   ├── ptc_server.cxx
│   ├── peek.cc
│   └── (generated) ptc.pb.cc / ptc.pb.h / ptc_pb2.py
└── (optional) ptc_client.py / ptc.py / ptc_client_ping.py
```

### Notes

* Protobuf outputs are generated automatically during the build process.
* Compiled binaries such as `ptc_server` and `peek` are **build artifacts** and should not be committed to the repository.

---

## Dependencies

Typical build requirements include:

* C++ compiler 
* `protoc` (Protocol Buffers compiler)
* ZeroMQ 
* Python 3 


---

## PTC Bring-Up Quickstart (Port 3345)

This section describes the initial bring-up procedure for a **fresh PTC microSD image** and a **DUNE server machine** connected via Ethernet.

The goal is to validate the following:

1. Network connectivity to the PTC
2. Direct register access on the PTC
3. Operation of the `ptc_server` ZMQ service
4. Successful client → server → hardware register read

Example validation command:

```bash
python3 src/ptc_client.py -s <PTC_IP> peek 0x800201FC
```

A successful result should return the expected register value:

```
0xDEADBEEF
```

---

### Assumptions

* The PTC boots successfully from the microSD image.
* Login access to the PTC as `root` is available.
* The PTC and DUNE server share the same routable network.
* `ptc_server` is configured to bind to **TCP port 3345**.

---

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

---

### 2. Optional: Enable Passwordless SSH

For convenience during deployment:

```bash
ssh-copy-id root@<PTC_IP>
```

Verify access:

```bash
ssh root@<PTC_IP>
```

---

### 3. Clone the Repository

On the DUNE server:

```bash
git clone https://github.com/tamago227sk/PTC_development.git
cd PTC_development
```

Ensure required build dependencies are available before proceeding.

---

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

---

### 5. Validation on the PTC

SSH into the PTC:

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
ss -lntp | grep 3345
```

Expected output: a `LISTEN` entry for port `3345`.

#### Confirm direct register access

```bash
/usr/local/bin/peek 0x800201FC
```

Expected output:

```
0xDEADBEEF
```

---

### 6. Validation from the DUNE Server

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

---

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
1234

# ss -lntp | grep 3345
LISTEN ... 0.0.0.0:3345 ... users:("ptc_server",pid=1234)

# /usr/local/bin/peek 0x800201FC
0xDEADBEEF
```

---

## Development Workflow

### Git Usage

Recommended practices:

* Commit small, well-defined changes.
* Include commit messages describing both **the modification and validation method**.

Example tag for validated states:

```bash
git tag -a stand-ready-YYYYMMDD -m "PTC server + ping/peek validated"
git push --tags
```

### Branching Strategy

| Branch      | Purpose                           |
| ----------- | --------------------------------- |
| `main`      | Stable, runnable repository state |
| `feature/*` | Development of new capabilities   |

Example branches:

```
feature/i2c-sensors
feature/wib-i2c
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
