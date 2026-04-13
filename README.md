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
- [Network Model](#network-model)
- [Dependencies](#dependencies)
- [Software Architecture](#software-architecture)
- [PTC Bring-Up Quickstart (Port 7820)](#ptc-bring-up-quickstart-port-7820)
  - [Assumptions](#assumptions)
  - [1. Identify the PTC IP Address](#1-identify-the-ptc-ip-address)
  - [2. SSH and login](#2-ssh-and-login)
  - [3. Clone the Repository and transfer to PTC](#3-clone-the-repository-and-transfer-to-ptc)
  - [4. Build on the PTC](#4-build-on-the-ptc)
  - [5. Start the Server on the PTC](#5-start-the-server-on-the-ptc)
  - [6. Validation of PTC client](#6-validation-of-ptc-client)
- [Example Successful Output](#example-successful-output)
- [Running ptc_server on Boot](#running-ptc_server-on-boot)
  - [1. Create the systemd Service File](#1-create-the-systemd-service-file)
  - [2. Ensure the Init Script is Executable](#2-ensure-the-init-script-is-executable)
  - [3. Enable the Service](#3-enable-the-service)
  - [4. Start the Service](#4-start-the-service)
  - [5. Verification after Reboot](#5-verification-after-reboot)
- [Roadmap](#roadmap)

---

## Overview

This repository provides a framework for developing and validating software for the **Power and Timing Card (PTC)**. <br>
The immediate objectives are:

1. Bring up a PTC-side service (`ptc_server`).
2. Validate basic register access (direct `peek`, followed by client → server → hardware access). 
3. Establish the foundation for slow-control extensions such as I2C sensor reads, EEPROM identification, and WIB-facing monitoring once the full hardware test stand becomes available.

---

## Repository Contents

| Component        | Description                                                                 |
|------------------|-----------------------------------------------------------------------------|
| `src/`           | C++ source code for the PTC service, register access, and I²C utilities    |
| `build.sh`       | Generates protobuf files and compiles the server (`ptc_server`) and tools  |
| `deploy.sh`      | ⚠️ Legacy script (builds locally → may cause arch mismatch)                |
| `ptc_init.sh`    | Startup script used to launch the PTC service on the target environment    |
| Python utilities | Lightweight scripts for validating connectivity and register access        |

> ⚠️ Note: `deploy.sh` is deprecated and no longer used.  
> All builds are performed directly on the PTC.
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
## Network Model

The PTC is operated on a **private network with no internet access**.

As a result:

- The repository must be cloned on a machine with internet access (e.g. DUNE server)
- The code must then be transferred to the PTC manually (e.g. via `scp`)
- Dependencies must already exist on the PTC or be manually installed

Typical workflow:

DUNE Server (internet)
    ↓ git clone
    ↓ scp
PTC (no internet)
    ↓ build.sh

---
## Dependencies

Typical build requirements include:

* C++ compiler 
* `protoc` (Protocol Buffers compiler)
* ZeroMQ 
* Python 3

### Dependency Setup (DUNE → PTC)
#### On DUNE server (download only)
```
git clone https://github.com/zeromq/libzmq.git
git clone https://github.com/protocolbuffers/protobuf.git

tar -czf deps.tar.gz libzmq protobuf
```
#### Copy to PTC server
```
scp deps.tar.gz root@<PTC_IP>:~
```

#### On PTC server (build dependencies)
```
tar -xzf deps.tar.gz

cd libzmq
./autogen.sh
./configure
make -j2
make install
ldconfig

cd ../protobuf
./autogen.sh
./configure
make -j2
make install
ldconfig
```

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



### 2. SSH and login 

```
ssh root@<PTC_IP>
```



### 3. Clone the Repository and transfer to PTC

On the DUNE server:

```bash
git clone https://github.com/tamago227sk/PTC_development.git
scp -r PTC_development root@<PTC_IP>:~/
cd PTC_development
```

Ensure required build dependencies are available before proceeding.



### 4. Build on the PTC

On the PTC server:

```bash
cd ~/PTC_development
chmod +x build.sh
./build.sh
```

Successful completion expect the output:

```
root@ptc:~/PTC_development# ./build.sh
--- Step 1: Generating Protobuf Files (C++ and Python) ---
--- Step 2: Compiling ptc_server ---
--- Step 3: Compiling peek utility ---
--- Step 4: Deployment ---
Build Complete. Binaries installed to /bin/
```


### 5. Start the Server on the PTC

#### Confirm `ptc_server` is running

```bash
ptc_server
```

Expected output:
```
PTC: I2C initialized successfully
PTC: Hardware registers mapped at 0x80020000
PTC Server: Started and listening on port 7820
```

#### Confirm direct register access

```bash
/usr/local/bin/peek 0x800201FC
```

Expected output:

```
0xDEADBEEF
```



### 6. Validation of PTC client

#### Confirm ping function
While keeping the ptc_server running on PTC, execute the client command from /PTC_development/src on dune server
```bash
python ptc_client.py -s 192.168.0.19 ping
```

Expected successful output:

```
PTC alive
```

#### Confirm peek function
While keeping the ptc_server running on PTC, execute the client command from /PTC_development/src on dune server

```bash
python3 src/ptc_client.py -s <PTC_IP> peek 0x800201FC
```

Expected successful output:

```
0xdeadbeef
```

#### Confirm poke function
While keeping the ptc_server running on PTC, execute the client command from /PTC_development/src on dune server

```bash
python3 src/ptc_client.py -s <PTC_IP> peek xxxxxxxxxx
```

Note: xxxxxxxxxx could be any unassigned RW registers (from #13 to #63). For #13, it'd be 0x80020034. The expected value is 0x0 if this resgiter hasn't been re-written.  

Then, execute 
```bash
python3 src/ptc_client.py -s <PTC_IP> poke xxxxxxxxxx 0xA5A5A5A5
```
Note: the value can be anything, but using 0xA5A5A5A5 as an example here. the expected output is 0xa5a5a5a5.

Fianlly, execute 
```bash
python3 src/ptc_client.py -s <PTC_IP> peek xxxxxxxxxx 
```
Confirm it reurns the value that you've typed in previously. 

Successful expected output woudl look like:

```
[skubota@dune01 src]$ python ptc_client.py -s 192.168.0.19 peek 0x80020034
0x0
[skubota@dune01 src]$ python ptc_client.py -s 192.168.0.19 poke 0x80020034 0xA5A5A5A5
0xa5a5a5a5
[skubota@dune01 src]$ python ptc_client.py -s 192.168.0.19 peek 0x80020034
0xa5a5a5a5
```

These confirms the full stack:

* Ethernet communication
* ZMQ REQ/REP transport
* Protobuf command packing
* Server dispatch logic
* Register access through `/dev/mem`



### Example Successful Output

#### PTC

```
root@ptc:~# cd PTC_development/
root@ptc:~/PTC_development# ./build.sh
--- Step 1: Generating Protobuf Files (C++ and Python) ---
--- Step 2: Compiling ptc_server ---
--- Step 3: Compiling peek utility ---
--- Step 4: Deployment ---
Build Complete. Binaries installed to /bin/
root@ptc:~/PTC_development# peek 0x800201FC    
PTC: I2C initialized successfully
PTC: Hardware registers mapped at 0x80020000
0xdeadbeef
PTC destroyed
root@ptc:~/PTC_development# ptc_server
PTC: I2C initialized successfully
PTC: Hardware registers mapped at 0x80020000
PTC Server: Started and listening on port 7820
^Z
[1]+  Stopped(SIGTSTP)        ptc_server
root@ptc:~/PTC_development# kill %1
[1]+  Terminated              ptc_server
root@ptc:~/PTC_development# jobs

```

#### DUNE Server
```
[skubota@dune01 src]$ python ptc_client.py -s 192.168.0.19 ping
PTC alive
[skubota@dune01 src]$ python ptc_client.py -s 192.168.0.19 peek 0x800201FC
0xdeadbeef
[skubota@dune01 src]$ python ptc_client.py -s 192.168.0.19 peek 0x80020034
0x0
[skubota@dune01 src]$ python ptc_client.py -s 192.168.0.19 poke 0x80020034 0xA5A5A5A5
0xa5a5a5a5
[skubota@dune01 src]$ python ptc_client.py -s 192.168.0.19 peek 0x80020034
0xa5a5a5a5
```

---

## Running ptc_server on Boot

This section describes how to configure the PTC server to automatically start at boot using a `systemd` service. This replaces older SysV-style (`/etc/rc*.d/`) initialization methods and provides a more robust and maintainable solution.

### Overview

The startup chain is structured as:

```
boot → systemd → ptc_server.service → ptc_init.sh → ptc_server
```

The `ptc_init.sh` script is responsible for performing initialization checks and launching the server.


### 1. Create the systemd Service File

Create the following file:

```
/etc/systemd/system/ptc_server.service
```

With the following contents:

```ini
[Unit]
Description=PTC Server
After=network.target

[Service]
ExecStart=/bin/sh /home/root/PTC_development/ptc_init.sh
Restart=always
User=root

[Install]
WantedBy=multi-user.target
```


### 2. Ensure the Init Script is Executable

```
chmod +x /home/root/PTC_development/ptc_init.sh
```


### 3. Enable the Service

Reload systemd and enable the service:

```
systemctl daemon-reload
systemctl enable ptc_server
```


### 4. Start the Service 

```
systemctl start ptc_server
```

Check status:

```
systemctl status ptc_server
```

A successful run should show the service as `active (running)`.


This shoulod have executed the `ptc_init.sh` script, which performs the following steps:

1. Logs startup activity:

   ```
   /var/log/ptc_startup.log
   ```

2. Performs a hardware sanity check:

   ```
   peek 0x800201FC
   ```

   Expected value: `0xDEADBEEF`

3. Starts the server and logs output:

   ```
   /bin/ptc_server >> /var/log/ptc_server.log 2>&1
   ```


### 5. Verification After Reboot

After rebooting the PTC, verify that the server is running:

Check that the port is open:

```
netstat -tuln | grep 7820
```

Check that the process is running:

```
ps aux | grep ptc_server
```

Check connectivity from a remote machine:

```
nc -vz <PTC_IP> 7820
```



### Notes

* The `Restart=always` option ensures the server is automatically restarted if it crashes.
* This configuration assumes that `ptc_server` and `peek` are installed in `/bin/`.
* Logging is handled via files in `/var/log/`, which can be inspected for debugging startup issues.

---



### Context within PTC System

The PTC operates as a slow-control interface between hardware subsystems (power regulation, timing distribution, and I2C sensor readout) and external control systems via Ethernet. :contentReference[oaicite:0]{index=0}

Ensuring `ptc_server` runs at boot guarantees that register access, monitoring, and control pathways are available immediately after system initialization.

---

## Roadmap

This repository is being developed alongside a hardware test stand consisting of:

* 1× PTC
* 1× WIB
* optional 1× FEMB
* a DUNE server on the same network

The next steps are:
* Auto-start ptc_server at boot
* Expand I2C sensor support
* Slow controls integration

