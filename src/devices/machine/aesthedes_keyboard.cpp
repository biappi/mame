#include "aesthedes_keyboard.h"

#define VERBOSE (1)
#include "logmacro.h"

DEFINE_DEVICE_TYPE(AES2_KEYBOARD, aesthedes_keyboard_device, "aes2_keyboard", "Aesthedes 2 Keyboard")

aesthedes_keyboard_device::aesthedes_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, AES2_KEYBOARD, tag, owner, clock)
    , m_out_a_strobe_func(*this)
    , m_out_a_port_func(*this)
{
}

void aesthedes_keyboard_device::device_start()
{
    m_fake_keystrokes_timer = timer_alloc(FUNC(aesthedes_keyboard_device::fake_keystrokes), this);
}

void aesthedes_keyboard_device::device_reset()
{
    m_out_a_port_func(0xff);
    m_out_a_strobe_func(1);
    m_fake_keystrokes_timer->adjust(attotime::from_seconds(29), 0, attotime::from_hz(1));
}

TIMER_CALLBACK_MEMBER(aesthedes_keyboard_device::fake_keystrokes)
{
    static u8 n;
    m_out_a_port_func(n++);
    m_out_a_strobe_func(0);
    m_out_a_strobe_func(1);
}