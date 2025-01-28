#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <memory>
#include <array>
#include <string_view>
#include <assert.h>

static void die(std::string_view msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg.data());
    abort();
}

static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;
        }

        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;
        }
        assert((size_t)rv <= n);
        n -=(size_t)rv;
        buf += rv;
    }
    return 0;
}


void do_something(int connfd) {
    std::array<char, 64> rbuf = {};
    ssize_t n = read(connfd, rbuf.data(), rbuf.size() - 1);
    if (n < 0) {
        die("read() error");
    }
    std::cerr << "client says: " << rbuf.data() << std::endl;

    std::string_view wbuf = "world!";
    write(connfd, wbuf.data(), wbuf.size());
}

const size_t k_max_msg = 4096;

static int32_t one_request(int connfd) {
    std::array<char, 4+k_max_msg> rbuf;
    errno = 0;
    int32_t err = read_full(connfd, rbuf.data(), 4);
    if (err) {
        die(errno == 0 ? "EOF" : "read() error");
        return err;
    }
    uint32_t len = 0;
    memcpy(&len, rbuf.data(), 4); // network byte order

    if (len > k_max_msg) {
        die("too long");
        return -1;
    }
    // body
    err = read_full(connfd, rbuf.data() + 4, len);
    if (err) {
        die("read() error");
        return err;
    }
    printf("Client says: %.*s\n", len , rbuf.data() + 4);

    const char reply[] = {'w', 'o', 'r', 'l', 'd', '!', '\0'};
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int rv = bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (rv) {
        die("bind()");
    }
    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }

    while (true) {
        sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, reinterpret_cast<sockaddr*>(&client_addr), &socklen);
        if (connfd < 0) {
            continue; // error
        }
        while (true) {
            int32_t err = one_request(connfd);
            if (err) {
                break;
            }
        }

        do_something(connfd);
        close(connfd);
    }
    return 0;
}
