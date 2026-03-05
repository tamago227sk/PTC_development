#ifndef ptc_h
#define ptc_h



#include "i2c.h"
// dont know ehether we nneed to have io_reg, but if we add later,
// #include "io_reg.h"
#include "log.h"

#include <cstdint>
#include <string>
#include <vector>

//Memory base addresses of AXI interfaces
static constexpr size_t PTC_REG_BASE = 0x80020000;
static constexpr uint8_t ADDR_TMP117_BASE = 0x48; // Typical TMP117 addr
static constexpr uint8_t ADDR_LTC2945_WIB0 = 0x67; // Check schematic for actuals

class PTC{
public:
    PTC();
    virtual ~PTC();

    // --- I2C Interface ---//
    i2c_t selected_i2c; // symmetric to WIB's bus ownership

    // --- Debug / Low-Level ---
    uint32_t peek(size_t address);
    void poke(size_t address, uint32_t value);

    // Power Control 
    void power_wib(int slot, bool on);

    // Sensor reading and returning value as double 
    double read_temperature(uint8_t addr);
    double read_voltage(uint8_t channel);
    double read_current(uint8_t addr, double sense_resistor);

    // Status 
    bool ping();

protected:
    volatile uint32_t *reg_ptr; // Pointer to the mapped hardware memory
    // Later:
    // I2C i2c;
    void select_bus(uint8_t bus_idx);
    // IOReg regs;
};

#endif