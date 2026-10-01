#include "aes2_gpu.h"
#include "screen.h"

#define LOG_FAIL    (1U << 1)
#define LOG_REGS    (1U << 2)

#define VERBOSE (LOG_FAIL)

#include "logmacro.h"

#define LOGFAIL(...)    LOGMASKED(LOG_FAIL,  __VA_ARGS__)
#define LOGREGS(...)    LOGMASKED(LOG_REGS, __VA_ARGS__)

DEFINE_DEVICE_TYPE(VME_AESTHEDES2_GPU, aesthedes2_vme_gpu_device, "aesthedes2_gpu", "Aesthedes2 VME GPU");

void aesthedes2_vme_gpu_device::device_add_mconfig(machine_config &config)
{
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_refresh_hz(50);
	screen.set_screen_update("ef9365", FUNC(ef9365_device::screen_update));
	screen.set_size(512, 512);
	screen.set_visarea(0, 512-1, 0, 512-1);

	PALETTE(config, "palette").set_entries(256);

	EF9365(config, m_ef9365, 14_MHz_XTAL/8);
	m_ef9365->set_nb_bitplanes(6);
	m_ef9365->set_screen("screen");
	m_ef9365->set_palette_tag("palette");
	m_ef9365->set_display_mode(ef9365_device::DISPLAY_MODE_512x512);
}


void aesthedes2_vme_gpu_device::device_start()
{
	if (m_base_addr == 0)
		fatalerror("base address not set");

	vme_space(vme::AM_09).install_readwrite_handler(
		m_base_addr, m_base_addr + 0xff,
		read32_delegate(*this, FUNC(aesthedes2_vme_gpu_device::read32)),
		write32_delegate(*this, FUNC(aesthedes2_vme_gpu_device::write32)));

	vme_space(vme::AM_0d).install_readwrite_handler(
		m_base_addr, m_base_addr + 0xff,
		read32_delegate(*this, FUNC(aesthedes2_vme_gpu_device::read32)),
		write32_delegate(*this, FUNC(aesthedes2_vme_gpu_device::write32)));
}

u32 aesthedes2_vme_gpu_device::read32(address_space &space, offs_t offset, u32 mem_mask)
{
	if (!(ACCESSING_BITS_16_23) && !(ACCESSING_BITS_0_7)) {
		LOGFAIL("unknown read @%08x mask=%08x\n", m_base_addr + (offset << 2), mem_mask);
        return 0;
    }

	int card_offset = (offset << 1);
    int shift;
    if (ACCESSING_BITS_16_23) {
        shift = 16;
    }
    if (ACCESSING_BITS_0_7) {
		card_offset += 1;
        shift = 0;
    }

	bool is_ef9365 = (card_offset < 0xC);
	
	u8 data;
	if (is_ef9365) {
		data = m_ef9365->data_r(card_offset);
	} else {
		data = 0xff;
	}
	
    LOGREGS("%s reg READ  @%02x data=%02x\n", machine().describe_context(), card_offset, data);
	return data << shift;
}

void aesthedes2_vme_gpu_device::write32(address_space &space, offs_t offset, u32 data, u32 mem_mask)
{
    if (!(ACCESSING_BITS_16_23) && !(ACCESSING_BITS_0_7)) {
        LOGFAIL("unknown write @%08x mask=%08x data=%08x\n", m_base_addr + (offset << 2), mem_mask, data);
        return;
    }

    int card_offset = (offset << 1);
    int shift;
    if (ACCESSING_BITS_16_23) {
        shift = 16;
    }
    if (ACCESSING_BITS_0_7) {
        card_offset += 1;
        shift = 0;
    }

    uint8_t reg_data = data >> shift;

	LOGREGS("%s reg WRITE @%02x data=%02x\n", machine().describe_context(), card_offset, reg_data);
	
	bool is_ef9365 = (card_offset < 0xC);
	if (is_ef9365) {
		m_ef9365->data_w(card_offset, reg_data);
	}

	if (card_offset == 0x0E) {
		if (reg_data < 0x40) {
			m_ef9365->set_color_filler(reg_data);
		} else {
			LOGFAIL("GPU maybe_color_index too high data=%02x\n", reg_data);
		}
	}
}