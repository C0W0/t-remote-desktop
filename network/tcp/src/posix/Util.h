//
// Created by FNU Zhaluo on 9/30/26.
//

#ifndef TRDP_UTIL_H
#define TRDP_UTIL_H

#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>

namespace network {
// Returns 0 on success, otherwise errno.
inline int setNonBlockingCloexec(const int fd) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return errno;
    }
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        return errno;
    }
    return 0;
}

// Creates a non-blocking, close-on-exec pipe. Returns 0 on success, otherwise errno.
inline int makeWakePipe(int (&fds)[2]) {
    if (pipe(fds) == -1) {
        return errno;
    }
    for (const int fd : fds) {
        if (const int err = setNonBlockingCloexec(fd); err != 0) {
            ::close(fds[0]);
            ::close(fds[1]);
            fds[0] = fds[1] = -1;
            return err;
        }
    }
    return 0;
}
}

#endif //TRDP_UTIL_H
