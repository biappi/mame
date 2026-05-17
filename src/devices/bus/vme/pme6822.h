// license:BSD-3-Clause
// copyright-holders:Enrico Gueli
#ifndef MAME_BUS_VME_PME6822_H
#define MAME_BUS_VME_PME6822_H


#pragma once

#include "emu.h"

#include "bus/vme/vme.h"
#include "cpu/m68000/m68020.h"
#include "machine/mc68681.h"
#include "machine/ds1215.h"
#include "machine/ncr5385.h"
#include "machine/nscsi_bus.h"

DECLARE_DEVICE_TYPE(VME_PME6822, vme_pme6822_card_device)

class vme_pme6822_card_device : public device_t, public device_vme_card_interface
{
public:
    vme_pme6822_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

    void set_eprom_regions(const char *eprom0, const char *eprom1);

    auto rs232_tx_cb() { return m_duart_a_tx.bind(); }
    
protected:
    // device_t overrides
    virtual void device_start() override ATTR_COLD;
    virtual void device_reset() override ATTR_COLD;

	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

private:
	required_device<m68000_musashi_device> m_maincpu;
    required_device<mc68681_device> m_duart;
	required_device<ds1216e_device> m_rtc;
	required_device<ncr5385_device> m_ncr;

    devcb_write_line m_duart_a_tx;

    const char *m_eprom0_region;
    const char *m_eprom1_region;

    uint8_t m_ncr_reg6_cache;
    attotime m_ncr_reg6_cached_at;

    void main_map(address_map &map) ATTR_COLD;

    u8 ncr_port_r(offs_t offset);
    void ncr_port_w(offs_t offset, u8 data);
    bool ncr_reg6_cache_valid() const;

    void duart_output(uint8_t data);
    void duart_a_tx(int state);
};

#endif // MAME_BUS_VME_PME6822_H
