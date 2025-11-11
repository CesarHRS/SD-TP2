Trabalho Prático 2 - Transferência de Arquivos Peer-to-Peer (P2P)
Sistemas Distribuídos - 2025

## 1. DESCRIÇÃO GERAL

Este projeto implementa um sistema de transferência de arquivos baseado
em uma arquitetura Peer-to-Peer (P2P), na qual cada nó (Peer) atua como
cliente e servidor simultaneamente. Os arquivos são divididos em blocos
de tamanho fixo e distribuídos entre os Peers até que todos possuam o
arquivo completo.

O sistema foi desenvolvido em C++17, utilizando threads (std::thread),
sockets POSIX e sincronização via mutex. A arquitetura adota um modelo
simétrico, onde qualquer Peer pode enviar ou receber blocos.

## 2. TECNOLOGIAS E DEPENDÊNCIAS

Linguagem: C++17
Bibliotecas: std::thread, std::mutex, std::filesystem, POSIX sockets
Compilador: g++ versão 10 ou superior

Ambiente recomendado:

- Linux (Ubuntu, Debian, Fedora, etc.)
- macOS (com clang++ 10+)

Para Windows:

- Usar WSL (Windows Subsystem for Linux) ou Cygwin, que possuem suporte
  a sockets POSIX.

## 3. ESTRUTURA DO PROJETO

TP2-P2P/
├── src/
│ ├── main.cpp
│ ├── Peer.cpp
│ ├── FileHandler.cpp
│ ├── Peer.h
│ ├── FileHandler.h
│ ├── Protocol.h
│ └── ...
├── run_test.sh
├── Makefile
└── README.txt

## 4. COMPILAÇÃO

Para compilar o projeto, execute o comando na raiz do diretório:

    make

O executável será gerado em:

    ./build/peer

## 5. EXECUÇÃO PADRÃO (CENÁRIO DE TESTE)

Para executar o cenário de teste padrão com três Peers (um Seeder e
dois Leechers), utilize o comando:

    make test

ou, alternativamente:

    ./run_test.sh

O script de teste realiza as seguintes ações:

1. Compila o projeto (se necessário);
2. Inicia três Peers:
   - Peer 1: Seeder (porta 9001) - possui o arquivo completo
   - Peer 2: Leecher (porta 9002)
   - Peer 3: Leecher (porta 9003)
3. Monitora o progresso de download dos Leechers;
4. Após a conclusão, gera checksums SHA-256 dos arquivos baixados;
5. Compara os resultados com o arquivo original para verificar integridade;
6. Salva logs e resultados de verificação em build/logs/.

## 6. EXECUÇÃO MANUAL (PEER INDIVIDUAL)

É possível executar manualmente um Peer individual:

    ./build/peer <ID_do_Peer> <IsSeeder>

Exemplo:

    ./build/peer 1 1 &   # Peer 1 atua como Seeder
    ./build/peer 2 0     # Peer 2 atua como Leecher

Parâmetros:

- ID_do_Peer: número identificador único do Peer (ex: 1, 2, 3)
- IsSeeder: 1 = verdadeiro (Seeder), 0 = falso (Leecher)

## PASSO A PASSO MANUALMENTE

0. Inicia o Seeder em primeiro plano (para ver a criação do arquivo)
   `./build/peer 1 1`

 No nosso caso, o main.cpp tem um loop infinito de 10s para manter o peer vivo:

 `"while (true) { std::this_thread::sleep_for(std::chrono::seconds(10)); }"`

 Isso significa que você precisa usar CTRL+C para matar o Peer 1.

 Vamos seguir a estrutura do script, mas fora dele, para ver o PID.

 Certifique-se que o executável está na pasta build

make all

1.  Abra um terminal
    `./build/peer 1 1 > peer1.log 2>&1 &
    PID1=$!
    echo "Peer 1 (Seeder) rodando com PID: $PID1"`

2.  Abra outro terminal
    `./build/peer 2 0 > peer2.log 2>&1 &
    PID2=$!
    echo "Peer 2 (Leecher) rodando com PID: $PID2"`

3.  Abra outro terminal
    `./build/peer 3 0 > peer3.log 2>&1 &
    PID3=$!
    echo "Peer 3 (Leecher) rodando com PID: $PID3"`

4.  Monitore os logs
    `tail -f peer2.log peer3.log`

5.  Para matar todos após o teste:
    `kill $PID1 $PID2 $PID3`


## 7. CONFIGURAÇÃO PADRÃO DOS PEERS

| Peer | Porta | Função Inicial | Vizinhos Conhecidos  |
| ---- | ----- | -------------- | -------------------- |
| 1    | 9001  | Seeder         | P2 (9002), P3 (9003) |
| 2    | 9002  | Leecher        | P1 (9001), P3 (9003) |
| 3    | 9003  | Leecher        | P1 (9001), P2 (9002) |

A vizinhança é configurada estaticamente no código (main.cpp), de modo
que todos os Peers se conheçam no momento da inicialização.

## 8. COMPONENTES PRINCIPAIS

1. Peer.cpp / Peer.h

   - Controla as threads de servidor e cliente.
   - Estabelece conexões TCP com os vizinhos.
   - Solicita blocos faltantes e envia blocos disponíveis.

2. FileHandler.cpp / FileHandler.h

   - Controla o acesso concorrente ao arquivo e ao bitfield.
   - Fragmenta o arquivo em blocos de 1024 bytes.
   - Atualiza o mapa de blocos disponíveis (bitfield) conforme downloads.

3. Protocol.h
   - Define o protocolo de mensagens binárias:
     REQ_BITMAP -> Solicitação do bitfield
     RES_BITMAP -> Resposta com o bitfield
     REQ_BLOCK -> Solicitação de bloco
     RES_BLOCK -> Envio do bloco solicitado
   - Estruturas empacotadas via pragma pack e uso de htonl/ntohl
     para compatibilidade de endianness entre sistemas.

## 9. ESTRATÉGIA DE DOWNLOAD E CONCORRÊNCIA

Cada Peer executa duas threads principais:

- serverThread: escuta e atende solicitações de blocos.
- clientThread: conecta-se aos vizinhos e solicita blocos faltantes.

A sincronização é garantida através de mutexes:

- fileMutex: protege o acesso ao arquivo durante leitura e escrita.
- bitfieldMutex: protege o mapa de blocos possuídos.

O cliente adota uma estratégia simples:

1. Solicita o bitfield de cada vizinho.
2. Identifica o primeiro bloco ausente que o vizinho possui.
3. Solicita o bloco e o grava localmente.
4. Atualiza o bitfield e passa a servir o bloco a outros Peers.

5. TESTES E RESULTADOS ESPERADOS

Ao executar o script de teste (make test ou ./run_test.sh), espera-se:

- Os Peers Leechers (2 e 3) completam o download do arquivo.
- O Peer Seeder (1) distribui blocos continuamente.
- Após conclusão, todos os Peers passam a ter o arquivo completo.
- O checksum SHA-256 dos arquivos baixados é idêntico ao original.

Exemplo de saída esperada no terminal:

    Peer 2: Solicitando bloco 240 de 127.0.0.1:9001
    Peer 3: Solicitando bloco 100 de 127.0.0.1:9002
    Peer 2: DOWNLOAD COMPLETO
    Peer 3: DOWNLOAD COMPLETO
    Arquivos validados com sucesso (SHA-256 idêntico)

## 11. CONCLUSÕES

O projeto demonstra a aplicação prática dos conceitos de sistemas
distribuídos e arquitetura Peer-to-Peer:

- Cada Peer atua como cliente e servidor simultaneamente.
- A troca de blocos é descentralizada e cooperativa.
- O sistema garante integridade e consistência dos dados.
- A sincronização via mutex evita condições de corrida.
- O modelo de comunicação é simples, eficiente e escalável.

## 12. AUTORES

Trabalho desenvolvido por alunos César Soares e Gabriel Quezada da disciplina de Sistemas Distribuídos
(2025), utilizando C++17 e sockets POSIX.
