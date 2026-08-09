#include "aesthedes_bitpad.h"

#define VERBOSE (1)
#include "logmacro.h"

DEFINE_DEVICE_TYPE(AES2_BITPAD, aesthedes_bitpad_device, "aes2_bitpad", "Aesthedes 2 Bitpad")

#define DO_FAKE_MOVEMENT 0

aesthedes_bitpad_device::aesthedes_bitpad_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, AES2_BITPAD, tag, owner, clock)
    , m_out_a_strobe_func(*this)
    , m_out_a_port_func(*this)
{
}

void aesthedes_bitpad_device::device_start()
{
    m_fake_movement_timer = timer_alloc(FUNC(aesthedes_bitpad_device::fake_movement), this);
}

void aesthedes_bitpad_device::device_reset()
{
    m_out_a_port_func(0xff);
    m_out_a_strobe_func(1);
#if DO_FAKE_MOVEMENT
    m_fake_movement_timer->adjust(attotime::from_msec(28800), 0, attotime::from_hz(1000));
#endif
    m_fake_movement_count = 0;
}

TIMER_CALLBACK_MEMBER(aesthedes_bitpad_device::fake_movement)
{
    m_out_a_port_func(0xaa);
    m_out_a_strobe_func(0);
    m_out_a_strobe_func(1);

    m_fake_movement_count--;
    if (m_fake_movement_count >= 20) {
        m_fake_movement_timer->enable(false);
    }
}