using namespace std;
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <array>
#include <assert.h>

const size_t k_max_msg = 4096;

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

static int32_t query(int fd, const char *text) {
    uint32_t len = (uint32_t)strlen(text);
    if (len > k_max_msg) {
        return -1;
    }
    // send
    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], text, len);
    if (int32_t err = write_all(fd, wbuf, 4 + len)) {
        return err;
    }
    // 4 byte header
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        die(errno == 0 ? "EOF" : "read() error");
        return err;
    }
    memcpy(&len, rbuf, 4);
    if (len > k_max_msg) {
        die("too long");
        return -1;
    }
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        die("read() error");
        return err;
    }
    printf("server says: %.*s\n", len, &rbuf[4]);
    return 0;
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
    int rv = connect(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if (rv) {
        die("connect()");
    }

    int32_t err = query(fd, "hello1");
    if (err) {
        close(fd);
        return 0;
    }

    err = query(fd, "hello2");
    if (err) {
        close(fd);
        return 0;
    }
    close(fd);
    return 0;
}
