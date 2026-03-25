// license:BSD-3-Clause
// copyright-holders:Enrico Gueli
#ifndef MAME_BUS_VME_PME6822_H
#define MAME_BUS_VME_PME6822_H


#pragma once

#include "emu.h"

#include "bus/vme/vme.h"
#include "cpu/m68000/m68020.h"

DECLARE_DEVICE_TYPE(VME_PME6822, vme_pme6822_card_device)

class vme_pme6822_card_device : public device_t, public device_vme_card_interface
{
public:
    vme_pme6822_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
    // device_t overrides
    virtual void device_start() override ATTR_COLD;

	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

private:
	required_device<m68000_musashi_device> m_maincpu;

    void main_map(address_map &map) ATTR_COLD;
};

#endif // MAME_BUS_VME_PME6822_H
