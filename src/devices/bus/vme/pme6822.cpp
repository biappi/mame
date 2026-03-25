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
{
}

void vme_pme6822_card_device::device_add_mconfig(machine_config &config)
{
	M68020(config, m_maincpu, 16670000);
    m_maincpu->set_addrmap(AS_PROGRAM, &vme_pme6822_card_device::main_map);
}

void vme_pme6822_card_device::main_map(address_map &map)
{
    // 8MB RAM, according to system info printed by the "mfree" command
    map(0x00000000, 0x007fffff).ram();

    // 64KB for OS-9 kernel ROM
    map(0xf0000000, 0xf000ffff).rom().region("eprom0", 0);

    // 8KB for AE_CONFIG module
    map(0xf0010000, 0xf0011fff).rom().region("eprom1", 0);
}

void vme_pme6822_card_device::device_start()
{
	LOG("%s\n", FUNCNAME);
}