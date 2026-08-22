#include "aesthedes_keyboard.h"

#include "asio.h"

#define VERBOSE (1)
#include "logmacro.h"

#include <charconv>
#include <istream>

DEFINE_DEVICE_TYPE(AES2_KEYBOARD, aesthedes_keyboard_device, "aes2_keyboard", "Aesthedes 2 Keyboard")

class aesthedes_keyboard_server
{
public:
    aesthedes_keyboard_server(aesthedes_keyboard_device &keyboard)
        : m_keyboard(keyboard)
        , m_acceptor(m_io_context, asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 6822))
    {
        accept_client();
        m_thread = std::thread([this] { m_io_context.run(); });
    }

    ~aesthedes_keyboard_server()
    {
        m_io_context.stop();
        if (m_thread.joinable())
            m_thread.join();
    }

private:
    void accept_client()
    {
        m_acceptor.async_accept([this](std::error_code error, asio::ip::tcp::socket socket)
        {
            if (!error)
            {
                auto client = std::make_shared<client_session>(m_keyboard, std::move(socket));
                client->start();
            }

            if (!m_io_context.stopped())
                accept_client();
        });
    }

    class client_session : public std::enable_shared_from_this<client_session>
    {
    public:
        client_session(aesthedes_keyboard_device &keyboard, asio::ip::tcp::socket socket)
            : m_keyboard(keyboard), m_socket(std::move(socket))
        {
        }

        void start()
        {
            static constexpr char greeting[] = "Aesthedes 2 emulator - keycodes injector\n";
            auto self = shared_from_this();
            asio::async_write(m_socket, asio::buffer(greeting, sizeof(greeting) - 1),
                [self](std::error_code error, std::size_t)
                {
                    if (!error)
                        self->read_line();
                });
        }

    private:
        void read_line()
        {
            auto self = shared_from_this();
            asio::async_read_until(m_socket, m_input, '\n',
                [self](std::error_code error, std::size_t)
                {
                    if (error)
                        return;

                    std::string line;
                    std::istream input_stream(&self->m_input);
                    std::getline(input_stream, line);
                    if (!line.empty() && line.back() == '\r')
                        line.pop_back();

                    uint32_t value = 0;
                    const auto result = std::from_chars(line.data(), line.data() + line.size(), value);
                    const bool valid = result.ec == std::errc() && result.ptr == line.data() + line.size() && value <= 0xffff;
                    if (valid)
                    {
                        self->m_keyboard.machine().scheduler().synchronize(
                            timer_expired_delegate(FUNC(aesthedes_keyboard_device::inject_keycode), &self->m_keyboard), value);
                    }

                    self->write_response(valid ? "OK\n" : "NOPE\n");
                });
        }

        void write_response(const char *response)
        {
            auto self = shared_from_this();
            asio::async_write(m_socket, asio::buffer(response, std::strlen(response)),
                [self](std::error_code error, std::size_t)
                {
                    if (!error)
                        self->read_line();
                });
        }

        aesthedes_keyboard_device &m_keyboard;
        asio::ip::tcp::socket m_socket;
        asio::streambuf m_input;
    };

    aesthedes_keyboard_device &m_keyboard;
    asio::io_context m_io_context;
    asio::ip::tcp::acceptor m_acceptor;
    std::thread m_thread;
};

aesthedes_keyboard_device::~aesthedes_keyboard_device() = default;

aesthedes_keyboard_device::aesthedes_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, AES2_KEYBOARD, tag, owner, clock)
    , m_out_a_strobe_func(*this)
    , m_out_a_port_func(*this)
    , m_out_b_port_func(*this)
{
}

void aesthedes_keyboard_device::device_start()
{
	m_server = std::make_unique<aesthedes_keyboard_server>(*this);
}

void aesthedes_keyboard_device::device_reset()
{
    m_out_a_port_func(0x0f);
    m_out_b_port_func(0xff);
    m_out_a_strobe_func(1);
}

void aesthedes_keyboard_device::device_stop()
{
    m_server.reset();
}

void aesthedes_keyboard_device::inject_keycode(int keycode)
{
    LOG("sending keycode %d\n", keycode);
    m_out_a_port_func((u8)(keycode >> 8));
    m_out_b_port_func((u8)(keycode & 0xff));
    m_out_a_strobe_func(0);
    m_out_a_strobe_func(1);
}

void aesthedes_keyboard_device::leds_pa_w(u8 data) {
    m_leds_latch = (m_leds_latch & 0x00ff) | (data << 8);
}

void aesthedes_keyboard_device::leds_pb_w(u8 data) {
    m_leds_latch = (m_leds_latch & 0xff00) | (data);
}

void aesthedes_keyboard_device::leds_x1_w(int state) {
    if (state == 0) {
        LOG("LEDs: %04x\n", m_leds_latch);
    }
}
