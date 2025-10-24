#pragma once

#include <string>
#include <vector>
#include <thread>
#include <memory>
#include "FileHandler.h"

class Peer {
public:
    struct Neighbor {
        std::string ip;
        int port;
    };

    Peer(int id, int port, std::vector<Neighbor> neighbors, std::shared_ptr<FileHandler> fileHandler, bool isSeeder);
    ~Peer();

    void start(); 
    void stop();

private:
    void runServer();
    void handleConnection(int clientSocket);

    void runClient();

    // Cliente aux
    std::vector<bool> requestBitfield(const Neighbor& neighbor);
    bool requestBlock(const Neighbor& neighbor, uint32_t blockIndex);

    // Funções de rede (wrapper para send/recv)
    bool sendData(int sock, const void* data, size_t size);
    bool recvData(int sock, void* data, size_t size);

    int peerId;
    int serverPort;
    int serverSocket;
    std::vector<Neighbor> neighbors;
    std::shared_ptr<FileHandler> fileHandler;
    bool isSeeder;

    std::thread serverThread;
    std::thread clientThread;
    bool running; 
};
