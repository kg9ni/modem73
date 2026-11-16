#!/usr/bin/env python3
"""Regenerate packaging/windows/vara_port.hh from the Linux vara_port.hh."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "vara_port.hh"
DST = Path(__file__).resolve().parent / "vara_port.hh"

OLD_INCLUDES = """#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#include <unistd.h>"""

NEW_INCLUDES = """#include <winsock2.h>
#include <ws2tcpip.h>
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#ifndef SHUT_RD
#define SHUT_RD SD_RECEIVE
#endif
#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif
#ifndef socklen_t
using socklen_t = int;
#endif
#ifndef _SSIZE_T_DEFINED
using ssize_t = int;
#endif"""

HELPERS = """
// Winsock helpers (modem73-win uses int FDs + closesocket like control_port.hh).
static inline void sock_close(int fd) {
    if (fd >= 0) closesocket(fd);
}
static inline std::string sock_err() {
    return "WSA " + std::to_string(WSAGetLastError());
}

"""

HEADER = """#pragma once

// Windows/Winsock port of the VARA-compatible host ports (from kg9ni/modem73).
// Keep in sync with the Linux vara_port.hh; regenerate via packaging/windows/mk-winsock-vara-port.py
"""


def main() -> int:
    text = SRC.read_text()
    if OLD_INCLUDES not in text:
        print("POSIX include block not found in", SRC, file=sys.stderr)
        return 1
    text = text.replace(OLD_INCLUDES, NEW_INCLUDES)
    text = text.replace("class VaraPort {\npublic:", HELPERS + "class VaraPort {\npublic:")
    text = re.sub(r"\bclose\(", "sock_close(", text)
    text = text.replace("strerror(errno)", "sock_err()")

    wake_old = """        int s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0) return;
        struct timeval tv{0, 100000};
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        connect(s, (sockaddr*)&addr, sizeof(addr));
        sock_close(s);"""
    wake_new = """        int s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0) return;
        DWORD tv_ms = 100;
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_ms, sizeof(tv_ms));
        setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv_ms, sizeof(tv_ms));
        connect(s, (sockaddr*)&addr, sizeof(addr));
        sock_close(s);"""
    if wake_old not in text:
        print("wake() timeout block not found", file=sys.stderr)
        return 1
    text = text.replace(wake_old, wake_new)

    def cast_setsockopt(m: re.Match[str]) -> str:
        full = m.group(0)
        if "(const char*)" in full or "(char*)" in full:
            return full
        return re.sub(
            r"setsockopt\(([^,]+),\s*([^,]+),\s*([^,]+),\s*&([a-zA-Z0-9_]+),\s*sizeof\(([a-zA-Z0-9_]+)\)\)",
            r"setsockopt(\1, \2, \3, (const char*)&\4, sizeof(\5))",
            full,
        )

    text = re.sub(r"setsockopt\([^;]+\)", cast_setsockopt, text)
    text = re.sub(r"recv\(([^,]+),\s*buf,\s*", r"recv(\1, (char*)buf, ", text)
    text = re.sub(r"recv\(([^,]+),\s*buf\.data\(\),\s*", r"recv(\1, (char*)buf.data(), ", text)
    text = re.sub(r"send\(([^,]+),\s*p \+ off,\s*", r"send(\1, (const char*)(p + off), ", text)

    if text.startswith("#pragma once"):
        text = HEADER + text[len("#pragma once") :].lstrip("\n")
    else:
        text = HEADER + text

    DST.write_text(text)
    print("Wrote", DST)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
