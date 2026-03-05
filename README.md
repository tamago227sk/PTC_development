# PTC_development

Software development repository for the **PTC (Power and Timing Card)** control, intended for bench / test-stand use with a DUNE server machine and eventually a full PTC + WIB + optional FEMB setup.

The goal of this repo is to provide a minimal, reproducible way to (1) bring up a PTC-side service, (2) validate basic communication paths, and (3) extend toward slow-control features (e.g., I2C sensor reads, EEPROM ID, and WIB-facing sensors) once hardware access is available.

---

## Contents

* `src/`
  C++ source and headers for the PTC service, plus protobuf definition.
* `build.sh`
  Generates protobuf, and builds  the server and helper utilities.
* `ptc_init.sh`
  Startup script for running the PTC service on the target environment.
* Python codes
  Minimal scripts used to validate connectivity and basic register access.
---

## Repository layout (typical)

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
│   ├── peek.cc (for initial test)
│   └── (generated) ptc.pb.cc / ptc.pb.h / ptc_pb2.py
└── (optional) ptc_client.py / ptc.py / ptc_client_ping.py
```

Notes:

* Protobuf outputs are typically generated during build (see `build.sh`).
* Compiled binaries (`ptc_server`, `peek`, etc.) are build artifacts and should not be committed.

---

## Dependencies



---

# PTC Bring-Up Quickstart (Port 3345)

This section describes the “day‑0” bring‑up flow for a **fresh PTC microSD image** (PetaLinux already built and flashed) and a **fresh DUNE server machine** connected to the PTC via Ethernet. The goal is to validate:

1. The PTC is reachable over the network.
2. The PTC-side register access works (via `peek`).
3. The PTC-side ZMQ service (`ptc_server`) is running and listening on **port 3345**.
4. The off-board client can read a known register through the server:

```bash
python3 src/ptc_client.py -s <PTC_IP> peek 0x800201FC
```

Successful execution should return the expected “ID register” value `0xDEADBEEF`.

---

## Assumptions

* The PTC boots successfully from the microSD image.
* You can log into the PTC as `root` (same operational model as WIB bring-up).
* The PTC and DUNE server are connected via Ethernet and are on a routable network.
* `ptc_server` is configured to bind to **TCP port 3345**.

---

## 1. Identify the PTC IP address

On the PTC console (serial or local shell), find the IP:

```bash
ip a
```

From the DUNE server, confirm connectivity:

```bash
ping -c 2 <PTC_IP>
```

**Expected result (success):** the PTC responds to ping.

---

## 2. (Optional) Enable passwordless SSH from the DUNE server

This makes `deploy.sh` painless:

```bash
ssh-copy-id root@<PTC_IP>
```

Then verify:

```bash
ssh root@<PTC_IP>
```

---

## 3. Clone the PTC repository on the DUNE server

On the DUNE server:

```bash
git clone <YOUR_REPO_URL>
cd PTC_development
```

Ensure the machine has the build dependencies required by `build.sh` (compiler toolchain, protobuf compiler, ZeroMQ/protobuf development libraries, and Python tooling used by the client).

---

## 4. Build + deploy to the PTC

From the repo root on the DUNE server:

```bash
chmod +x deploy.sh
./deploy.sh <PTC_IP>
```

**What `deploy.sh` does:**

* runs `./build.sh` on the DUNE server (builds `ptc_server` and `peek`)
* copies `ptc_server` and `peek` to the PTC via `scp`
* installs them on the PTC (default install location in the script)
* starts `ptc_server` on the PTC and writes logs to `/var/log/ptc_server.log`

**Expected result (success):** `deploy.sh` completes and prints the “Installed …” message.

---

## 5. Validate on the PTC (local sanity checks)

SSH into the PTC:

```bash
ssh root@<PTC_IP>
```

### 5.1 Confirm `ptc_server` is running

```bash
pidof ptc_server
```

**Expected result (success):** a PID is printed.

### 5.2 Confirm `ptc_server` is listening on port 3345

```bash
ss -lntp | grep 3345
```

**Expected result (success):** a `LISTEN` line showing `:3345` with `ptc_server`.

### 5.3 Confirm direct register read via `peek`

Run:

```bash
/usr/local/bin/peek 0x800201FC
```

**Expected result (success):** output contains the value `0xDEADBEEF`.

---

## 6. Validate from the DUNE server (remote client → server → hardware)

On the DUNE server, run:

```bash
python3 src/ptc_client.py -s <PTC_IP> peek 0x800201FC
```

**Expected result (success):** prints `0xdeadbeef` (case/format may vary).

This confirms the full stack:

* Ethernet connectivity between DUNE server and PTC
* ZMQ REQ/REP communication
* protobuf message packing (`Command` with `Any`)
* server dispatch for `Peek`
* PTC register access through `/dev/mem`

---

## Successful “happy-path” terminal output (example)

### DUNE server

```text
$ ./deploy.sh <PTC_IP>
=== Building on dune-server ===
... (protoc + g++ build output) ...
=== Copying binaries to PTC (<PTC_IP>) ===
ptc_server  100% ...
peek        100% ...
=== Installing to /usr/local/bin on PTC ===
Installed: /usr/local/bin/ptc_server and /usr/local/bin/peek
ptc_server log: /var/log/ptc_server.log
=== Done ===

$ python3 src/ptc_client.py -s <PTC_IP> peek 0x800201FC
0xdeadbeef
```

### PTC

```text
# pidof ptc_server
1234

# ss -lntp | grep 3345
LISTEN ... 0.0.0.0:3345 ... users:(("ptc_server",pid=1234,fd=3))

# /usr/local/bin/peek 0x800201FC
... 0xDEADBEEF
```
-- 
## Development workflow

### Recommended git usage

* Make small commits with messages that describe **what changed** and **how it was validated**.
* Tag “known good” states before test-stand campaigns.

Example:

```bash
git tag -a stand-ready-YYYYMMDD -m "PTC server + basic ping/peek validated on bench"
git push --tags
```

### Branching

* `main` should be kept in a runnable state.
* Use feature branches for larger changes (e.g., `feature/i2c-sensors`, `feature/wib-i2c`).

---

## Roadmap (near-term)

This repo is being developed alongside a test stand consisting of:

* 1× PTC
* 1× WIB
* (optional) 1× FEMB
* A DUNE server machine on the same network

Planned incremental milestones:

1. Communication baseline: server launches reliably; client can ping/read a known register.
2. Local PTC monitoring: PTC-side I2C sensors + EEPROM identification.
3. WIB-side monitoring/control: PTC I2C via PL (“WIB I2C”) path after connectivity/addressing are confirmed.
4. Operational robustness: logging, error handling, timeouts, and clear failure modes.


