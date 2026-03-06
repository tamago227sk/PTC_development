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

// page size for mapping (4KB)
static constexpr size_t PTC_REG_SIZE = 0x200;

// Based on top_RTL.v: VP12_EN[0] = reg_rw_in[4]
// Register 4 * 4 bytes/reg = offset 0x10
static constexpr size_t PWR_EN_OFFSET = 0x10; 

/* Constructor */
// set the reg_ptr to MAP_FAILED as a sentinel value to indicate mapping failure for later
PTC::PTC() : reg_ptr((volatile uint32_t*)MAP_FAILED) {
    // 1. Initialize I2C (Bus 0 as per documentation)
    if (i2c_init(&selected_i2c, "/dev/i2c-0") != 0) {
        glog.log("PTC: I2C init failed\n");
    } else {
        glog.log("PTC: I2C initialized successfully\n");
    }

    // 2. Persistent Hardware Mapping
    #ifndef SIMULATION
    int fd = open("/dev/mem", O_RDWR | O_SYNC);

    if (fd < 0) {
        glog.log("PTC: Failed to open /dev/mem: %s\n", strerror(errno));
    } 
    
    else {
        // Map 4KB starting at PTC_REG_BASE (0x80020000)
        void* ptr = mmap(NULL, PTC_REG_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, PTC_REG_BASE);
        if (ptr == MAP_FAILED) {
            glog.log("PTC: mmap failed: %s\n", strerror(errno));
        } 
        
        else {
            // Cast to volatile as defined in header
            reg_ptr = (volatile uint32_t*)ptr;
            glog.log("PTC: Hardware registers mapped at 0x%08X\n", PTC_REG_BASE);
        }

        close(fd); 
    }
    #endif
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

    // Calculate byte offset from the base
    size_t offset = addr - PTC_REG_BASE;
    
    // Use a char* to move exactly 'offset' bytes, then cast to uint32_t*
    return *(volatile uint32_t*)((uint8_t*)reg_ptr + offset);
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

    size_t idx = (addr - PTC_REG_BASE) / 4;
    reg_ptr[idx] = val;
    #endif
}

/* Power on WIB */
void PTC::power_wib(int slot, bool on) {
    // This currently does not perform any hardware writes.
    glog.log("PTC: power_wib called for slot %d (on=%s) - [Placeholder: No action taken]\n", slot, on ? "true" : "false");
    return;
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

    // PCA9544A Control Register: 
    // Bits [2:0] select the channel. 0x4 enables channel 0, 0x5 channel 1, etc.
    // Usually, 0x04 | bus_idx is the command byte.
    uint8_t control_byte = 0x04 | bus_idx; 
    
    // Mux Address is typically 0x70 on these boards
    if (i2c_write(&this->selected_i2c, 0x70, &control_byte, 1) != 0) {
        glog.log("PTC: Failed to select I2C bus %d\n", bus_idx);
    } else {
        glog.log("PTC: I2C bus %d selected\n", bus_idx);
    }
}