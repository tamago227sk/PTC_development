# PTC_development

Software development repository for the **PTC (Power and Timing Card)** control stack, intended for bench / test-stand use with a DUNE server machine and (eventually) a PTC + WIB (+ optional FEMB) setup.

The goal of this repo is modest and practical: provide a minimal, reproducible way to (1) bring up a PTC-side service, (2) validate basic communication paths, and (3) extend toward slow-control features (e.g., I2C sensor reads, EEPROM ID, and WIB-facing monitoring/control) once hardware access is available.

---

## Contents

* `src/`
  C++ source and headers for the PTC service and utilities, plus protobuf definition(s).
* `build.sh`
  Builds the server and helper utilities (and generates protobuf bindings).
* `ptc_init.sh`
  Startup script for running the PTC service on the target environment.
* Python client(s)
  Minimal scripts used to validate connectivity and basic register access (e.g., “does it respond at all?” tests).

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
│   ├── peek.cc
│   └── (generated) ptc.pb.cc / ptc.pb.h / ptc_pb2.py
└── (optional) ptc_client.py / ptc.py / ptc_client_ping.py
```

Notes:

* Protobuf outputs are typically generated during build (see `build.sh`).
* Compiled binaries (`ptc_server`, `peek`, etc.) are build artifacts and should not be committed.

---

## Dependencies

### Build-time

* C++ compiler (`g++` or `clang++`, C++11 or later)
* `protoc` (Protocol Buffers compiler)
* Protobuf development libraries (`libprotobuf`)
* ZeroMQ (`libzmq`)

### Runtime

* ZeroMQ runtime
* Access to relevant device files (e.g., `/dev/i2c-*` if/when I2C is used)

---

## Quick start

These steps assume you are in the repository root (`PTC_development/`).

### 0) One-time: install dependencies (macOS)

If you use Homebrew:

```bash
brew install protobuf zeromq
```

You will also need a C++ compiler (Xcode Command Line Tools provides `clang++`).

Verify:

```bash
protoc --version
```

---

### 1) Build (generates protobuf + compiles binaries)

```bash
chmod +x build.sh
./build.sh
```

After a successful build, you should see server/util binaries in the repo root (e.g., `ptc_server` or `tpc_server`, and `peek`).

---

### 2) Run the server (two options)

**Option A: use the init script**

```bash
chmod +x ptc_init.sh
./ptc_init.sh
```

**Option B: run the server binary directly**

If your build produces `tpc_server`:

```bash
./tpc_server
```

If your build produces `ptc_server`:

```bash
./ptc_server
```

Leave the server running in this terminal.

---

### 3) Sanity-check from a second terminal

Open a new terminal in the same repo directory and run your Python client.

If you have `ptc_client.py`:

```bash
python3 ptc_client.py
```

If you have `ptc_client_ping.py`:

```bash
python3 ptc_client_ping.py
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

---

## Troubleshooting

### GitHub authentication fails with “Password authentication is not supported”

GitHub does not accept account passwords for HTTPS Git operations. Use a Personal Access Token, GitHub CLI (`gh auth login`), or SSH keys.

### Build fails: `protoc: command not found`

Install Protocol Buffers compiler (`protoc`).

### Runtime I2C issues

If/when I2C is enabled:

* verify the correct `/dev/i2c-*` device
* verify user permissions (may require group membership or `sudo`)
* verify bus wiring and pull-ups on the test stand

---

## Contributing / Notes

This repository is currently optimized for rapid iteration during hardware bring-up.

* Prioritize clear logging and minimal “magic”.
* Prefer simple, testable steps over clever abstractions.
* Document any assumption that touches hardware (addresses, bus numbers, register meanings, timing).

When in doubt: add one more sanity check before adding one more feature.
