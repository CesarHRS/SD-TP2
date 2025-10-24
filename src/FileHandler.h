#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include "Protocol.h"

class FileHandler {
public:
    FileHandler(const std::string& peerDir, const std::string& filename, uint32_t fileSize, bool isSeeder);
    ~FileHandler();

    // Métodos para o Bitfield
    bool hasBlock(uint32_t index);
    std::vector<bool> getBitfield();
    bool isComplete();
    uint32_t getTotalBlocks() const;
    uint32_t getNextMissingBlock(); // Encontra o próximo bloco faltando

    // Métodos de E/S de Arquivo
    // Escreve dados em um bloco específico
    void setBlock(uint32_t index, const char* data);
    // Lê dados de um bloco específico
    bool getBlock(uint32_t index, char* buffer);

private:
    void createDummyFile(const std::string& path, uint32_t size);
    void initializeBitfield(bool isSeeder);

    std::string dataFilePath;
    uint32_t fileSize;
    uint32_t totalBlocks;

    std::vector<bool> bitfield;
    uint32_t blocksOwned;

    // Mutexes para garantir thread-safety
    std::mutex bitfieldMutex; // Protege o bitfield e blocksOwned
    std::mutex fileMutex;     // Protege o acesso ao fstream

    // Stream de arquivo (entrada e saída)
    std::fstream fileStream;
};
