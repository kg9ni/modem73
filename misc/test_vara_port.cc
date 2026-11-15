// Smoke test for VaraPort host semantics (no RF).
#include "vara_port.hh"
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

static int connect_port(int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons((uint16_t)port);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
    if (connect(s, (sockaddr*)&a, sizeof(a)) < 0) {
        close(s);
        return -1;
    }
    return s;
}

static std::string read_line(int s, int timeout_ms = 2000) {
    std::string out;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline) {
        char c;
        ssize_t n = recv(s, &c, 1, MSG_DONTWAIT);
        if (n == 1) {
            if (c == '\r') return out;
            if (c != '\n') out.push_back(c);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    return out;
}

static void send_all(int s, const char* msg) {
    send(s, msg, strlen(msg), 0);
}

int main() {
    std::vector<std::vector<uint8_t>> txq;
    VaraPort::Hooks h;
    h.queue_tx = [&](const std::vector<uint8_t>& d) { txq.push_back(d); };
    h.is_ptt = [] { return false; };
    h.is_busy = [] { return false; };
    h.payload_capacity = [] { return (size_t)256; };
    h.set_drive = [](float) {};
    h.get_drive = [] { return 0.562341f; }; // ~-5 dB

    // Bind ephemeral: find free even port
    int port = 18300;
    for (; port < 18400; port += 2) {
        VaraPort probe(port, "127.0.0.1", 3, h);
        if (probe.start()) {
            probe.stop();
            break;
        }
    }
    if (port >= 18400) {
        fprintf(stderr, "FAIL: no free ports\n");
        return 1;
    }

    VaraPort v(port, "127.0.0.1", 3, h);
    if (!v.start()) {
        fprintf(stderr, "FAIL: start\n");
        return 1;
    }

    int cmd = connect_port(port);
    int data = connect_port(port + 1);
    if (cmd < 0 || data < 0) {
        fprintf(stderr, "FAIL: connect cmd/data\n");
        return 1;
    }
    // BUSY toggle on attach
    auto busy = read_line(cmd);
    if (busy != "BUSY ON" && busy != "BUSY OFF") {
        fprintf(stderr, "FAIL: expected BUSY, got '%s'\n", busy.c_str());
        return 1;
    }

    send_all(cmd, "VERSION\r");
    auto ver = read_line(cmd);
    if (ver != "VERSION VARA HF v4.9.0") {
        fprintf(stderr, "FAIL: VERSION '%s'\n", ver.c_str());
        return 1;
    }

    send_all(cmd, "MYCALL N0CALL\r");
    if (read_line(cmd) != "OK") {
        fprintf(stderr, "FAIL: MYCALL\n");
        return 1;
    }

    send_all(cmd, "RETRIES 2\r");
    if (read_line(cmd) != "OK") {
        fprintf(stderr, "FAIL: RETRIES\n");
        return 1;
    }
    if (v.retries() != 2) {
        fprintf(stderr, "FAIL: retries value %u\n", v.retries());
        return 1;
    }

    send_all(cmd, "RETRIES 0\r");
    read_line(cmd);
    if (v.retries() != 3) { // restored previous (startup was 3, then saved before set to 2)
        // After start retries_=3, RETRIES 2 saved 3 and set 2, RETRIES 0 restores 3
        fprintf(stderr, "FAIL: RETRIES 0 restore got %u want 3\n", v.retries());
        return 1;
    }

    send_all(cmd, "RETRIES 2\r");
    read_line(cmd);

    send_all(cmd, "LISTEN ON\r");
    if (read_line(cmd) != "OK") {
        fprintf(stderr, "FAIL: LISTEN\n");
        return 1;
    }

    send_all(cmd, "BW2300\r");
    if (read_line(cmd) != "OK") {
        fprintf(stderr, "FAIL: BW\n");
        return 1;
    }

    size_t before = txq.size();
    send_all(cmd, "CONNECT N0CALL N1CALL\r");
    if (read_line(cmd) != "OK") {
        fprintf(stderr, "FAIL: CONNECT\n");
        return 1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (txq.size() < before + 1) {
        fprintf(stderr, "FAIL: no connect TX frame\n");
        return 1;
    }

    // Wait through retries (2 attempts * 8s is long) — just check first attempt fired
    // and that RETRIES is honored by watching attempts via multiple short waits with
    // a custom port... For smoke, inject CONN_ACK to complete.
    auto& req = txq.back();
    // Build ACK: swap calls in the request payload type 1 -> 2
    std::vector<uint8_t> ack = req;
    ack[3] = 2;
    v.handle_rx(ack);
    auto conn = read_line(cmd);
    if (conn.rfind("CONNECTED ", 0) != 0) {
        fprintf(stderr, "FAIL: CONNECTED '%s'\n", conn.c_str());
        return 1;
    }

    send_all(data, "hello");
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    bool saw_buf = false;
    for (int i = 0; i < 20; ++i) {
        auto l = read_line(cmd, 200);
        if (l.rfind("BUFFER ", 0) == 0) saw_buf = true;
        if (l.empty()) break;
    }
    if (!saw_buf) {
        fprintf(stderr, "FAIL: no BUFFER after data\n");
        return 1;
    }

    close(cmd);
    close(data);
    v.stop();
    printf("OK vara_port smoke passed on %d/%d\n", port, port + 1);
    return 0;
}
