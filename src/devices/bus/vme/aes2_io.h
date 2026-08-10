#include "emu.h"

#include "bus/vme/vme.h"
#include "bus/vme/vme_cards.h"
#include "machine/6821pia.h"

DECLARE_DEVICE_TYPE(VME_AESTHEDES2_IO, aesthedes2_vme_io_device);

class aesthedes2_vme_io_device : public device_t, public device_vme_card_interface
{
public:
    aesthedes2_vme_io_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
        : device_t(mconfig, VME_AESTHEDES2_IO, tag, owner, clock)
        , device_vme_card_interface(mconfig, *this)
        , m_base_addr(0)
        , m_pia_a(*this, "pia_a")
        , m_pia_b(*this, "pia_b")
        , m_pia_c(*this, "pia_c")
        , m_connector_a(*this)
        , m_connector_b(*this)
        , m_connector_c(*this)
    {
    }

    class parallel_connector {
        public:        
            parallel_connector(device_t &owner)
            : m_porta(owner), m_portb(owner)
            , m_x1(owner), m_x2(owner)
            , m_x19(owner), m_x20(owner)
            {};
        
            auto porta_cb() { return m_porta.bind(); }
            auto portb_cb() { return m_portb.bind(); }
            auto x1_cb() { return m_x1.bind(); }
            auto x2_cb() { return m_x2.bind(); }
            auto x19_cb() { return m_x19.bind(); }
            auto x20_cb() { return m_x20.bind(); }

        private:
            devcb_write8 m_porta;
            devcb_write8 m_portb;
            devcb_write_line m_x1;
            devcb_write_line m_x2;
            devcb_write_line m_x19;
            devcb_write_line m_x20;

            friend class aesthedes2_vme_io_device;
    };

    void set_base_address(offs_t addr)
    {
        if (m_base_addr != 0)
            fatalerror("Attempting to set base address twice");
        
        m_base_addr = addr;
    }

    auto &connector_a() { return m_connector_a; }
    auto &connector_b() { return m_connector_b; }
    void connector_a_pa_w(u8 data) { m_pia_a->porta_w(data); }
    void connector_a_pb_w(u8 data) { m_pia_a->portb_w(data); }
    void connector_a_x1_w(int state) { m_pia_a->cb2_w(state); }
    void connector_a_x2_w(int state) { m_pia_a->ca1_w(state); }
    void connector_a_x19_w(int state) { m_pia_a->cb1_w(state); }
    void connector_a_x20_w(int state) { m_pia_a->ca2_w(state); }
    void connector_b_pa_w(u8 data) { m_pia_b->porta_w(data); }
    void connector_b_pb_w(u8 data) { m_pia_b->portb_w(data); }
    void connector_b_x1_w(int state) { m_pia_b->cb2_w(state); }
    void connector_b_x2_w(int state) { m_pia_b->ca1_w(state); }
    void connector_b_x19_w(int state) { m_pia_b->cb1_w(state); }
    void connector_b_x20_w(int state) { m_pia_b->ca2_w(state); }
    void connector_c_pa_w(u8 data) { m_pia_c->porta_w(data); }
    void connector_c_pb_w(u8 data) { m_pia_c->portb_w(data); }
    void connector_c_x1_w(int state) { m_pia_c->cb2_w(state); }
    void connector_c_x2_w(int state) { m_pia_c->ca1_w(state); }
    void connector_c_x19_w(int state) { m_pia_c->cb1_w(state); }
    void connector_c_x20_w(int state) { m_pia_c->ca2_w(state); }

protected:
    virtual void device_start() override ATTR_COLD;
    virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

private:
    offs_t m_base_addr;
    required_device<pia6821_device> m_pia_a;
    required_device<pia6821_device> m_pia_b;
    required_device<pia6821_device> m_pia_c;

    parallel_connector m_connector_a;
    parallel_connector m_connector_b;
    parallel_connector m_connector_c;

    u32 read32(address_space &space, offs_t offset, u32 mem_mask);
    void write32(address_space &space, offs_t offset, u32 data, u32 mem_mask);
};