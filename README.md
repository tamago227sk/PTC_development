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

## Quick start

These steps assume you are in the repository root (`PTC_development/`).


### 1) Build (generates protobuf + compiles binaries)

```bash
chmod +x build.sh
./build.sh
```

After a successful build, you should see server/util binaries in the repo root (e.g., `ptc_server` and `peek`).

---

### 2) Run the server (two options)

**Option A: use the init script**

```bash
chmod +x ptc_init.sh
./ptc_init.sh
```

**Option B: run the server binary directly**

With `ptc_server` produced via build:

```bash
./ptc_server
```

This will initiate and leave the server running in this terminal.

---

### 3) Sanity-check from a second terminal

Open a new terminal in the same repo directory and run the Python client.


```bash
python3 ptc_client.py
```


Expected outcome: the client connects to the server and performs a minimal read (e.g., a known register value such as `0xDEADBEEF`) to confirm end-to-end communication.

---

### 4) Stop the server

Press `Ctrl+C` in the server terminal.

---

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


