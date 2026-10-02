// license:BSD-3-Clause

#include "emu.h"
#include "aesthedes_framebuffer.h"

#include "screen.h"

#include <algorithm>


DEFINE_DEVICE_TYPE(AESTHEDES_FRAMEBUFFER, aesthedes_framebuffer_device, "aesthedes_framebuffer", "Aesthedes C100 framebuffer")


aesthedes_framebuffer_device::aesthedes_framebuffer_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, AESTHEDES_FRAMEBUFFER, tag, owner, clock)
	, m_width(512)
	, m_height(512)
	, m_pixels()
	, m_port_latch{}
	, m_table{}
	, m_table_cursor{}
	, m_table_bank(0)
	, m_table_half(0)
	, m_table_selected(false)
	, m_write_latch{}
	, m_read_latch{}
	, m_write_component(0)
	, m_read_component(0)
	, m_write_x(0)
	, m_write_y(0)
	, m_read_x(0)
	, m_read_y(0)
{
}


aesthedes_framebuffer_device &aesthedes_framebuffer_device::set_resolution(u16 width, u16 height)
{
	if (!width || !height)
		throw emu_fatalerror("%s: framebuffer resolution must be non-zero", tag());

	m_width = width;
	m_height = height;
	return *this;
}


void aesthedes_framebuffer_device::device_start()
{
	m_pixels = std::make_unique<u32[]>(u32(m_width) * m_height);

	save_item(NAME(m_width));
	save_item(NAME(m_height));
	save_pointer(NAME(m_pixels), u32(m_width) * m_height);
	save_item(NAME(m_port_latch));
	save_item(NAME(m_table));
	save_item(NAME(m_table_cursor));
	save_item(NAME(m_table_bank));
	save_item(NAME(m_table_half));
	save_item(NAME(m_table_selected));
	save_item(NAME(m_write_latch));
	save_item(NAME(m_read_latch));
	save_item(NAME(m_write_component));
	save_item(NAME(m_read_component));
	save_item(NAME(m_write_x));
	save_item(NAME(m_write_y));
	save_item(NAME(m_read_x));
	save_item(NAME(m_read_y));
}


void aesthedes_framebuffer_device::device_reset()
{
	std::fill_n(m_pixels.get(), u32(m_width) * m_height, 0);
	std::fill_n(m_port_latch, PORT_COUNT, 0);
	std::fill_n(&m_table[0][0][0][0], sizeof(m_table), 0);
	std::fill_n(&m_table_cursor[0][0][0], sizeof(m_table_cursor), 0);
	m_table_bank = 0;
	m_table_half = 0;
	m_table_selected = false;
	std::fill_n(m_write_latch, 3, 0);
	std::fill_n(m_read_latch, 3, 0);
	m_write_component = 0;
	m_read_component = 0;
	m_write_x = 0;
	m_write_y = 0;
	m_read_x = 0;
	m_read_y = 0;
}


void aesthedes_framebuffer_device::map(address_map &map)
{
	// The physical aperture is $ffc000-$ffcfff.  This single handler keeps the
	// offset intact, which matters because the ports are spaced by $100.
	map(0x000, PORT_END).rw(FUNC(aesthedes_framebuffer_device::read), FUNC(aesthedes_framebuffer_device::write));
}


u8 aesthedes_framebuffer_device::read(offs_t offset)
{
	// With a 68000 low-byte mapping (.umask16(0x00ff)), MAME normalizes the
	// physical odd byte addresses to consecutive handled units: $ffc001,
	// $ffc101, ... become offsets $000, $080, ... respectively.  Also accept
	// the literal offsets so this submap remains useful when attached to an
	// 8-bit address space or a direct byte map.
	int port = -1;
	if ((offset & 0xff) == 0x01)
		port = (offset >> 8) & 0x0f;
	else if ((offset & 0x7f) == 0x00)
		port = (offset >> 7) & 0x0f;

	if (port < 0 || port >= PORT_COUNT)
		return 0xff;

	// printf("%s read: %02x\n", machine().describe_context().c_str(), port);

	if (port == (PIXEL_READ >> 8) && m_table_selected)
		return table_status_r();

	if (port == (PIXEL_READ >> 8))
		return pixel_stream_r();

	return m_port_latch[port];
}


void aesthedes_framebuffer_device::write(offs_t offset, u8 data)
{
	int port = -1;
	if ((offset & 0xff) == 0x01)
		port = (offset >> 8) & 0x0f;
	else if ((offset & 0x7f) == 0x00)
		port = (offset >> 7) & 0x0f;

	if (port < 0 || port >= PORT_COUNT)
		return;

	// printf("%s write: %02x --> %02x\n", machine().describe_context().c_str(), port, data);

	m_port_latch[port] = data;

	if (port == (COMMAND >> 8))
		table_select_w(data);
	else if (port == (CHANNEL_A >> 8))
		table_data_w(0, data);
	else if (port == (CHANNEL_B >> 8))
		table_data_w(1, data);
	else if (port == (CHANNEL_C >> 8))
		table_data_w(2, data);
	else if (port == (PIXEL_WRITE >> 8))
		pixel_stream_w(data);
}


void aesthedes_framebuffer_device::table_select_w(u8 data)
{
	// The boot ROM uses 7e/7f and 8e/8f.  The high nibble distinguishes two
	// banks, while bit 0 selects a 256-entry half.  Other C801 commands are
	// still retained in m_port_latch, but do not direct table data.
	if ((data & 0xfe) == 0x7e || (data & 0xfe) == 0x8e)
	{
		m_table_bank = (data >> 4) - 7;
		m_table_half = BIT(data, 0);
		m_table_selected = true;
	}
	else
	{
		m_table_selected = false;
	}
}


void aesthedes_framebuffer_device::table_data_w(unsigned channel, u8 data)
{
	if (!m_table_selected)
		return;

	u8 &cursor = m_table_cursor[m_table_bank][m_table_half][channel];
	u8 const index = cursor++;
	m_table[m_table_bank][m_table_half][channel][index] = data;

	printf("%s -- set table[%2x][%2x][%2x][%2x] = %02x\n", machine().describe_context().c_str(), m_table_bank, m_table_half, channel, index, data);
}


u8 aesthedes_framebuffer_device::table_status_r()
{
	// Boot code uses TST.B on C601 immediately around the C401/C501 streams;
	// it does not branch on the value.  A ready value models the observed
	// acknowledgement without conflating the access with the RGB read stream.
	return 0x00;
}


void aesthedes_framebuffer_device::pixel_stream_w(u8 data)
{
	m_write_latch[m_write_component++] = data;
	if (m_write_component != 3)
		return;

	// C001 is always written in triples.  The hardware's byte order has not
	// yet been electrically verified; RGB is the least-assumptive display
	// order and can be revised in this one location when a trace proves it.
	m_pixels[pixel_index(m_write_x, m_write_y)] = rgb_t(m_write_latch[0], m_write_latch[1], m_write_latch[2]);
	m_write_component = 0;
	advance_write_position();
}


u8 aesthedes_framebuffer_device::pixel_stream_r()
{
	if (!m_read_component)
	{
		u32 const pixel = m_pixels[pixel_index(m_read_x, m_read_y)];
		m_read_latch[0] = u8(pixel >> 16);
		m_read_latch[1] = u8(pixel >> 8);
		m_read_latch[2] = u8(pixel);
	}

	u8 const data = m_read_latch[m_read_component++];
	if (m_read_component == 3)
	{
		m_read_component = 0;
		advance_read_position();
	}

	m_port_latch[PIXEL_READ >> 8] = data;
	return data;
}


void aesthedes_framebuffer_device::set_write_position(u16 x, u16 y)
{
	m_write_x = x % m_width;
	m_write_y = y % m_height;
	m_write_component = 0;
}


void aesthedes_framebuffer_device::set_read_position(u16 x, u16 y)
{
	m_read_x = x % m_width;
	m_read_y = y % m_height;
	m_read_component = 0;
}


void aesthedes_framebuffer_device::advance_write_position()
{
	if (++m_write_x == m_width)
	{
		m_write_x = 0;
		if (++m_write_y == m_height)
			m_write_y = 0;
	}
}


void aesthedes_framebuffer_device::advance_read_position()
{
	if (++m_read_x == m_width)
	{
		m_read_x = 0;
		if (++m_read_y == m_height)
			m_read_y = 0;
	}
}


u32 aesthedes_framebuffer_device::pixel_index(u16 x, u16 y) const
{
	return u32(y) * m_width + x;
}


void aesthedes_framebuffer_device::clear(u32 rgb)
{
	if (m_pixels)
		std::fill_n(m_pixels.get(), u32(m_width) * m_height, rgb);
}


u32 aesthedes_framebuffer_device::screen_update(screen_device &, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(rgb_t::black(), cliprect);

	for (int y = cliprect.min_y; y <= cliprect.max_y && y < m_height; ++y)
	{
		u32 *const dst = &bitmap.pix(y, cliprect.min_x);
		for (int x = cliprect.min_x; x <= cliprect.max_x && x < m_width; ++x)
			dst[x - cliprect.min_x] = m_pixels[pixel_index(x, y)];
	}

	return 0;
}
