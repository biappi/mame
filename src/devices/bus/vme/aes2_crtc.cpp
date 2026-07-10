#include "aes2_crtc.h"

DEFINE_DEVICE_TYPE(VME_AESTHEDES2_CRTC, aesthedes2_vme_crtc_device, "aesthedes2_crtc", "Aesthedes2 VME CRTC (AES C100/0009)");

void aesthedes2_vme_crtc_device::device_start()
{
    if (m_base_addr == 0)
        fatalerror("base address not set");
        
    vme_space(vme::AM_09).install_readwrite_handler(
        m_base_addr, m_base_addr + 0x1f,
        read32_delegate(*this, FUNC(aesthedes2_vme_crtc_device::read32)),
        write32_delegate(*this, FUNC(aesthedes2_vme_crtc_device::write32)));

    vme_space(vme::AM_0d).install_readwrite_handler(
        m_base_addr, m_base_addr + 0x1f,
        read32_delegate(*this, FUNC(aesthedes2_vme_crtc_device::read32)),
        write32_delegate(*this, FUNC(aesthedes2_vme_crtc_device::write32)));
}

void aesthedes2_vme_crtc_device::device_add_mconfig(machine_config &config)
{
    screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
    screen.set_raw(14_MHz_XTAL, 896, 0, 640, 290, 0, 240);
    screen.set_screen_update("crtc", FUNC(mc6845_device::screen_update));

    R6545_1(config, m_crtc, 14_MHz_XTAL / 8);
    m_crtc->set_screen("screen");
    m_crtc->set_show_border_area(false);
    m_crtc->set_char_width(8);
    // m_crtc->set_on_update_addr_change_callback(FUNC(coco_wpkrs_device::crtc_addr));
    // m_crtc->set_update_row_callback(FUNC(coco_wpkrs_device::crtc_update_row));
}

u32 aesthedes2_vme_crtc_device::read32(address_space &space, offs_t offset, u32 mem_mask)
{
    // if (!machine().side_effects_disabled())
    //     logerror("read @%08x mask=%08x\n", m_base_addr + (offset << 2), mem_mask);

    if (ACCESSING_BITS_16_23) {
        return m_crtc->status_r();
    } else if (ACCESSING_BITS_0_7) {
        return m_crtc->register_r();
    } else {
        logerror("unknown read @%08x mask=%08x\n", m_base_addr + (offset << 2), mem_mask);
        return 0;
    }
}

void aesthedes2_vme_crtc_device::write32(address_space &space, offs_t offset, u32 data, u32 mem_mask)
{
    // if (!machine().side_effects_disabled())
    //     logerror("write @%08x mask=%08x data=%08x\n", m_base_addr + (offset << 2), mem_mask, data);

    if (ACCESSING_BITS_16_23) {
        m_crtc->address_w((u8)(data >> 16));
    } else if (ACCESSING_BITS_0_7) {
        m_crtc->register_w((u8)data);
    } else {
        logerror("unknown write @%08x mask=%08x data=%08x\n", m_base_addr + (offset << 2), mem_mask, data);
    }
}