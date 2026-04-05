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

    // 1. Initialize I2C (Bus 0 as per documentation)
    if (i2c_init(&selected_i2c, "/dev/i2c-0") != 0) {
        glog.log("PTC: I2C init failed\n");
    } else {
        glog.log("PTC: I2C initialized successfully\n");
    }

    // 2. Hardware Mapping
    #ifndef SIMULATION
    int fd = open("/dev/mem", O_RDWR | O_SYNC);

    if (fd < 0) {
        glog.log("PTC: Failed to open /dev/mem: %s\n", strerror(errno));
    } 
    
    else {
        // Map 512 bytes starting at PTC_REG_BASE (0x80020000)
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

/*Helper function for reading the i2c registers*/
int PTC::i2c_read_reg16(uint8_t addr, uint8_t reg, uint16_t &val) {
    uint8_t wbuf[1] = {reg};
    uint8_t rbuf[2];

    int ret = i2c_writeread(&selected_i2c, addr, wbuf, 1, rbuf, 2);
    if (ret < 0) {
        glog.log("I2C read failed (addr=0x%02X reg=0x%02X)\n", addr, reg);
        return -1;
    }

    val = (rbuf[0] << 8) | rbuf[1]; // big endian
    return 0;
}

/* Read temperature from sensor */
double PTC::read_temperature(uint8_t addr) {
    select_bus(0); // NEEDS TO BE CHECKED!!!!

    uint16_t raw;

    // Assume we're reading 0x00 register
    if (i2c_read_reg16(addr, 0x00, raw) < 0) {
        return 0.0;
    }

    // TMP117 uses signed 16-bit
    int16_t signed_raw = (int16_t)raw;

    // for TMP117, 1 LSB = 0.0078125 C
    double temp = signed_raw * 0.0078125;
    glog.log("TMP117[0x%02X] = %.2f C\n", addr, temp);

    return temp;
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
