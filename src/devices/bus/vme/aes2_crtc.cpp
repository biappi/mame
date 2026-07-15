#include "aes2_crtc.h"
#include "test_font_8x12.h"

#define LOG_FAIL    (1U << 1)
#define LOG_REGS    (1U << 2)

#define VERBOSE (LOG_FAIL|LOG_REGS)

#include "logmacro.h"

#define LOGFAIL(...)    LOGMASKED(LOG_FAIL,  __VA_ARGS__)
#define LOGREGS(...)    LOGMASKED(LOG_REGS,  __VA_ARGS__)

DEFINE_DEVICE_TYPE(VME_AESTHEDES2_CRTC, aesthedes2_vme_crtc_device, "aesthedes2_crtc", "Aesthedes2 VME CRTC (AES C100/0009)");

void aesthedes2_vme_crtc_device::device_start()
{
    if (m_base_addr == 0)
        fatalerror("base address not set");

    m_video_ram.resize(0x7e8);

	save_item(NAME(m_video_ram));
	save_item(NAME(m_char_latch));

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
    // most of this is copied from mame/src/devices/bus/coco/coco_wpk.cpp
    screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
    screen.set_raw(/*xtal*/14_MHz_XTAL, /*htotal*/896, /*hbend*/0, /*hbstart*/640, /*vtotal*/313, /*vbend*/0, /*vbstart*/288);
    screen.set_screen_update("crtc", FUNC(mc6845_device::screen_update));

    R6545_1(config, m_crtc, 14_MHz_XTAL / 8);
    m_crtc->set_screen("screen");
    m_crtc->set_show_border_area(false);
    m_crtc->set_char_width(8);
    m_crtc->set_on_update_addr_change_callback(FUNC(aesthedes2_vme_crtc_device::crtc_addr));
    m_crtc->set_update_row_callback(FUNC(aesthedes2_vme_crtc_device::crtc_update_row));
}

u32 aesthedes2_vme_crtc_device::read32(address_space &space, offs_t offset, u32 mem_mask)
{
    if (!ACCESSING_BITS_16_23)
        LOGREGS("%s read @%08x mask=%08x\n", machine().describe_context(), m_base_addr + (offset << 2), mem_mask);

    if (ACCESSING_BITS_16_23) {
        return ((u32)m_crtc->status_r()) << 16;
    } else if (ACCESSING_BITS_0_7) {
        return m_crtc->register_r();
    } else {
        LOGFAIL("unknown read @%08x mask=%08x\n", m_base_addr + (offset << 2), mem_mask);
        return 0;
    }
}

void aesthedes2_vme_crtc_device::write32(address_space &space, offs_t offset, u32 data, u32 mem_mask)
{
    LOGREGS("%s write @%08x mask=%08x data=%08x\n", machine().describe_context(), m_base_addr + (offset << 2), mem_mask, data);

    if (ACCESSING_BITS_16_23) {
        m_crtc->address_w((u8)(data >> 16));
    } else if (ACCESSING_BITS_0_7) {
        m_crtc->register_w((u8)data);
        m_char_latch = (u8)data;
    } else {
        LOGFAIL("unknown write @%08x mask=%08x data=%08x\n", m_base_addr + (offset << 2), mem_mask, data);
    }
}

MC6845_ON_UPDATE_ADDR_CHANGED(aesthedes2_vme_crtc_device::crtc_addr)
{
    int row = address / 80;
    int column = address % 80;
	logerror("crtc_addr: %04x (R%02d C%02d) %d\n", address, row, column, strobe);
	m_video_ram[address] = m_char_latch;
}

MC6845_UPDATE_ROW(aesthedes2_vme_crtc_device::crtc_update_row)
{
    // printf("ma=%4x ra=%3d y=%3d x_count=%2d cursor_x=%2d de=2%d hbp=3%d vbp=2%d  -- ",
    //      ma, ra, y, x_count, cursor_x, de, hbp, vbp);

    // for (int i = 0; i < x_count; i++)
    //     printf("%c", m_video_ram[ma + i]);

    // printf("\n");

    u32 *p = &bitmap.pix(y);

    for (int i = 0; i < x_count; i++)
    {
        rgb_t fg = rgb_t::white();
		rgb_t bg = rgb_t::black();

        auto ch = m_video_ram[ma + i];
        u8 data = 0x00;

        if (ch >= 0x20 && ch < 0x7e) {
            data = test_font_8x12[ch - 0x20][ra];
        } else {
            data = (ra & 1) ? 0x55 : 0xaa;
        }

        for (int j = 0; j < 8; j++)
        {
            *p++ = BIT(data, 7-j) ? fg : bg;
        }
    }
}