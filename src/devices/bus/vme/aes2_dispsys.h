#include "emu.h"

#include "bus/vme/vme.h"
#include "bus/vme/vme_cards.h"

DECLARE_DEVICE_TYPE(VME_AES2_DISPSYS, aesthedes2_vme_dispsys_device);

class aesthedes2_vme_dispsys_device : public device_t, public device_vme_card_interface
{
public:
    aesthedes2_vme_dispsys_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
        : device_t(mconfig, VME_AES2_DISPSYS, tag, owner, clock)
        , device_vme_card_interface(mconfig, *this)
        , m_base_addr(0)
    {
    }

    void set_base_address(offs_t addr)
    {
        if (m_base_addr != 0)
            fatalerror("Attempting to set base address twice");
        
        m_base_addr = addr;
    }

    protected:
    virtual void device_start() override ATTR_COLD;
    virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

private:
    offs_t m_base_addr;

    u32 read32(address_space &space, offs_t offset, u32 mem_mask);
    void write32(address_space &space, offs_t offset, u32 data, u32 mem_mask);
};