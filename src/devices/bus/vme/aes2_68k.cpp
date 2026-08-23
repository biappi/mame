#include "aes2_68k.h"

DEFINE_DEVICE_TYPE(VME_AESTHEDES2_68K, aesthedes2_vme_68k_device, "aesthedes2_68k", "Aesthedes2 68k (AES C100/0038)");

void aesthedes2_vme_68k_device::device_start()
{

}

void aesthedes2_vme_68k_device::device_add_mconfig(machine_config &config)
{
	M68000(config, m_cpu, 8_MHz_XTAL);
    m_cpu->set_addrmap(AS_PROGRAM, &aesthedes2_vme_68k_device::main_map);
}

void aesthedes2_vme_68k_device::set_rom(const char *rom_name)
{
    m_rom_name = rom_name;
}

void aesthedes2_vme_68k_device::main_map(address_map &map)
{
    map(0x00000000, 0x00000007).rom().region(m_rom_name, 0);
    map(0x00000008, 0x000fffff).ram();
    map(0x00800000, 0x0080ffff).rom().region(m_rom_name, 0);
}