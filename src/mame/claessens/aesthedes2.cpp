#include "emu.h"

#include "bus/vme/vme.h"
#include "bus/vme/vme_cards.h"

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
        VME_SLOT(config, "crate5:04", vme_cards, "pme6822");
    }

};

static INPUT_PORTS_START(aesthedes2)
INPUT_PORTS_END

}

ROM_START(aesthedes2)
ROM_END

SYST(1985+, aesthedes2, 0, 0, aesthedes2, aesthedes2, aesthedes2_state, empty_init, "Claessens Product Consultants", "Aesthedes 2", MACHINE_NO_SOUND);
