// license:BSD-3-Clause

#ifndef MAME_VIDEO_AESTHEDES_FRAMEBUFFER_H
#define MAME_VIDEO_AESTHEDES_FRAMEBUFFER_H

#pragma once

#include "emu.h"
#include "emupal.h"


// Provisional model of the C100 framebuffer controller at $ffc000-$ffcfff.
//
// The firmware addresses its byte-wide ports at $ffcx01, so each port is
// separated by $100 bytes.  The C001 RGB stream remains provisional.  The
// boot ROM establishes four table selections at C801 (7e, 7f, 8e and 8f) and
// streams data through C401/C501 while reading C601 for ready/acknowledge
// side effects.
class aesthedes_framebuffer_device : public device_t
{
public:
	aesthedes_framebuffer_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

	// Map the complete $1000-byte register aperture at $ffc000.  Only x01
	// locations are decoded by the current model; the intervening addresses
	// deliberately read as open bus and ignore writes.
	void map(address_map &map);

	// Must be called during machine configuration, before device_start().
	aesthedes_framebuffer_device &set_resolution(u16 width, u16 height);

	// Board glue can use these while the exact Cx01 address-generation protocol
	// remains under investigation.
	void set_write_position(u16 x, u16 y);
	void set_read_position(u16 x, u16 y);
	void clear(u32 rgb = 0);

	u32 screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

	// Register offsets relative to an address-map base of $ffc000.
	static constexpr offs_t PIXEL_WRITE = 0x001;
	static constexpr offs_t CONTROL_1   = 0x101;
	static constexpr offs_t CONTROL_2   = 0x201;
	static constexpr offs_t CONTROL_3   = 0x301;
	static constexpr offs_t CHANNEL_A   = 0x401;
	static constexpr offs_t CHANNEL_B   = 0x501;
	static constexpr offs_t PIXEL_READ  = 0x601;
	static constexpr offs_t CONTROL_7   = 0x701;
	static constexpr offs_t COMMAND     = 0x801;
	static constexpr offs_t CHANNEL_C   = 0x901;

protected:
	virtual void device_start() override;
	virtual void device_reset() override;

private:
	static constexpr offs_t PORT_END = 0x0fff;
	static constexpr unsigned PORT_COUNT = 10;
	static constexpr unsigned TABLE_BANKS = 2;
	static constexpr unsigned TABLE_HALVES = 2;
	static constexpr unsigned TABLE_CHANNELS = 3;
	static constexpr unsigned TABLE_ENTRIES = 256;

	u8 read(offs_t offset);
	void write(offs_t offset, u8 data);

	u8 pixel_stream_r();
	void pixel_stream_w(u8 data);
	void table_select_w(u8 data);
	void table_data_w(unsigned channel, u8 data);
	u8 table_status_r();

	void advance_write_position();
	void advance_read_position();
	u32 pixel_index(u16 x, u16 y) const;

	u16 m_width;
	u16 m_height;
	std::unique_ptr<u32[]> m_pixels;

	// x01, x101, ... x901 latches.  Entries 0 and 6 are also the write/read
	// stream ports, so their values retain the most recent bus byte.
	u8 m_port_latch[PORT_COUNT];

	// The boot ROM fills two 512-entry banks through each C401/C501 data lane.
	// Each bank is split into two 256-entry halves, selected by C801's low bit.
	// C901 is retained as a third, currently unobserved, data lane.
	u8 m_table[TABLE_BANKS][TABLE_HALVES][TABLE_CHANNELS][TABLE_ENTRIES];
	u8 m_table_cursor[TABLE_BANKS][TABLE_HALVES][TABLE_CHANNELS];
	u8 m_table_bank;
	u8 m_table_half;
	bool m_table_selected;

	u8 m_write_latch[3];
	u8 m_read_latch[3];
	u8 m_write_component;
	u8 m_read_component;
	u16 m_write_x;
	u16 m_write_y;
	u16 m_read_x;
	u16 m_read_y;
};


DECLARE_DEVICE_TYPE(AESTHEDES_FRAMEBUFFER, aesthedes_framebuffer_device)

#endif // MAME_VIDEO_AESTHEDES_FRAMEBUFFER_H
