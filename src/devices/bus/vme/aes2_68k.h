#include "emu.h"

#include "bus/vme/vme.h"
#include "bus/vme/vme_cards.h"
#include "cpu/m68000/m68000.h"
#include "machine/6821pia.h"

DECLARE_DEVICE_TYPE(VME_AESTHEDES2_68K, aesthedes2_vme_68k_device);

class aesthedes2_vme_68k_device : public device_t, public device_vme_card_interface
{
public:
    aesthedes2_vme_68k_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
        : device_t(mconfig, VME_AESTHEDES2_68K, tag, owner, clock)
        , device_vme_card_interface(mconfig, *this)
        , m_cpu(*this, "cpu")
        , m_pia_a(*this, "pia_a")
        , m_pia_b(*this, "pia_b")
        , m_pia_c(*this, "pia_c")
        , m_pia_d(*this, "pia_d")
    {
    }

    void set_rom(const char *rom_name);

protected:
    // device_t overrides
    virtual void device_start() override ATTR_COLD;

    virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

private:
	required_device<m68000_device> m_cpu;
    required_device<pia6821_device> m_pia_a;
    required_device<pia6821_device> m_pia_b;
    required_device<pia6821_device> m_pia_c;
    required_device<pia6821_device> m_pia_d;

    void main_map(address_map &map) ATTR_COLD;

    const char *m_rom_name;
};
