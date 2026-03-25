#include "emu.h"

#include "bus/vme/vme.h"
#include "bus/vme/vme_cards.h"
#include "bus/vme/pme6822.h"

namespace {

class aesthedes2_state : public driver_device
{
public:
    aesthedes2_state(const machine_config &mconfig, device_type type, const char *tag)
        : driver_device(mconfig, type, tag)
    {
    }

    void aesthedes2(machine_config &config)
    {
        VME(config, "crate5");
        VME_SLOT(config, "crate5:04", [](device_slot_interface &device) {
            device.option_add("pme6822", VME_PME6822).machine_config([](device_t *dev) {
                downcast<vme_pme6822_card_device &>(*dev).set_eprom_regions(":os9kernel", ":ae_config_504");
            });
        }, "pme6822", true);
        
        VME(config, "crate3");
        VME_SLOT(config, "crate3:02", [](device_slot_interface &device) {
            device.option_add("pme6822", VME_PME6822).machine_config([](device_t *dev) {
                downcast<vme_pme6822_card_device &>(*dev).set_eprom_regions(":os9kernel", ":ae_config_302");
            });
        }, "pme6822", true);
    }

};

static INPUT_PORTS_START(aesthedes2)
INPUT_PORTS_END

}

ROM_START(aesthedes2)
    ROM_REGION32_BE(0x10000, "os9kernel", 0)
    ROM_LOAD("os9kernel.bin", 0x0000, 0x10000, CRC(1eb71799))

    ROM_REGION32_BE(0x2000, "ae_config_504", 0)
    ROM_LOAD("ae_config_504.bin", 0x0000, 0x2000, CRC(66823c33))

    ROM_REGION32_BE(0x2000, "ae_config_302", 0)
    ROM_LOAD("ae_config_302.bin", 0x0000, 0x2000, CRC(4c618d47))
ROM_END

SYST(1985+, aesthedes2, 0, 0, aesthedes2, aesthedes2, aesthedes2_state, empty_init, "Claessens Product Consultants", "Aesthedes 2", MACHINE_NO_SOUND);
