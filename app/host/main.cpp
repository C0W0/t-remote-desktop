#include <iostream>
#include <string>
#include <memory>
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
    addrinfo *result = nullptr;

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo(nullptr, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        std::printf("Error at socket(): %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(listenSocket, result->ai_addr, static_cast<int>(result->ai_addrlen));
    if (iResult == SOCKET_ERROR) {
        std::printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::printf("Listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    sockaddr_in clientAddr{};
    int addrLen = sizeof(clientAddr);
    SOCKET clientSocket = accept(listenSocket, static_cast<sockaddr*>(static_cast<void*>(&clientAddr)), &addrLen);
    if (clientSocket == INVALID_SOCKET) {
        std::printf("accept failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    closesocket(listenSocket);

    std::string addrStr{INET_ADDRSTRLEN};
    addrStr.resize(INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &clientAddr.sin_addr, addrStr.data(), INET_ADDRSTRLEN);
    addrStr.resize(std::strlen(addrStr.data()));
    unsigned short clientPort = ntohs(clientAddr.sin_port);
    std::println(std::cout, "Accepting connection from {}:{}", addrStr, clientPort);

    char recvbuf[DEFAULT_BUFLEN];
    // Receive until the peer shuts down the connection
    do {
        int bytesRecv = recv(clientSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (bytesRecv > 0) {
            std::printf("Bytes received: %d\n", bytesRecv);

            // Echo the buffer back to the sender
            int iSendResult = send(clientSocket, recvbuf, bytesRecv, 0);
            if (iSendResult == SOCKET_ERROR) {
                std::printf("send failed: %d\n", WSAGetLastError());
                closesocket(clientSocket);
                WSACleanup();
                return 1;
            }
            std::printf("Bytes sent: %d\n", iSendResult);
        } else if (bytesRecv == 0)
            std::printf("Connection closing...\n");
        else {
            std::printf("recv failed: %d\n", WSAGetLastError());
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }
    } while (iResult > 0);


    // shutdown the send half of the connection since no more data will be sent
    iResult = shutdown(clientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        std::printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    // cleanup
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
