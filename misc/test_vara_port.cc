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

    // Exhaust RETRIES=2 with no peer answer; expect complete Failure log + DISCONNECTED.
    std::vector<std::string> logs;
    h.log = [&](const std::string& m) { logs.push_back(m); };
    // Rebuild hooks into a fresh port for the failure path (same ports after stop).
    v.stop();
    close(cmd);
    close(data);
    VaraPort v2(port, "127.0.0.1", 2, h);
    // Shorten retry wait for the test by exhausting via many attempt ticks:
    // CONNECT_RETRY_MS is 8s; force give-up by calling RETRIES 1 after first TX.
    if (!v2.start()) {
        fprintf(stderr, "FAIL: restart\n");
        return 1;
    }
    cmd = connect_port(port);
    data = connect_port(port + 1);
    if (cmd < 0 || data < 0) {
        fprintf(stderr, "FAIL: reconnect\n");
        return 1;
    }
    read_line(cmd); // BUSY
    send_all(cmd, "MYCALL N0CALL\r");
    read_line(cmd);
    send_all(cmd, "RETRIES 1\r");
    read_line(cmd);
    send_all(cmd, "CONNECT N0CALL N1CALL\r");
    if (read_line(cmd) != "OK") {
        fprintf(stderr, "FAIL: CONNECT for failure path\n");
        return 1;
    }
    // Wait past first attempt window so retries==1 exhausts.
    // With RETRIES 1: first attempt fires, next deadline gives up.
    // CONNECT_RETRY_MS=8000 — wait a bit over that. Drain async host lines.
    std::this_thread::sleep_for(std::chrono::milliseconds(8500));
    bool saw_disc = false;
    for (int i = 0; i < 40; ++i) {
        auto l = read_line(cmd, 300);
        if (l == "DISCONNECTED") saw_disc = true;
        if (l.empty() && saw_disc) break;
    }
    if (!saw_disc) {
        fprintf(stderr, "FAIL: no DISCONNECTED after retries\n");
        return 1;
    }
    bool saw_fail = false;
    for (auto& m : logs) {
        if (m == "Failure with N1CALL") saw_fail = true;
    }
    if (!saw_fail) {
        fprintf(stderr, "FAIL: missing complete 'Failure with N1CALL' log\n");
        for (auto& m : logs) fprintf(stderr, "  log: %s\n", m.c_str());
        return 1;
    }

    // Successful CONNECT path on a third port pair
    v2.stop();
    close(cmd);
    close(data);
    int port3 = port + 2;
    for (; port3 < 18400; port3 += 2) {
        VaraPort probe(port3, "127.0.0.1", 3, h);
        if (probe.start()) { probe.stop(); break; }
    }
    VaraPort v3(port3, "127.0.0.1", 3, h);
    if (!v3.start()) {
        fprintf(stderr, "FAIL: start success path\n");
        return 1;
    }
    cmd = connect_port(port3);
    data = connect_port(port3 + 1);
    read_line(cmd);
    send_all(cmd, "MYCALL N0CALL\r");
    read_line(cmd);
    size_t before = txq.size();
    send_all(cmd, "CONNECT N0CALL N1CALL\r");
    if (read_line(cmd) != "OK") {
        fprintf(stderr, "FAIL: CONNECT\n");
        return 1;
    }
    read_line(cmd); // BUSY
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (txq.size() < before + 1) {
        fprintf(stderr, "FAIL: no connect TX frame\n");
        return 1;
    }
    auto& req = txq.back();
    std::vector<uint8_t> ack = req;
    ack[3] = 2;
    v3.handle_rx(ack);
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
    v3.stop();
    printf("OK vara_port smoke passed (failure + connect) on %d\n", port);
    return 0;
}
