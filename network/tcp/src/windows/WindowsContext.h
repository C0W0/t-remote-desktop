//
// Created by Terry on 2026-07-13.
//

#ifndef TRDP_WINDOWSCONTEXT_H
#define TRDP_WINDOWSCONTEXT_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdexcept>

class WinsockInitializer
{
public:
    static void Initialize() {
        static WinsockInitializer instance;
    }

    WinsockInitializer(const WinsockInitializer&) = delete;
    WinsockInitializer& operator=(const WinsockInitializer&) = delete;
private:
    WinsockInitializer()
    {
        WSADATA data{};
        int result = WSAStartup(MAKEWORD(2, 2), &data);

        if (result != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
    }

    ~WinsockInitializer()
    {
        WSACleanup();
    }
};

#endif //TRDP_WINDOWSCONTEXT_H
