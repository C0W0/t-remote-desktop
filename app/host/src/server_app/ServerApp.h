//
// Created by Terry Zha on 10/17/26.
//

#ifndef TRDP_SERVERAPP_H
#define TRDP_SERVERAPP_H

#include <thread>

#include "tcp/Server.h"
#include "capture/Capturer.h"

namespace ServerApp {
std::jthread run(
    network::ArcConnectionSocket& connectionSocket,
    capture::FrameQueue& frameQueue,
    std::function<void()> abortCommand
);
}


#endif //TRDP_SERVERAPP_H
