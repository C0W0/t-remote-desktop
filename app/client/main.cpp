#include <iostream>

#include <winsock2.h>
#include <ws2tcpip.h>

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

int main() {
    WSADATA wsaData{};
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed with error : " << iResult << std::endl;
    }

    addrinfo* result = nullptr;
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo("100.109.181.42", DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET connectSocket{};
    for(; result != nullptr; result = result->ai_next) {
        // Create a SOCKET for connecting to server
        connectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (connectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %d\n", WSAGetLastError());
            freeaddrinfo(result);
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(connectSocket, result->ai_addr, static_cast<int>(result->ai_addrlen));
        if (iResult == SOCKET_ERROR) {
            closesocket(connectSocket);
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    const auto sendbuf = "Hello world from C++";

    // Send an initial buffer
    iResult = send(connectSocket, sendbuf, static_cast<int>(strlen(sendbuf)), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %d\n", iResult);

    // shutdown the connection for sending since no more data will be sent
    // the client can still use the ConnectSocket for receiving data
    iResult = shutdown(connectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    // Receive data until the server closes the connection
    do {
        char recvbuf[DEFAULT_BUFLEN];
        iResult = recv(connectSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (iResult > 0)
            printf("Bytes received: %d\n", iResult);
        else if (iResult == 0)
            printf("Connection closed\n");
        else
            printf("recv failed: %d\n", WSAGetLastError());
    } while (iResult > 0);

    // cleanup
    closesocket(connectSocket);
    WSACleanup();

    return 0;
}
