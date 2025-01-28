using namespace std;
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <array>

static void die(std::string_view msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg.data());
    abort();
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

    string_view msg = "hello!";
    write(fd, msg.data(), msg.size());

    array<char, 64> rbuf = {};
    ssize_t n = read(fd, rbuf.data(), rbuf.size() - 1);
    if (n < 0) {
        die("read()");
    }
    cout << "server says: " << rbuf.data() << endl;
    close(fd);
    return 0;
}
