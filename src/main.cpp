#define ENET_IMPLEMENTATION
#include "server.h"

int main(int argc, char** argv) { 
    try {
        Logger::printStartMessage();
        if (UdpManager::getUdpManager()->initializeENet() != 0) {
            Logger::Log(ERROR, "An error occurred while trying to initialize ENet");
            return EXIT_FAILURE;
        }
        UdpManager::getUdpManager()->createENetHost(2, 0, 0, 2);
        UdpManager::getUdpManager()->run(5);
    } catch (std::exception& e) {
        Logger::Log(ERROR, e.what());
    }
    return EXIT_SUCCESS;
}
