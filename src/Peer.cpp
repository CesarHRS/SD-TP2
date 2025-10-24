#include "Peer.h"
#include "Protocol.h"
#include <iostream>
#include <cstring> 
#include <unistd.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <stdexcept>
#include <chrono> 

#include <netinet/in.h> 

Peer::Peer(int id, int port, std::vector<Neighbor> neighbors, std::shared_ptr<FileHandler> fileHandler, bool isSeeder)
    : peerId(id), serverPort(port), neighbors(neighbors), fileHandler(fileHandler), isSeeder(isSeeder), running(false), serverSocket(-1) {
    
    std::cout << "Peer " << peerId << " inicializado. Porta: " << serverPort 
              << (isSeeder ? " (Seeder)" : " (Leecher)") << std::endl;
}

Peer::~Peer() {
    stop();
}

void Peer::start() {
    running = true;
    serverThread = std::thread(&Peer::runServer, this);

    if (!isSeeder) {
        clientThread = std::thread(&Peer::runClient, this);
    }
}

void Peer::stop() {
    running = false;

    // Fecha o socket do servidor para desbloquear o 'accept'
    if (serverSocket != -1) {
        close(serverSocket);
        serverSocket = -1;
    }

    if (serverThread.joinable()) {
        serverThread.join();
    }
    if (clientThread.joinable()) {
        clientThread.join();
    }
}

// --- Servidor ---

void Peer::runServer() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Falha ao criar socket (servidor)");
        return;
    }

    // Permite reutilizar o endereço imediatamente
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(serverPort);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Falha no bind (servidor)");
        close(serverSocket);
        return;
    }

    if (listen(serverSocket, 5) < 0) {
        perror("Falha no listen (servidor)");
        close(serverSocket);
        return;
    }

    std::cout << "Peer " << peerId << " escutando na porta " << serverPort << std::endl;

    while (running) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket < 0) {
            if (running) perror("Erro no accept (servidor)");
            continue; // Continua se 'accept' falhou (pode ser pelo stop())
        }

        // Cria uma nova thread para tratar a conexão
        std::thread(&Peer::handleConnection, this, clientSocket).detach();
    }

    if (serverSocket != -1) {
        close(serverSocket);
    }
}

void Peer::handleConnection(int clientSocket) {
    MsgHeader header;
    
    // Cabeçalho
    if (!recvData(clientSocket, &header, sizeof(header))) {
        close(clientSocket);
        return;
    }
    // Converte para a ordem do host
    header.payloadSize = ntohl(header.payloadSize);


    // Processa
    switch (header.type) {
        case MessageType::REQ_BITMAP: {
            // Cliente solicitou nosso bitfield
            std::vector<bool> bitfield = fileHandler->getBitfield();
            std::vector<uint8_t> bitfieldData(bitfield.size());
            for (size_t i = 0; i < bitfield.size(); ++i) {
                bitfieldData[i] = static_cast<uint8_t>(bitfield[i]);
            }

            // Envia cabeçalho de resposta
            MsgHeader resHeader;
            resHeader.type = MessageType::RES_BITMAP;
            resHeader.payloadSize = htonl(static_cast<uint32_t>(bitfieldData.size()));
            sendData(clientSocket, &resHeader, sizeof(resHeader));

            // Envia payload 
            sendData(clientSocket, bitfieldData.data(), bitfieldData.size());
            break;
        }

        case MessageType::REQ_BLOCK: {
            // Cliente solicitou um bloco específico
            if (header.payloadSize != sizeof(MsgReqBlock)) {
                break; // Tamanho de payload inválido
            }

            MsgReqBlock reqBlock;
            recvData(clientSocket, &reqBlock, sizeof(reqBlock));
            reqBlock.blockIndex = ntohl(reqBlock.blockIndex);

            char blockBuffer[BLOCK_SIZE];
            if (fileHandler->getBlock(reqBlock.blockIndex, blockBuffer)) {
                // Bloco encontrado, envia resposta
                MsgResBlock resBlock;
                resBlock.blockIndex = htonl(reqBlock.blockIndex);
                memcpy(resBlock.data, blockBuffer, BLOCK_SIZE);

                MsgHeader resHeader;
                resHeader.type = MessageType::RES_BLOCK;
                resHeader.payloadSize = htonl(sizeof(resBlock));

                sendData(clientSocket, &resHeader, sizeof(resHeader));
                sendData(clientSocket, &resBlock, sizeof(resBlock));
            }
            // Se getBlock falhar, não envia nada
            break;
        }
        
        default:
            std::cerr << "Peer " << peerId << ": Tipo de mensagem desconhecido recebido." << std::endl;
            break;
    }

    close(clientSocket);
}


// --- Cliente ---

void Peer::runClient() {
    std::cout << "Peer " << peerId << " (Cliente) iniciado." << std::endl;
    
    while (running && !fileHandler->isComplete()) {
        bool FaltamBlocos = false;

        // Itera sobre todos os vizinhos
        for (const auto& neighbor : neighbors) {
            // 1. Solicita o Bitfield do vizinho
            std::vector<bool> neighborBitfield = requestBitfield(neighbor);
            if (neighborBitfield.empty()) {
                continue; // Falha ao conectar ou obter bitfield
            }

            // 2. Compara com nosso bitfield e encontra blocos para baixar
            for (uint32_t i = 0; i < fileHandler->getTotalBlocks(); ++i) {
                if (!fileHandler->hasBlock(i) && neighborBitfield[i]) {
                    // Vizinho tem um bloco que não temos
                    FaltamBlocos = true;
                    std::cout << "Peer " << peerId << ": Solicitando bloco " << i 
                              << " de " << neighbor.ip << ":" << neighbor.port << std::endl;
                    
                    // 3. Solicita o bloco
                    if (requestBlock(neighbor, i)) {
                        // Sucesso
                    } else {
                        // Falha
                        std::cerr << "Peer " << peerId << ": Falha ao baixar bloco " << i << std::endl;
                    }

                    // Estratégia simples: Pega um bloco por vizinho por ciclo
                    break; 
                }
            }
        }
        
        if (!FaltamBlocos && !fileHandler->isComplete()) {
            // Não encontramos blocos novos, mas ainda não completamos.
            // Isso pode acontecer se os vizinhos também não tiverem.
             std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        // Pausa curta para não sobrecarregar a rede
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (fileHandler->isComplete()) {
        std::cout << "******************************************" << std::endl;
        std::cout << "Peer " << peerId << ": DOWNLOAD COMPLETO!" << std::endl;
        std::cout << "******************************************" << std::endl;
    }
}

// Conecta a um vizinho e pede seu bitfield
std::vector<bool> Peer::requestBitfield(const Neighbor& neighbor) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return {};

    sockaddr_in neighborAddr;
    neighborAddr.sin_family = AF_INET;
    neighborAddr.sin_port = htons(neighbor.port);
    neighborAddr.sin_addr.s_addr = inet_addr(neighbor.ip.c_str());

    if (connect(sock, (sockaddr*)&neighborAddr, sizeof(neighborAddr)) < 0) {
        close(sock);
        return {};
    }

    // Envia REQ_BITMAP
    MsgHeader reqHeader;
    reqHeader.type = MessageType::REQ_BITMAP;
    reqHeader.payloadSize = 0; // Sem payload
    if (!sendData(sock, &reqHeader, sizeof(reqHeader))) {
        close(sock);
        return {};
    }

    // Recebe RES_BITMAP
    MsgHeader resHeader;
    if (!recvData(sock, &resHeader, sizeof(resHeader)) || resHeader.type != MessageType::RES_BITMAP) {
        close(sock);
        return {};
    }

    uint32_t payloadSize = ntohl(resHeader.payloadSize);
    if (payloadSize != fileHandler->getTotalBlocks()) {
         std::cerr << "Peer " << peerId << ": Tamanho de bitfield inesperado de " << neighbor.port << std::endl;
         close(sock);
         return {};
    }

    std::vector<uint8_t> bitfieldData(payloadSize);
    if (!recvData(sock, bitfieldData.data(), payloadSize)) {
        close(sock);
        return {};
    }

    std::vector<bool> bitfield(payloadSize);
    for (size_t i = 0; i < payloadSize; ++i) {
        bitfield[i] = static_cast<bool>(bitfieldData[i]);
    }

    close(sock);
    return bitfield;
}

// Conecta a um vizinho e pede um bloco específico
bool Peer::requestBlock(const Neighbor& neighbor, uint32_t blockIndex) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;

    sockaddr_in neighborAddr;
    neighborAddr.sin_family = AF_INET;
    neighborAddr.sin_port = htons(neighbor.port);
    neighborAddr.sin_addr.s_addr = inet_addr(neighbor.ip.c_str());

    if (connect(sock, (sockaddr*)&neighborAddr, sizeof(neighborAddr)) < 0) {
        close(sock);
        return false;
    }

    // Envia REQ_BLOCK
    MsgReqBlock reqBlock;
    reqBlock.blockIndex = htonl(blockIndex);
    
    MsgHeader reqHeader;
    reqHeader.type = MessageType::REQ_BLOCK;
    reqHeader.payloadSize = htonl(sizeof(reqBlock));

    if (!sendData(sock, &reqHeader, sizeof(reqHeader))) {
        close(sock); return false;
    }
    if (!sendData(sock, &reqBlock, sizeof(reqBlock))) {
        close(sock); return false;
    }

    // Recebe RES_BLOCK
    MsgHeader resHeader;
    if (!recvData(sock, &resHeader, sizeof(resHeader)) || resHeader.type != MessageType::RES_BLOCK) {
        close(sock); return false;
    }

    uint32_t payloadSize = ntohl(resHeader.payloadSize);
    if (payloadSize != sizeof(MsgResBlock)) {
        close(sock); return false;
    }

    MsgResBlock resBlock;
    if (!recvData(sock, &resBlock, sizeof(resBlock))) {
        close(sock); return false;
    }

    if (ntohl(resBlock.blockIndex) == blockIndex) {
        fileHandler->setBlock(blockIndex, resBlock.data);
    } else {
        std::cerr << "Peer " << peerId << ": Recebido bloco " << ntohl(resBlock.blockIndex) << " quando esperado " << blockIndex << std::endl;
        close(sock);
        return false;
    }

    close(sock);
    return true;
}


// --- Funções Auxiliares de Rede (Wrappers) ---

// Garante que todos os dados sejam enviados
bool Peer::sendData(int sock, const void* data, size_t size) {
    size_t totalSent = 0;
    const char* buffer = static_cast<const char*>(data);
    while (totalSent < size) {
        ssize_t sent = send(sock, buffer + totalSent, size - totalSent, 0);
        if (sent <= 0) {
            perror("Erro no send");
            return false;
        }
        totalSent += sent;
    }
    return true;
}

// Garante que todos os dados sejam recebidos
bool Peer::recvData(int sock, void* data, size_t size) {
    size_t totalRecv = 0;
    char* buffer = static_cast<char*>(data);
    while (totalRecv < size) {
        ssize_t recvd = recv(sock, buffer + totalRecv, size - totalRecv, 0);
        if (recvd <= 0) {
            // Se recvd == 0, o peer desconectou
            // Se recvd < 0, foi um erro
            if (recvd < 0) perror("Erro no recv");
            return false;
        }
        totalRecv += recvd;
    }
    return true;
}
