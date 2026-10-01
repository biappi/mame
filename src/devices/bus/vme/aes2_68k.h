// license:BSD-3-Clause
// copyright-holders:Enrico Gueli

/*
 * Aesthedes 68000 CPU backplane card.
 * https://github.com/egueli/Aesthedes-notes/blob/main/hardware/cards/aes_68k/README.md
 * 
 */
#include "emu.h"

#include "bus/vme/vme.h"
#include "bus/vme/vme_cards.h"
#include "cpu/m68000/m68000.h"
#include "machine/6821pia.h"

DECLARE_DEVICE_TYPE(VME_AESTHEDES2_68K, aesthedes2_vme_68k_device);

// Class name contains "vme" but the actual backplane is likely Gespac G64.
class aesthedes2_vme_68k_device : public device_t, public device_vme_card_interface
{
public:
    aesthedes2_vme_68k_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
        : device_t(mconfig, VME_AESTHEDES2_68K, tag, owner, clock)
        , device_vme_card_interface(mconfig, *this)
        , m_cpu(*this, "cpu")
        , m_pia_a(*this, "pia_a")
        , m_pia_b(*this, "pia_b")
        , m_pia_c(*this, "pia_c")
        , m_pia_d(*this, "pia_d")
        , m_connector_a_porta(*this)
        , m_connector_a_portb(*this)
        , m_connector_a_x1(*this)
        , m_connector_a_x2(*this)
        , m_connector_a_x19(*this)
        , m_connector_a_x20(*this)
        , m_cpu_irq6(*this)
    {
    }

    void set_rom(const char *rom_name);

    auto connector_a_porta_cb() { return m_connector_a_porta.bind(); };
    auto connector_a_portb_cb() { return m_connector_a_portb.bind(); };
    void connector_a_porta_w(u8 data) { m_pia_a->porta_w(data); } 
    void connector_a_portb_w(u8 data) { m_pia_a->portb_w(data); } 
    void connector_a_x1_w(int state) { m_pia_a->cb2_w(state); }
    void connector_a_x2_w(int state) { m_pia_a->ca1_w(state); }
    void connector_a_x19_w(int state) { m_pia_a->cb1_w(state); }
    void connector_a_x20_w(int state) { m_pia_a->ca2_w(state); }
    auto connector_a_x1_cb() { return m_connector_a_x1.bind(); }
    auto connector_a_x2_cb() { return m_connector_a_x2.bind(); }
    auto connector_a_x19_cb() { return m_connector_a_x19.bind(); }
    auto connector_a_x20_cb() { return m_connector_a_x20.bind(); }

protected:
    // device_t overrides
    virtual void device_start() override ATTR_COLD;
    virtual void device_reset() override ATTR_COLD;

    virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

private:
	required_device<m68000_device> m_cpu;
    required_device<pia6821_device> m_pia_a;
    required_device<pia6821_device> m_pia_b;
    required_device<pia6821_device> m_pia_c;
    required_device<pia6821_device> m_pia_d;

    devcb_write8 m_connector_a_porta;
    devcb_write8 m_connector_a_portb;
    devcb_write_line m_connector_a_x1;
    devcb_write_line m_connector_a_x2;
    devcb_write_line m_connector_a_x19;
    devcb_write_line m_connector_a_x20;

    void main_map(address_map &map) ATTR_COLD;

    const char *m_rom_name;

    u16 g64_ext_r(offs_t offset, u16 mem_mask);
    void g64_ext_w(offs_t offset, u16 data, u16 mem_mask);

    TIMER_CALLBACK_MEMBER(fake_irq6_timer);
    TIMER_CALLBACK_MEMBER(clear_irq6_timer);
    emu_timer *m_fake_irq6_timer;
    emu_timer *m_clear_irq6_timer;
    devcb_write_line m_cpu_irq6;
};
