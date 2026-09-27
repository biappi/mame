#include "aesthedes_keyboard.h"

#include "machine/keyboard.h"

#include "asio.h"

#define VERBOSE (1)
#include "logmacro.h"

#include <charconv>
#include <istream>

inline constexpr std::array<std::uint16_t, 128> keycodes = [] {
    std::array<std::uint16_t, 128> result{};

    result[0x09] = 1354; // TAB
    result[0x0d] = 554;  // ENTER

    result[0x30] = 585; // 0
    result[0x31] = 549; // 1
    result[0x32] = 581; // 2
    result[0x33] = 613; // 3
    result[0x34] = 548; // 4
    result[0x35] = 580; // 5
    result[0x36] = 612; // 6
    result[0x37] = 552; // 7
    result[0x38] = 584; // 8
    result[0x39] = 616; // 9

    result[0x30] = 585; // 0
    result[0x31] = 549; // 1
    result[0x32] = 581; // 2
    result[0x33] = 613; // 3
    result[0x34] = 548; // 4
    result[0x35] = 580; // 5
    result[0x36] = 612; // 6
    result[0x37] = 552; // 7
    result[0x38] = 584; // 8
    result[0x39] = 616; // 9

    result[0x61] = 1061; // a
    result[0x62] = 1222; // b
    result[0x63] = 1158; // c
    result[0x64] = 1125; // d
    result[0x65] = 1124; // e
    result[0x66] = 1157; // f
    result[0x67] = 1189; // g
    result[0x68] = 1221; // h
    result[0x69] = 1284; // i
    result[0x6a] = 1253; // j
    result[0x6b] = 1285; // k
    result[0x6c] = 1317; // l
    result[0x6d] = 1286; // m
    result[0x6e] = 1254; // n
    result[0x6f] = 1316; // o
    result[0x70] = 1348; // p
    result[0x71] = 1060; // q
    result[0x72] = 1156; // r
    result[0x73] = 1093; // s
    result[0x74] = 1188; // t
    result[0x75] = 1252; // u
    result[0x76] = 1190; // v
    result[0x77] = 1092; // w
    result[0x78] = 1126; // x
    result[0x79] = 1220; // y
    result[0x7a] = 1094; // z
    return result;
}();


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

void aesthedes_keyboard_device::device_add_mconfig(machine_config &config)
{
    auto &keyboard = GENERIC_KEYBOARD(config, "keyboard", 0);
    keyboard.set_keyboard_callback(FUNC(aesthedes_keyboard_device::keyboard_cb));
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

void aesthedes_keyboard_device::keyboard_cb(u8 keycode) {
    if (keycode < keycodes.size() && keycodes[keycode] != 0) {
        inject_keycode(keycodes[keycode]);
        LOG("keyboard_cb: 0x%02x -> %04d\n", keycode, keycodes[keycode]);
    } else {
        LOG("keyboard_cb: 0x%02x -> ignored\n", keycode);
    }
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
