#include "ptc.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <sstream>
#include <unistd.h>
#include <fstream>
#include <sys/mman.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <cmath>
#include <math.h>

// page size for mapping (512 bytes)
// 64 R/W registers and 64 R/O registers (64 x 4 + 64 x 4 = 512 bytes = 0x200)
// but this still needs to be verified with the actual firmware!
static constexpr size_t PTC_REG_SIZE = 0x200;

// Based on top_RTL.v: VP12_EN[0] = reg_rw_in[4]
// Register 4 * 4 bytes/reg = offset 0x10
static constexpr size_t PWR_EN_OFFSET = 0x10; 

/* Constructor */
// set the reg_ptr to MAP_FAILED as a sentinel value to indicate mapping failure for later
PTC::PTC() : reg_ptr((volatile uint32_t*)MAP_FAILED) {

    // 1. Hardware Mapping FIRST
    #ifndef SIMULATION
    int fd = open("/dev/mem", O_RDWR | O_SYNC);

    if (fd < 0) {
        glog.log("PTC: Failed to open /dev/mem: %s\n", strerror(errno));
    } 
    
    else {
        void* ptr = mmap(NULL, PTC_REG_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, PTC_REG_BASE);
        if (ptr == MAP_FAILED) {
            glog.log("PTC: mmap failed: %s\n", strerror(errno));
        } 
        
        else {
            reg_ptr = (volatile uint32_t*)ptr;
            glog.log("PTC: Hardware registers mapped at 0x%08X\n", PTC_REG_BASE);

            // enabling the i2c access
            this->poke(0x80020000, 0x00000201);
            glog.log("PTC: I2C path enabled (reg 0x80020000 = 0x00000201)\n");
        }

        close(fd); 
    }
    #endif

    // 2. Initialize I2C (FIXED BUS)
    if (i2c_init(&selected_i2c, "/dev/i2c-1") != 0) {
        glog.log("PTC: I2C init failed on /dev/i2c-1\n");
    } else {
        glog.log("PTC: I2C initialized successfully on /dev/i2c-1\n");
    }
}

/* Destructor */
PTC::~PTC() {
    i2c_free(&this->selected_i2c);
    
    #ifndef SIMULATION
    if (reg_ptr != MAP_FAILED) {
        munmap((void*)reg_ptr, PTC_REG_SIZE);
    }
    #endif
    
    glog.log("PTC destroyed\n");
}



uint32_t PTC::peek(size_t addr) {
    #ifndef SIMULATION
    if (reg_ptr == MAP_FAILED) return 0xFFFFFFFF;

    // Safety: Ensure address is within our 4KB window and 4-byte aligned
    if (addr < PTC_REG_BASE || addr >= (PTC_REG_BASE + PTC_REG_SIZE) || (addr % 4) != 0) {
        glog.log("PTC: Invalid peek at 0x%08X\n", addr);
        return 0xFFFFFFFF;
    }

    size_t idx = (addr - PTC_REG_BASE) >> 2;
    return reg_ptr[idx];
    
    #else
    return 0x0;
    #endif
}

/* Poking function for FPGA register access */
void PTC::poke(size_t addr, uint32_t val) {
    #ifndef SIMULATION
    if (reg_ptr == MAP_FAILED) return;

    // --- SAFETY GUARDS ---
    if (addr < PTC_REG_BASE || 
        addr >= (PTC_REG_BASE + PTC_REG_SIZE) || 
        (addr % 4) != 0) {
        glog.log("PTC: Invalid poke at 0x%08X\n", addr);
        return;
    }

    size_t idx = (addr - PTC_REG_BASE) >> 2;
    reg_ptr[idx] = val;
    #endif
}

/* Power on WIB */
void PTC::power_wib(int slot, bool on) {
    select_bus(slot); 
    // Then perform the I2C write to the power controller (LTC2945)
    glog.log("PTC: power_wib called for slot %d (on=%s) - [Placeholder: No action taken]\n", slot, on ? "true" : "false");
    return;
}


int PTC::read_i2c_reg(uint8_t mux_channel, uint8_t slave, uint8_t reg) {
    select_bus(mux_channel);
    return i2c_reg_read(&selected_i2c, slave, reg);
}


/* Read temperature from sensor - Kept empty per request */
double PTC::read_temperature(uint8_t addr) {
    return 0.0;
}

/* Placeholder for voltage readings */
double PTC::read_voltage(uint8_t channel) {
    return 0.0;
}

/* Placeholder for current readings */
double PTC::read_current(uint8_t addr, double sense_resistor) {
    return 0.0; 
}

/* Status check */
bool PTC::ping() {
    uint32_t val = peek(0x800201FC);
    return (val == 0xDEADBEEF);
}

/* Select the active I2C bus via the PCA9544A Mux */
void PTC::select_bus(uint8_t bus_idx) {
    if (bus_idx > 3) {
        glog.log("PTC: Invalid I2C bus index %d\n", bus_idx);
        return;
    }

    uint8_t control_byte = (1 << bus_idx);

    if (i2c_write(&this->selected_i2c, 0x70, &control_byte, 1) != 0) {
        glog.log("PTC: Failed to select I2C bus %d\n", bus_idx);
    } else {
        glog.log("PTC: I2C bus %d selected (0x%02x)\n", bus_idx, control_byte);
    }
}
