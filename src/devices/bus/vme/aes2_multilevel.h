#include "emu.h"

#include "bus/vme/vme.h"
#include "bus/vme/vme_cards.h"

DECLARE_DEVICE_TYPE(VME_AESTHEDES2_MULTILEVEL_GPU, aesthedes2_multilevel_gpu_device);

class aesthedes2_multilevel_gpu_device : public device_t, public device_vme_card_interface
{
public:
    aesthedes2_multilevel_gpu_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
		: device_t(mconfig, VME_AESTHEDES2_MULTILEVEL_GPU, tag, owner, clock)
		, device_vme_card_interface(mconfig, *this)
    {
    }

protected:
    virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;

private:
	u32 read32(address_space &space, offs_t offset, u32 mem_mask);
	void write32(address_space &space, offs_t offset, u32 data, u32 mem_mask);
};
