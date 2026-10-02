#include "aes2_gpu.h"
#include "screen.h"

#define LOG_FAIL    (1U << 1)
#define LOG_REGS    (1U << 2)

#define VERBOSE (LOG_FAIL)

#include "logmacro.h"

#define LOGFAIL(...)    LOGMASKED(LOG_FAIL,  __VA_ARGS__)
#define LOGREGS(...)    LOGMASKED(LOG_REGS, __VA_ARGS__)
#define LOGGPU(...)     printf(__VA_ARGS__) // LOGMASKED(LOG_GPU, __VA_ARGS__)

DEFINE_DEVICE_TYPE(VME_AESTHEDES2_GPU, aesthedes2_vme_gpu_device, "aesthedes2_gpu", "Aesthedes2 VME GPU");

void aesthedes2_vme_gpu_device::device_add_mconfig(machine_config &config)
{
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_refresh_hz(50);
	screen.set_screen_update(FUNC(aesthedes2_vme_gpu_device::screen_update));
	screen.set_size(512, 512);
	screen.set_visarea(0, 512-1, 0, 512-1);

	PALETTE(config, "palette").set_entries(256);

	EF9365(config, m_ef9365, 14_MHz_XTAL/8);
	m_ef9365->set_nb_bitplanes(1);
	m_ef9365->set_screen("screen");
	m_ef9365->set_palette_tag("palette");
	m_ef9365->set_display_mode(ef9365_device::DISPLAY_MODE_512x512);
	m_ef9365->pixel_write_callback().set(FUNC(aesthedes2_vme_gpu_device::pixel_w));
}


void aesthedes2_vme_gpu_device::device_start()
{
	if (m_base_addr == 0)
		fatalerror("base address not set");

	m_vram.resize(VRAM_SIZE);
	save_item(NAME(m_vram));

	vme_space(vme::AM_09).install_readwrite_handler(
		m_base_addr, m_base_addr + 0xff,
		read32_delegate(*this, FUNC(aesthedes2_vme_gpu_device::read32)),
		write32_delegate(*this, FUNC(aesthedes2_vme_gpu_device::write32)));

	vme_space(vme::AM_0d).install_readwrite_handler(
		m_base_addr, m_base_addr + 0xff,
		read32_delegate(*this, FUNC(aesthedes2_vme_gpu_device::read32)),
		write32_delegate(*this, FUNC(aesthedes2_vme_gpu_device::write32)));

	save_item(NAME(m_vram_mode));
	save_item(NAME(m_vram_function_mode));
	save_item(NAME(m_vram_serial_1bit));
	save_item(NAME(m_vram_through_write));
	save_item(NAME(m_ink));
	save_item(NAME(m_drawing_target));
	save_item(NAME(m_display_control));
	save_item(NAME(m_init_latch));

	update_vram_mode();
}

void aesthedes2_vme_gpu_device::update_vram_mode()
{
	// The two observed erase routes are paired before returning to route 0.
	// Their physical decoding is not known, so the mode is retained and traced
	// here without imposing a speculative EF9365 plane selection.
	switch (m_vram_mode)
	{
	case 0x00:
		LOGGPU("%s: GPU update plane selection: 0x00\n", machine().describe_context().c_str());
		break;

	case 0x04:
		LOGGPU("%s: GPU update plane selection: 0x04\n", machine().describe_context().c_str());
		break;

	case 0x05:
		LOGGPU("%s: GPU update plane selection: 0x05\n", machine().describe_context().c_str());
		break;

	default:
		LOGGPU("%s: GPU update plane selection: unknown %02x\n", machine().describe_context().c_str(), m_vram_mode);
		break;
	}
}

void aesthedes2_vme_gpu_device::device_reset()
{
	m_vram_mode = 0;
	m_vram_function_mode = 0;
	m_vram_serial_1bit = false;
	m_vram_through_write = false;
	m_ink = 0;
	m_drawing_target = 0;
	m_display_control = 0;
	m_init_latch = 0;

	std::fill(m_vram.begin(), m_vram.end(), 0x00);

	update_vram_mode();
}

void aesthedes2_vme_gpu_device::pixel_w(offs_t offset, u8 operation)
{
	const u8 mask = 0x80 >> (offset & 7);
	const unsigned byte_offset = offset >> 3;
	const unsigned planes = m_drawing_target == 0 ? DISPLAY_BITPLANES : 1;

	const int ink = m_drawing_target == 0 ? m_ink : 1;

	for (unsigned plane = 0; plane < planes; plane++)
	{
		u8 &pixel = m_vram[target_vram_offset(m_drawing_target, plane) + byte_offset];

		if (!BIT(operation, 0) && BIT(ink, plane))
			pixel |= mask;
		else
			pixel &= ~mask;
	}
}

u8 aesthedes2_vme_gpu_device::target_pixel(u8 target, int pixel_offset) const
{
	u8 color = 0;
	const unsigned planes = target == 0 ? DISPLAY_BITPLANES : 1;

	for (unsigned plane = 0; plane < planes; plane++)
	{
		const unsigned offset = target_vram_offset(target, plane) + (pixel_offset >> 3);
		if (BIT(m_vram[offset], ~pixel_offset & 7))
			color |= 1U << plane;
	}

	return color;
}

u32 aesthedes2_vme_gpu_device::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	for (int y = cliprect.min_y; y <= cliprect.max_y; y++)
	{
		for (int x = cliprect.min_x; x <= cliprect.max_x; x++)
		{
			const int pixel_offset = (y * 512) + x;
			u8 color = target_pixel(0, pixel_offset);
			rgb_t col = m_palette->pen(color);

			for (u8 target = 1; target < DRAW_TARGETS; target++)
			{
				const u8 overlay_color = target_pixel(target, pixel_offset);
				if (overlay_color != 0)
					col = rgb_t(0xff, 0x00, 0xff);
			}

			bitmap.pix(y, x) = col;
		}
	}

	return 0;
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

	u8 data;
	if (card_offset < 0x0c)
	{
		data = m_ef9365->data_r(card_offset);
	}
	else
	{
		switch (card_offset)
		{
		case 0x0c: data = m_vram_mode; break;
		case 0x0d: data = m_vram_function_mode; break;
		case 0x0e: data = m_ink; break;
		case 0x0f: data = m_display_control; break;
		case 0x11: data = m_init_latch; break;
		default:
			data = 0xff;
			LOGGPU("%s unknown reg READ  @%02x data=%02x\n", machine().describe_context().c_str(), card_offset, data);
			break;
		}
	}

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

	if (card_offset < 0x0c)
	{
		m_ef9365->data_w(card_offset, reg_data);
		return;
	}

	switch (card_offset)
	{
	case 0x0c:
		m_vram_mode = reg_data;
		update_vram_mode();
		break;

	case 0x0d:
		m_vram_function_mode = reg_data;
		// The $03 then $05 setup sequence has two effects.  Keep the $03
		// serial-organisation change after the $05 THROUGH-write selection.
		if (reg_data == 0x03)
			m_vram_serial_1bit = true;
		else if (reg_data == 0x05)
			m_vram_through_write = true;
		else
			LOGGPU("%s: GPU update function mode: unknown %02x\n", machine().describe_context().c_str(), reg_data);
		break;

	case 0x0e:
		if (reg_data < 0x40)
		{
			m_drawing_target = 0;
			m_ink = reg_data;

			LOGGPU("%s: GPU update drawing target: 0\n", machine().describe_context().c_str());
		}
		else if (reg_data <= 0x46)
		{
			// $40-$46 are board-local work/overlay contexts rather than
			// 6-bit palette values.  Keep their writes off the normal surface.
			m_drawing_target = (reg_data - 0x40) + 1;
			LOGGPU("%s: GPU update drawing target: %02x\n", machine().describe_context().c_str(), m_drawing_target);
		}
		else
		{
			LOGGPU("%s: GPU unknown special context data=%02x\n", machine().describe_context().c_str(), reg_data);
		}
		break;

	case 0x0f:
		m_display_control = reg_data;
		LOGGPU("%s: GPU update display control: %02x\n", machine().describe_context().c_str(), reg_data);
		break;

	case 0x11:
		m_init_latch = reg_data;
		break;

	default:
		LOGGPU("%s unknown reg WRITE @%02x data=%02x\n", machine().describe_context().c_str(), card_offset, reg_data);
		break;
	}
}
