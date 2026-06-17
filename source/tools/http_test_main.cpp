#include "../http/HTTPServer.h"
#include <iostream>

int main() {
    HTTPServer server;
    if (!server.Start(17653, "app")) {
        std::cerr << "Failed to start server\n";
        return 1;
    }

    std::cout << "Serving ./app at http::localhost:17653 (press Enter to quit)\n";
    std::cin.get();
    server.Stop();
    return 0;
}
