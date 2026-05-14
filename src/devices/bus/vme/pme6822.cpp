// license:BSD-3-Clause
// copyright-holders:Enrico Gueli
#include "emu.h"
#include "pme6822.h"

#include "bus/nscsi/hd.h"

#ifdef _MSC_VER
#define FUNCNAME __func__
#else
#define FUNCNAME __PRETTY_FUNCTION__
#endif

#define VERBOSE (LOG_GENERAL)
#include "logmacro.h"

DEFINE_DEVICE_TYPE(VME_PME6822,   vme_pme6822_card_device,   "pme6822",   "Radstone PME 68-22")

namespace {

static void scsi_devices(device_slot_interface &device)
{
	device.option_add("harddisk", NSCSI_HARDDISK);
}

} // anonymous namespace

vme_pme6822_card_device::vme_pme6822_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
    : device_t(mconfig, VME_PME6822, tag, owner, clock)
    , device_vme_card_interface(mconfig, *this)
    , m_maincpu(*this, "maincpu")
    , m_duart(*this, "duart")
    , m_rtc(*this, "rtc")
    , m_ncr(*this, "scsi:7:ncr5385")
    , m_duart_a_tx(*this)
    , m_eprom0_region("eprom0")
    , m_eprom1_region("eprom1")
    , m_ncr_reg6_cache(0)
    , m_ncr_reg6_cache_valid(false)
{
}

void vme_pme6822_card_device::set_eprom_regions(const char *eprom0, const char *eprom1)
{
    m_eprom0_region = eprom0;
    m_eprom1_region = eprom1;
}

void vme_pme6822_card_device::device_add_mconfig(machine_config &config)
{
	M68020(config, m_maincpu, 16670000);
    m_maincpu->set_addrmap(AS_PROGRAM, &vme_pme6822_card_device::main_map);

    MC68681(config, m_duart, 8_MHz_XTAL / 2);
    m_duart->set_clocks(500000, 500000, 1000000, 1000000);
    m_duart->irq_cb().set_inputline(m_maincpu, M68K_IRQ_5);
    m_duart->outport_cb().set(FUNC(vme_pme6822_card_device::duart_output));
    m_duart->a_tx_cb().set(FUNC(vme_pme6822_card_device::duart_a_tx));

    DS1216E(config, m_rtc);

    NSCSI_BUS(config, "scsi");
    NSCSI_CONNECTOR(config, "scsi:0", scsi_devices, nullptr);
    NSCSI_CONNECTOR(config, "scsi:1", scsi_devices, nullptr);
    NSCSI_CONNECTOR(config, "scsi:2", scsi_devices, "harddisk");
    NSCSI_CONNECTOR(config, "scsi:3", scsi_devices, nullptr);
    NSCSI_CONNECTOR(config, "scsi:4", scsi_devices, nullptr);
    NSCSI_CONNECTOR(config, "scsi:5", scsi_devices, nullptr);
    NSCSI_CONNECTOR(config, "scsi:6", scsi_devices, nullptr);
    NSCSI_CONNECTOR(config, "scsi:7").option_set("ncr5385", NCR5385).clock(10'000'000).machine_config(
        [this](device_t *device)
        {
            ncr5385_device &adapter = downcast<ncr5385_device &>(*device);
            adapter.irq().set(*this, FUNC(vme_pme6822_card_device::ncr_irq_w));
        });
}

void vme_pme6822_card_device::main_map(address_map &map)
{
    // 64KB for OS-9 kernel ROM
    map(0x00000000, 0x0000ffff).rom().region(m_eprom0_region, 0);

    // 8KB for AE_CONFIG module
    map(0x00040000, 0x00041fff).rom().region(m_eprom1_region, 0);

    // 8MB RAM, according to system info printed by the "mfree" command
    map(0x08000000, 0x087fffff).ram();

    // more alleged RAM accessed by the ROM code
    map(0x02200000, 0x029fffff).ram();

    // MC2681P DUART
    map(0x00060000, 0x0006001f).rw(m_duart, FUNC(mc68681_device::read), FUNC(mc68681_device::write));

    // NCR 5385 SCSI (register file @ byte offsets 0x0-0xf; OS-9 touches e.g. 0x00020009)
    map(0x00020000, 0x0002000f).rw(FUNC(vme_pme6822_card_device::ncr_port_r), FUNC(vme_pme6822_card_device::ncr_port_w));
}

void vme_pme6822_card_device::device_start()
{
	LOG("%s\n", FUNCNAME);

    save_item(NAME(m_ncr_reg6_cache));
    save_item(NAME(m_ncr_reg6_cache_valid));

    // memory tap offers a tidy solution for the "phantom" rtc
	m_maincpu->space(AS_PROGRAM).install_read_tap(0x00041000, 0x00041fff, "rtc",
		[this](offs_t offset, u32 &data, u32 mem_mask)
		{
			if (ACCESSING_BITS_24_31)
			{
				if (m_rtc->ceo_r())
					data = (data & 0x00ffffffU) | u32(m_rtc->read(offset >> 2)) << 24;
				else
					m_rtc->read(offset >> 2);
			}
		});
}

u8 vme_pme6822_card_device::ncr_port_r(offs_t offset)
{
    if (offset == 5)
    {
        if (!m_ncr_reg6_cache_valid)
        {
            m_ncr_reg6_cache = m_ncr->reg_r(6);
            m_ncr_reg6_cache_valid = true;
        }

        if (m_ncr_reg6_cache != 0)
        {
            return 0x20;
        } else {
            return m_ncr->reg_r(5);
        }
    }

    if (offset == 6)
    {
        if (!m_ncr_reg6_cache_valid)
        {
            m_ncr_reg6_cache = m_ncr->reg_r(6);
            m_ncr_reg6_cache_valid = true;
        }

        return m_ncr_reg6_cache;
    }

    return m_ncr->reg_r(offset);
}

void vme_pme6822_card_device::ncr_port_w(offs_t offset, u8 data)
{
    if (m_ncr_reg6_cache_valid && offset == 1 && (data & 0x0f) == 4)
    {
        m_ncr_reg6_cache_valid = false;
    }
    m_ncr->reg_w(offset, data);
}

void vme_pme6822_card_device::ncr_irq_w(int state)
{
    if (state) {
        m_ncr_reg6_cache_valid = false;
    }

    m_maincpu->set_input_line(M68K_IRQ_3, state ? ASSERT_LINE : CLEAR_LINE);
}

void vme_pme6822_card_device::duart_output(uint8_t data)
{
    LOG("DUART_OUTPUT: %02X '%c'\n", data, data);
}

void vme_pme6822_card_device::duart_a_tx(int state)
{
    m_duart_a_tx(state);
}