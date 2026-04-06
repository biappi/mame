// license:BSD-3-Clause
// copyright-holders:Enrico Gueli
#include "emu.h"
#include "pme6822.h"

#ifdef _MSC_VER
#define FUNCNAME __func__
#else
#define FUNCNAME __PRETTY_FUNCTION__
#endif

#define VERBOSE (LOG_GENERAL)
#include "logmacro.h"

DEFINE_DEVICE_TYPE(VME_PME6822,   vme_pme6822_card_device,   "pme6822",   "Radstone PME 68-22")

vme_pme6822_card_device::vme_pme6822_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
    : device_t(mconfig, VME_PME6822, tag, owner, clock)
    , device_vme_card_interface(mconfig, *this)
    , m_maincpu(*this, "maincpu")
    , m_duart(*this, "duart")
    , m_eprom0_region("eprom0")
    , m_eprom1_region("eprom1")
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
}

void vme_pme6822_card_device::main_map(address_map &map)
{
    // 64KB for OS-9 kernel ROM
    map(0x00000000, 0x0000ffff).rom().region(m_eprom0_region, 0);

    // 8KB for AE_CONFIG module
    map(0x00010000, 0x00011fff).rom().region(m_eprom1_region, 0);

    // 8MB RAM, according to system info printed by the "mfree" command
    map(0x08000000, 0x087fffff).ram();

    // MC2681P DUART
    map(0x00060000, 0x0006001f).rw(m_duart, FUNC(mc68681_device::read), FUNC(mc68681_device::write));
}

void vme_pme6822_card_device::device_start()
{
	LOG("%s\n", FUNCNAME);
}

void vme_pme6822_card_device::duart_output(uint8_t data)
{
    LOG("DUART_OUTPUT: %02X '%c'\n", data, data);
}
