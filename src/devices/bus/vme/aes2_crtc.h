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
    {
    }

protected:
    virtual void device_start() override ATTR_COLD
    {
        vme_space(vme::AM_09).install_readwrite_handler(
            0x04ffa000, 0x04ffa0ff,
            read32_delegate(*this, FUNC(aesthedes2_vme_crtc_device::read32)),
            write32_delegate(*this, FUNC(aesthedes2_vme_crtc_device::write32)));

        vme_space(vme::AM_0d).install_readwrite_handler(
            0x04ffa000, 0x04ffa0ff,
            read32_delegate(*this, FUNC(aesthedes2_vme_crtc_device::read32)),
            write32_delegate(*this, FUNC(aesthedes2_vme_crtc_device::write32)));
    }

    virtual void device_reset() override ATTR_COLD
    {
        //machine().render().target_by_index(1)->set_view(0);
    }

    virtual void device_add_mconfig(machine_config &config) override ATTR_COLD
    {
        screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
        screen.set_raw(14.318181_MHz_XTAL, 896, 0, 640, 290, 0, 240);
        screen.set_screen_update("crtc", FUNC(mc6845_device::screen_update));

        R6545_1(config, m_crtc, 14.318181_MHz_XTAL / 8);
        m_crtc->set_screen("screen");
        m_crtc->set_show_border_area(false);
        m_crtc->set_char_width(8);
        // m_crtc->set_on_update_addr_change_callback(FUNC(coco_wpkrs_device::crtc_addr));
        // m_crtc->set_update_row_callback(FUNC(coco_wpkrs_device::crtc_update_row));
    }


private:
	required_device<r6545_1_device> m_crtc;

    u32 read32(address_space &space, offs_t offset, u32 mem_mask)
    {
        if (!machine().side_effects_disabled())
            logerror("read @%08x mask=%08x\n", 0x04ffa000U + (offset << 2), mem_mask);

        return 0xeeU;
    }

    void write32(address_space &space, offs_t offset, u32 data, u32 mem_mask)
    {
        if (!machine().side_effects_disabled())
            logerror("write @%08x mask=%08x data=%08x\n", 0x04ffa000U + (offset << 2), mem_mask, data);
    }
};

DEFINE_DEVICE_TYPE(VME_AESTHEDES2_CRTC, aesthedes2_vme_crtc_device, "aesthedes2_crtc", "Aesthedes2 VME CRTC (AES C100/0009)");
