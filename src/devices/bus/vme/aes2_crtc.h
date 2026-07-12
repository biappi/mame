#include "emu.h"

#include "bus/vme/vme.h"
#include "bus/vme/vme_cards.h"
#include "video/mc6845.h"
#include "screen.h"
#include "render.h"


DECLARE_DEVICE_TYPE(VME_AESTHEDES2_CRTC, aesthedes2_vme_crtc_device);

class aesthedes2_vme_crtc_device : public device_t, public device_vme_card_interface
{
public:
    aesthedes2_vme_crtc_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
        : device_t(mconfig, VME_AESTHEDES2_CRTC, tag, owner, clock)
        , device_vme_card_interface(mconfig, *this)
        , m_crtc(*this, "crtc")
        , m_base_addr(0)
    {
    }

    void set_base_address(offs_t addr)
    {
        if (m_base_addr != 0)
            fatalerror("Attempting to set base address twice");

        m_base_addr = addr;
    }

protected:
    virtual void device_start() override ATTR_COLD;
    virtual void device_reset() override ATTR_COLD
    {
        //machine().render().target_by_index(1)->set_view(0);
    }

    virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;


private:
	required_device<r6545_1_device> m_crtc;
    offs_t m_base_addr;

    u32 read32(address_space &space, offs_t offset, u32 mem_mask);
    void write32(address_space &space, offs_t offset, u32 data, u32 mem_mask);

	MC6845_ON_UPDATE_ADDR_CHANGED(crtc_addr);
};

