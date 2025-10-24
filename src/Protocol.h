#pragma once

#include <cstdint>

const int BLOCK_SIZE = 1024;

enum class MessageType : uint8_t {
    REQ_BITMAP = 1, // Solicitação do mapa de blocos
    RES_BITMAP = 2, // Resposta com o mapa de blocos
    REQ_BLOCK  = 3, // Solicitação de um bloco específico
    RES_BLOCK  = 4  // Resposta com os dados do bloco
};

#pragma pack(push, 1)
struct MsgHeader {
    MessageType type;
    uint32_t payloadSize;
};

struct MsgReqBlock {
    uint32_t blockIndex;
};

struct MsgResBlock {
    uint32_t blockIndex;
    char data[BLOCK_SIZE];
};
#pragma pack(pop)
