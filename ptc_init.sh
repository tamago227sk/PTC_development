#!/bin/sh

echo "PTC: Starting initialization..." >> /var/log/ptc_startup.log

# Wait for hardware to settle
sleep 2

# Prevent duplicate server
if pgrep ptc_server > /dev/null; then
    echo "ptc_server already running" >> /var/log/ptc_startup.log
    exit 0
fi

# Hardware sanity check
VAL=$(/bin/peek 0x800201FC)

if [ -z "$VAL" ]; then
    echo "ERROR: peek failed" >> /var/log/ptc_startup.log
else
    echo "Register 0x800201FC reads $VAL" >> /var/log/ptc_startup.log
fi

# Start server
echo "Starting ptc_server on port 7820" >> /var/log/ptc_startup.log
/bin/ptc_server 2>/var/log/ptc_server.err >/var/log/ptc_server.log &

sleep 2

echo "PTC: Initialization complete." >> /var/log/ptc_startup.log
