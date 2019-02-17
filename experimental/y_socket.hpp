#pragma once
//======================================================================

#include "y_basics.hpp"

//======================================================================

#if defined(Y_FEATURE_OS_WIN)
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #include <ws2tcpip.h>
    #include <WinSock2.h>
    #include <Windows.h>
    #define Y_SOCKET_ERROR__WOULD_BLOCK     WSAEWOULDBLOCK
    #if Y_FEATURE_COMPILER_MSVC
        #pragma comment (lib, "ws2_32")
    #endif
#elif defined(Y_FEATURE_OS_POSIX)
    #define <sys/socket.h>
    #define <netinet/in.h>
    typedef size_t SOCKET;
    #define INVALID_SOCKET                  (~0)
    #define closesocket(s)                  close(s)
    #define Y_SOCKET_ERROR__WOULD_BLOCK     EWOULDBLOCK     // ?
#else
    #error "What is this?!"
#endif

//======================================================================

namespace y {
    namespace Socket {

//======================================================================

#if defined(Y_FEATURE_OS_WIN)
    inline int Initialize () {::WSAData data = {}; return ::WSAStartup(MAKEWORD(2, 2), &data);}
    inline int Cleanup () {return ::WSACleanup();}
    inline int GetTheError () {return ::WSAGetLastError();}
    inline bool SetBlockingMode (SOCKET sock, bool should_be_blocking) {
        u_long arg = (should_be_blocking ? 0 : 1);
        return 0 == ::ioctlsocket(sock, FIONBIO, &arg);
    }
#else
    inline int Initialize () {return 0;}
    inline int CleanUp () {return 0;}
    inline int GetTheError () {return errno;}  // FIXME(yzt): Is this correct?
    inline bool SetBlockingMode (SOCKET sock, bool should_be_blocking) {
        int nonBlocking = (should_be_blocking ? 0 : 1);
        return -1 != ::fcntl(sock, F_SETFL, O_NONBLOCK, nonBlocking);
    }
#endif

//======================================================================

enum class Error {
    None = 0,

    Unknown,
    Other,
    Internal,

    BadParam,
    NoMem,
    BufferTooSmall,
    SocketCreate,
    SocketSetOptions,
    SocketBind,
};

//----------------------------------------------------------------------

struct Exception : y::Exception {
    Exception (Error err, char const * msg) : y::Exception(msg), error (err), platform_error_code (GetTheError()) {}

    Error error;
    int platform_error_code;
};

//======================================================================

class LibInitializer {
public:
    LibInitializer () {Initialize();}
    ~LibInitializer () {Cleanup();}
};

//======================================================================

struct Address {
    static Address IPv4 (U32 ip, U16 port) {
        Address ret = {};
        auto in = ret.asIPv4();
        in->sin_family = AF_INET;
        in->sin_port = ::htons(port);
        in->sin_addr.s_addr = ::htonl(ip);
        return ret;
    }
    static Address IPv4 (char const * dotted_ip, U16 port) {
        Address ret = {};
        auto in = ret.asIPv4();
        in->sin_family = AF_INET;
        in->sin_port = ::htons(port);
        in->sin_addr.s_addr = ::inet_addr(dotted_ip);   // NOTE(yzt): So sue me!
        return ret;
    }
    //static Address IPv4FromHostName (char const * host_name, U16 port) {
    //    Address ret = {};
    //    auto in = ret.asIPv4();
    //    in->sin_family = AF_INET;
    //    in->sin_port = ::htons(port);
    //    ::getaddrinfo(...)
    //    in->sin_addr.s_addr = ...;
    //    return ret;
    //}
    static Address IPv4Any (U16 port) {
        Address ret = {};
        auto in = ret.asIPv4();
        in->sin_family = AF_INET;
        in->sin_port = ::htons(port);
        in->sin_addr.s_addr = ::htonl(INADDR_ANY);
        return ret;
    }
    static Address IPv4Loopback (U16 port) {
        Address ret = {};
        auto in = ret.asIPv4();
        in->sin_family = AF_INET;
        in->sin_port = ::htons(port);
        in->sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
        return ret;
    }

    int len () const {return int(sizeof(m_storage));}
    sockaddr const * ptr () const {return reinterpret_cast<sockaddr const *>(&m_storage);}
    sockaddr * ptr () {return reinterpret_cast<sockaddr *>(&m_storage);}

    sockaddr_in const * asIPv4 () const {return reinterpret_cast<sockaddr_in const *>(&m_storage);}
    sockaddr_in * asIPv4 () {return reinterpret_cast<sockaddr_in *>(&m_storage);}
    sockaddr_in6 const * asIPv6 () const {return reinterpret_cast<sockaddr_in6 const *>(&m_storage);}
    sockaddr_in6 * asIPv6 () {return reinterpret_cast<sockaddr_in6 *>(&m_storage);}

    ADDRESS_FAMILY family () const {return m_storage.ss_family;}
    U16 port () const {return ::ntohs(asIPv4()->sin_port);}
    U32 ipv4 () const {return ::ntohl(asIPv4()->sin_addr.s_addr);}
    char const * ipv4str () const {return ::inet_ntoa(asIPv4()->sin_addr);}

    friend bool operator == (Address const & a, Address const & b) {
        Y_ASSERT(a.family() == AF_INET && b.family() == AF_INET);
        return a.family() == b.family() && a.ipv4() == b.ipv4() && a.port() == b.port();
    }
private:
    sockaddr_storage m_storage;
};

//======================================================================

class UDP {
public:
    struct Config {
        Address bind_address;
        U32 recv_buffer_size;
    };

    enum class SendResult {
        Success,
        TryAgainLater,
        Failure,
    };

public:
    explicit UDP (Config const & cfg)
        : m_sock (INVALID_SOCKET)
        , m_recv_buf ()
        , m_has_received_msg (false)
        , m_cfg (cfg)
    {
        m_sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (INVALID_SOCKET == m_sock)
            throw Exception{Error::SocketCreate, "socket() failed."};

        if (!SetBlockingMode(m_sock, false))
            throw Exception{Error::SocketSetOptions, "Failed to set socket to non-blocking mode."};

        /* Set socket options. Most of these are only checked/take effect after bind(). */
        constexpr ::LINGER linger = {};   // disable linger
        ::setsockopt(m_sock, SOL_SOCKET, SO_LINGER, (char const *)&linger, sizeof(linger));

        //::setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (char const *)&m_cfg.os_recv_buff_size, sizeof(m_cfg.os_recv_buff_size));
        //::setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (char const *)&m_cfg.os_send_buff_size, sizeof(m_cfg.os_send_buff_size));

        constexpr int should_reuse_addr = 0;
        ::setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char const *)&should_reuse_addr, sizeof(should_reuse_addr));

        constexpr int exclusive_addr_use = 1;
        ::setsockopt(m_sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char const *)&exclusive_addr_use, sizeof(exclusive_addr_use));

        if (0 != ::bind(m_sock, m_cfg.bind_address.ptr(), m_cfg.bind_address.len()))
            throw Exception{Error::SocketBind, "bind() failed."};

        m_recv_buf.ptr = reinterpret_cast<byte *>(::malloc(m_cfg.recv_buffer_size));
        m_recv_buf.end = m_recv_buf.ptr;
        if (!Y_PTR_VALID(m_recv_buf.ptr))
            throw Exception{Error::NoMem, "Couldn't allocate memory for recv buffer."};
        m_recv_buf.cap = m_recv_buf.ptr + m_cfg.recv_buffer_size;
    }

    ~UDP () noexcept {
        ::closesocket(m_sock);
        m_sock = INVALID_SOCKET;

        ::free(m_recv_buf.ptr);
        m_recv_buf = {};
    }

    Address getBoundAddr () const {
        Address ret = {};
        int addr_len = ret.len();
        ::getsockname(m_sock, ret.ptr(), &addr_len);
        return ret;
    }

    bool recvNext () {
        m_has_received_msg = false;
        m_recv_buf.end = m_recv_buf.ptr;
        m_last_error = 0;

        int peer_addr_len = m_recv_peer.len();
        int r = ::recvfrom(m_sock,
            (char *)m_recv_buf.ptr, int(m_recv_buf.capacity()), 0,
            m_recv_peer.ptr(), &peer_addr_len
        );

        if (r < 0) {
            Y_ASSERT(SOCKET_ERROR == r);
            auto err = GetTheError();
            if (Y_SOCKET_ERROR__WOULD_BLOCK != err)
                m_last_error = err;
            return false;
        } else {    // Actually received some data...
            m_has_received_msg = true;
            m_recv_buf.end = m_recv_buf.ptr + unsigned(r);
            return true;
        }
    }

    bool hasMsg () const {
        return m_has_received_msg;
    }

    Buffer const & getCurrMsg () const {
        return m_recv_buf;
    }
    Address const & getCurrSender () const {
        return m_recv_peer;
    }

    SendResult send (Address const & dest, Buffer const & msg) {
        m_last_error = 0;

        int r = ::sendto(m_sock, (char const *)msg.ptr, int(msg.size()), 0, dest.ptr(), dest.len());

        if (r == int(msg.size())) {
            return SendResult::Success;
        } else if (r < 0) {
            Y_ASSERT(r == SOCKET_ERROR);
            auto err = GetTheError();
            if (err == Y_SOCKET_ERROR__WOULD_BLOCK) {
                return SendResult::TryAgainLater;
            } else {
                m_last_error = err;
                return SendResult::Failure;
            }
        } else {
            m_last_error = -10;
            return SendResult::Failure;
        }
    }

private:
    SOCKET m_sock;
    Buffer m_recv_buf;
    int m_last_error;
    bool m_has_received_msg;
    Address m_recv_peer;
    Config const m_cfg;
};

//======================================================================

    }   // namespace Socket
}   // namespace y

//======================================================================
//======================================================================
