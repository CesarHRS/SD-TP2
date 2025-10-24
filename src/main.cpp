#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>       
#include <csignal>      
#include "Peer.h"
#include "FileHandler.h"
#include "Protocol.h"     

const std::string FILE_NAME = "shared_file.dat";
const uint32_t FILE_SIZE = 5 * 1024 * BLOCK_SIZE; 
const std::string BASE_IP = "127.0.0.1";

//(mapa de ID -> Porta)
std::map<int, int> PEER_PORTS = {
    {1, 9001},
    {2, 9002},
    {3, 9003}
};

std::map<int, std::vector<Peer::Neighbor>> PEER_NEIGHBORS = {
    // Peer 1 (Seeder) conhece 2 e 3
    {1, {{BASE_IP, 9002}, {BASE_IP, 9003}}},
    // Peer 2 (Leecher) conhece 1 e 3
    {2, {{BASE_IP, 9001}, {BASE_IP, 9003}}},
    // Peer 3 (Leecher) conhece 1 e 2
    {3, {{BASE_IP, 9001}, {BASE_IP, 9002}}}
};

std::shared_ptr<Peer> globalPeer; // Ponteiro global para o peer
void signalHandler(int signum) {
    std::cout << "\nSinal (Ctrl+C) recebido. Encerrando peer..." << std::endl;
    if (globalPeer) {
        globalPeer->stop();
    }
    exit(signum);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <ID_do_Peer> <IsSeeder (1 ou 0)>" << std::endl;
        std::cerr << "Ex (Seeder): " << argv[0] << " 1 1" << std::endl;
        std::cerr << "Ex (Leecher): " << argv[0] << " 2 0" << std::endl;
        return 1;
    }

    int peerId = 0;
    bool isSeeder = false;
    try {
        peerId = std::stoi(argv[1]);
        isSeeder = std::stoi(argv[2]) == 1;
    } catch (const std::exception& e) {
        std::cerr << "Erro: ID e IsSeeder devem ser números inteiros." << std::endl;
        return 1;
    }

    if (PEER_PORTS.find(peerId) == PEER_PORTS.end()) {
        std::cerr << "Erro: ID do Peer inválido. IDs válidos: 1, 2, 3." << std::endl;
        return 1;
    }

    // Configurações do Peer
    int myPort = PEER_PORTS[peerId];
    std::vector<Peer::Neighbor> myNeighbors = PEER_NEIGHBORS[peerId];
    std::string myDirectory = "peer_" + std::to_string(peerId) + "_files";

    try {
        // Registra o handler para Ctrl+C
        signal(SIGINT, signalHandler);

        // 1. Cria o FileHandler
        auto fileHandler = std::make_shared<FileHandler>(myDirectory, FILE_NAME, FILE_SIZE, isSeeder);

        // 2. Cria o Peer
        globalPeer = std::make_shared<Peer>(peerId, myPort, myNeighbors, fileHandler, isSeeder);

        // 3. Inicia o Peer (isto irá bloquear até que o Peer termine ou Ctrl+C)
        globalPeer->start();

        // O 'join' nas threads (se implementado) ou um loop de espera
        // No nosso caso, as threads rodam em background e o main terminaria.
        // Vamos manter o main vivo para esperar o Ctrl+C.
        std::cout << "Peer " << peerId << " em execução. Pressione Ctrl+C para sair." << std::endl;
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }

    } catch (const std::exception& e) {
        std::cerr << "Erro fatal: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
