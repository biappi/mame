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
#include <queue>

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

    attotime m_ncr_int_cached_at;
    int m_ncr_int_state;

    bool m_ncr_waits_fifo_before_read;
    bool m_ncr_waits_fifo_before_write;
    std::vector<u8> m_ncr_dma_buffer;
    size_t m_ncr_dma_read_head;
    size_t m_ncr_dma_write_head;
    size_t m_ncr_dma_size;

    void main_map(address_map &map) ATTR_COLD;

    u8 ncr_port_r(offs_t offset);
    void ncr_port_w(offs_t offset, u8 data);
    void ncr_irq_w(int state);
    bool ncr_int_cache_valid() const;

    u8 ncr_dma_scratchpad_r(offs_t offset);
    void ncr_dma_scratchpad_w(offs_t offset, u8 data);
    void ncr_dreq(int state);

    void duart_output(uint8_t data);
    void duart_a_tx(int state);
};

#endif // MAME_BUS_VME_PME6822_H
