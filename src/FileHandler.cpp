#include "FileHandler.h"
#include <iostream>
#include <cmath>
#include <filesystem>
#include <numeric> 

FileHandler::FileHandler(const std::string& peerDir, const std::string& filename, uint32_t fileSize, bool isSeeder)
    : fileSize(fileSize), blocksOwned(0) {
    
    this->totalBlocks = static_cast<uint32_t>(std::ceil(static_cast<double>(fileSize) / BLOCK_SIZE));
    this->dataFilePath = std::filesystem::path(peerDir) / filename;

    std::filesystem::create_directory(peerDir);

    if (isSeeder) {
        if (!std::filesystem::exists(dataFilePath)) {
            createDummyFile(dataFilePath, fileSize);
        }
        initializeBitfield(true);
    } else {
        if (!std::filesystem::exists(dataFilePath)) {
            std::ofstream(dataFilePath, std::ios::binary).close();
            std::filesystem::resize_file(dataFilePath, fileSize);
        }
        initializeBitfield(false);
    }

    fileStream.open(dataFilePath, std::ios::in | std::ios::out | std::ios::binary);
    if (!fileStream.is_open()) {
        throw std::runtime_error("Falha ao abrir o arquivo: " + dataFilePath);
    }
}

FileHandler::~FileHandler() {
    if (fileStream.is_open()) {
        fileStream.close();
    }
}

void FileHandler::createDummyFile(const std::string& path, uint32_t size) {
    std::cout << "Criando arquivo dummy de " << size << " bytes em " << path << std::endl;
    std::ofstream ofs(path, std::ios::binary | std::ios::out);
    if (!ofs) {
        throw std::runtime_error("Falha ao criar arquivo dummy.");
    }
    // Preenche o arquivo com dados (ex: 'A', 'B', 'C'...)
    char data = 'A';
    for (uint32_t i = 0; i < size; i++) {
        ofs.write(&data, 1);
        if (++data > 'Z') data = 'A';
    }
    ofs.close();
}

void FileHandler::initializeBitfield(bool isSeeder) {
    bitfield.resize(totalBlocks, isSeeder);
    blocksOwned = isSeeder ? totalBlocks : 0;
}

bool FileHandler::hasBlock(uint32_t index) {
    std::lock_guard<std::mutex> lock(bitfieldMutex);
    if (index >= totalBlocks) return false;
    return bitfield[index];
}

std::vector<bool> FileHandler::getBitfield() {
    std::lock_guard<std::mutex> lock(bitfieldMutex);
    return bitfield;
}

bool FileHandler::isComplete() {
    std::lock_guard<std::mutex> lock(bitfieldMutex);
    return blocksOwned == totalBlocks;
}

uint32_t FileHandler::getTotalBlocks() const {
    return totalBlocks;
}

// Escreve dados no arquivo (thread-safe)
void FileHandler::setBlock(uint32_t index, const char* data) {
    if (index >= totalBlocks) return;

    {
        // Trava o arquivo para escrita
        std::lock_guard<std::mutex> lock(fileMutex);
        fileStream.seekp(static_cast<std::streamoff>(index) * BLOCK_SIZE);
        fileStream.write(data, BLOCK_SIZE);
        // Garante que a escrita seja salva no disco
        fileStream.flush(); 
    }

    {
        // Trava o bitfield para atualização
        std::lock_guard<std::mutex> lock(bitfieldMutex);
        if (!bitfield[index]) {
            bitfield[index] = true;
            blocksOwned++;
            
            double progress = (static_cast<double>(blocksOwned) / totalBlocks) * 100.0;
            std::cout.precision(2);
            std::cout << "Progresso: " << blocksOwned << "/" << totalBlocks << " blocos (" 
                      << std::fixed << progress << "%)" << std::endl;
        }
    }
}

// Lê dados do arquivo (thread-safe)
bool FileHandler::getBlock(uint32_t index, char* buffer) {
    if (!hasBlock(index)) { // Verifica o bitfield primeiro
        return false;
    }

    std::lock_guard<std::mutex> lock(fileMutex);
    fileStream.seekg(static_cast<std::streamoff>(index) * BLOCK_SIZE);
    fileStream.read(buffer, BLOCK_SIZE);
    
    // Verifica se a leitura foi bem-sucedida (pode falhar no último bloco se não for completo)
    return fileStream.gcount() > 0;
}

// Encontra o primeiro bloco que falta (estratégia simples)
uint32_t FileHandler::getNextMissingBlock() {
    std::lock_guard<std::mutex> lock(bitfieldMutex);
    for (uint32_t i = 0; i < totalBlocks; ++i) {
        if (!bitfield[i]) {
            return i;
        }
    }
    return static_cast<uint32_t>(-1); 
}
