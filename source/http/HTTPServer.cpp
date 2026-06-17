#include "HTTPServer.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

// ---- Helpers
static std::string mimeType(const std::string& path) {
    auto dot = path.find_last_of('.');
    std::string ext = (dot == std::string::npos) ? "" : path.substr(dot);
    if (ext == ".html")  return "text/html";
    if (ext == ".js" || ext == ".mjs") return "text/javascript";
    if (ext == ".css")   return "text/css";
    if (ext == ".json")  return "application/json";
    if (ext == ".svg")   return "image/svg+xml";
    if (ext == ".png")   return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif")   return "image/gif";
    if (ext == ".ico")   return "image/x-icon";
    if (ext == ".woff")  return "font/woff";
    if (ext == ".woff2") return "font/woff2";
    if (ext == ".ttf")   return "font/ttf";
    if (ext == ".wasm")  return "application/wasm";
    return "application/octet-stream";
}

static bool readFile(const std::string& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

static void sendAll(SOCKET s, const char* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        int n = send(s, data + sent, static_cast<int>(len - sent), 0);
        if (n <= 0) break;
        sent += static_cast<size_t>(n);
    }
}

static void sendResponse(SOCKET client, const std::string& status,
                         const std::string& contentType, const std::string& body) {
    std::string header =
        "HTTP/1.1 " + status + "\r\n"
        "Content-Type: " + contentType + "\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n";
    sendAll(client, header.data(), header.size());
    sendAll(client, body.data(), body.size());
}

static void handleClient(SOCKET client, const std::string& rootDir) {
    char buffer[4096];
    int bytes = recv(client, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) return;
    buffer[bytes] = '\0';

    std::string req(buffer, bytes);

    // Parse the request line: "GET /path HTTP/1.1"
    size_t sp1 = req.find(' ');
    size_t sp2 = req.find(' ', sp1 + 1);
    if (sp1 == std::string::npos || sp2 == std::string::npos) {
        sendResponse(client, "400 Bad Request", "text/plain", "Bad Request");
        return;
    }
    std::string path = req.substr(sp1 + 1, sp2 - sp1 - 1);

    // Strip query string (?...)
    size_t q = path.find('?');
    if (q != std::string::npos) path = path.substr(0, q);

    // Directory-traversal guard
    if (path.find("..") != std::string::npos) {
        sendResponse(client, "400 Bad Request", "text/plain", "Bad Request");
        return;
    }

    if (path == "/") path = "/index.html";

    std::string body;
    if (readFile(rootDir + path, body)) {
        sendResponse(client, "200 OK", mimeType(path), body);
    } else if (readFile(rootDir + "/index.html", body)) {
        // SPA fallback: unknown route -> let the Vue app boot and route itself
        sendResponse(client, "200 OK", "text/html", body);
    } else {
        sendResponse(client, "404 Not Found", "text/plain", "404 Not Found");
    }
}

HTTPServer::~HTTPServer() {
    Stop();
}

bool HTTPServer::Start(int port, const std::string& rootDir) {
    port_ = port;
    rootDir_ = rootDir;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    BOOL yes = TRUE;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&yes), sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // 127.0.0.1 only

    if (bind(listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR ||
        listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(listenSock);
        WSACleanup();
        return false;
    }

    listenSocket_ = static_cast<uintptr_t>(listenSock);
    running_ = true;
    thread_ = std::thread(&HTTPServer::acceptLoop, this);
    return true;
}

void HTTPServer::acceptLoop() {
    SOCKET listenSock = static_cast<SOCKET>(listenSocket_);
    while (running_) {
        SOCKET client = accept(listenSock, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            if (!running_) break;   // Stop() closed the socket
            continue;
        }
        handleClient(client, rootDir_);
        closesocket(client);
    }
}

void HTTPServer::Stop() {
    if (!running_.exchange(false)) return;  // already stopped

    if (listenSocket_ != ~uintptr_t(0)) {
        closesocket(static_cast<SOCKET>(listenSocket_));  // unblocks accept()
        listenSocket_ = ~uintptr_t(0);
    }
    if (thread_.joinable()) thread_.join();
    WSACleanup();
}
