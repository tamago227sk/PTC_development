// peek.cc
#include "ptc.h"
#include <iostream>

// in the ptc_init.sh, I have peek 0x800201FC as a sanity check for the PTC server
// so the argc = 2 is expected, where argv will be [peek, 0x800201FC]
int main(int argc, char** argv) {
    if (argc < 2) return 1;

    // no check is needed for argv[0]='peek' but construct
    // so just extract the address, which is 0x800201FC in ptc_init.sh
    // and purse it as a hex number
    size_t addr = std::stoul(argv[1], nullptr, 16);

    // bilding the PTC object will perform the hardware mapping and I2C initialization
    PTC ptc;
    // and print out the value at the address in hex format, which should be 0xDEADBEEF
    std::cout << "0x" << std::hex << ptc.peek(addr) << std::endl;
    return 0;
}