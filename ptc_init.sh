#!/bin/sh
# PTC Initialization Script (Mirroring wib_init.sh)

# 1. Log the startup process
echo "PTC: Starting initialization..." > /var/log/ptc_startup.log

# 2. Hardware-specific setup (Analogous to WIB clock/I2C phase setup)
# If your PTC requires specific I2C Mux or clock settings at boot:
# Example: Ensure I2C Mux (0x70) is set to a default state if needed
# i2cset -y 0 0x70 0x04 # Select bus 0

# 3. Read the Identifier Register to verify hardware is alive
# This is a sanity check similar to the WIB slot address check
# With the build.sh being run already, the 'peek' utility should be available.
# so when this iline is executed, the shell will search each directory in $PATH and find /bin/peek, 
#which is the utility we just built.
# check peek.cc for usage
echo "Register 0x800201FC reads $(peek 0x800201FC)" >> /var/log/ptc_startup.log

# 4. Launch the PTC Server
echo "Starting ptc_server on port 1234" >> /var/log/ptc_startup.log
# Redirect errors and output to logs
/bin/ptc_server 2>/var/log/ptc_server.err >/var/log/ptc_server.log &

# 5. Wait for server to bind to the socket
sleep 2

echo "PTC: Initialization complete." >> /var/log/ptc_startup.log