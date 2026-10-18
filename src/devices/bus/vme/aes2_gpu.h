#include "emu.h"

#include "bus/vme/vme.h"
#include "bus/vme/vme_cards.h"
#include "video/ef9365.h"

DECLARE_DEVICE_TYPE(VME_AESTHEDES2_GPU, aesthedes2_vme_gpu_device);

class aesthedes2_vme_gpu_device : public device_t, public device_vme_card_interface
{
public:
	aesthedes2_vme_gpu_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
		: device_t(mconfig, VME_AESTHEDES2_GPU, tag, owner, clock)
		, device_vme_card_interface(mconfig, *this)
		, m_base_addr(0)
		, m_ef9365(*this, "ef9365")
		, m_palette(*this, "palette")
		, m_vram(*this, "vram", VRAM_SIZE, ENDIANNESS_BIG)
		, m_display_layer_mask(*this, "display_layer_mask", FRAMEBUFFER_PIXELS * sizeof(u64), ENDIANNESS_BIG)
		, m_overlay_layer_mask(*this, "overlay_layer_mask", FRAMEBUFFER_PIXELS, ENDIANNESS_BIG)

	{
	}

	void set_base_address(offs_t addr)
	{
		if (m_base_addr != 0)
			fatalerror("Attempting to set base address twice");

		m_base_addr = addr;
	}

	// The display surface is organised as one bitplane for each of the 64
	// palette entries, followed by the seven board-local work/overlay layers.
	static constexpr unsigned DISPLAY_LAYERS = 64;
	static constexpr unsigned OVERLAY_LAYERS = 7;
	static constexpr unsigned FRAMEBUFFER_LAYERS = DISPLAY_LAYERS + OVERLAY_LAYERS;
	static constexpr unsigned FRAMEBUFFER_WIDTH = 512;
	static constexpr unsigned FRAMEBUFFER_HEIGHT = 512;
	static constexpr unsigned FRAMEBUFFER_PIXELS = FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT;
	static constexpr unsigned VRAM_SIZE = FRAMEBUFFER_LAYERS * ef9365_device::BITPLANE_MAX_SIZE;

	static constexpr unsigned layer_vram_offset(unsigned layer)
	{
		return layer * ef9365_device::BITPLANE_MAX_SIZE;
	}

protected:
    virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;
	virtual void device_post_load() override;

private:
	void update_vram_mode();
	void rebuild_layer_masks();
	void pixel_w(offs_t offset, u8 operation);
	u32 screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

	offs_t m_base_addr;
	required_device<ef9365_device> m_ef9365;
	required_device<palette_device> m_palette;
	memory_share_creator<u8> m_vram;

	// These are a transposed view of m_vram for quick display compositing:
	// one 64-bit colour-layer mask and one seven-bit overlay mask per pixel.
	memory_share_creator<u64> m_display_layer_mask;
	memory_share_creator<u8> m_overlay_layer_mask;

	// Board latches at offsets 0x0c-0x11.  These are not EF9365 registers on
	// the Aesthedes card, even though 0x0c and 0x0d share their positions with
	// the GDP light-pen registers.
	u8 m_vram_mode = 0;
	u8 m_vram_function_mode = 0;
	bool m_vram_serial_1bit = false;
	bool m_vram_through_write = false;
	u8 m_ink = 0;
	u8 m_drawing_target = 0;
	u8 m_display_control = 0;
	u8 m_init_latch = 0;

	u32 read32(address_space &space, offs_t offset, u32 mem_mask);
	void write32(address_space &space, offs_t offset, u32 data, u32 mem_mask);
};
