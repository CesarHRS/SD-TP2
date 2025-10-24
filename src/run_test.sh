#!/bin/bash
set -euo pipefail


ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_EXE="$ROOT_DIR/build/peer"

if [ ! -x "$BUILD_EXE" ]; then
  echo "Compilando projeto..."
  make
fi

LOGDIR="$ROOT_DIR/build/logs"
mkdir -p "$LOGDIR"

echo "Iniciando Peer 1 (Seeder)..."
"$BUILD_EXE" 1 1 > "$LOGDIR/peer1.log" 2>&1 &
PID1=$!
sleep 1

echo "Iniciando Peer 2 (Leecher)..."
"$BUILD_EXE" 2 0 > "$LOGDIR/peer2.log" 2>&1 &
PID2=$!
sleep 1

echo "Iniciando Peer 3 (Leecher)..."
"$BUILD_EXE" 3 0 > "$LOGDIR/peer3.log" 2>&1 &
PID3=$!

echo "Peers iniciados (PIDs): $PID1 $PID2 $PID3"
echo "Aguardando conclusão dos downloads (timeout 300s)..."

TIMEOUT=300
START=$SECONDS
while true; do
  if grep -q "DOWNLOAD COMPLETO" "$LOGDIR/peer2.log" && grep -q "DOWNLOAD COMPLETO" "$LOGDIR/peer3.log"; then
    echo "Downloads completos detectados nos leechers.";
    break
  fi
  if [ $((SECONDS - START)) -ge $TIMEOUT ]; then
    echo "Timeout atingido ($TIMEOUT s) esperando downloads.";
    break
  fi
  sleep 2
done

echo "Gerando checksums..."
sha256sum peer_1_files/shared_file.dat > "$LOGDIR/seeder.sha256" 2>/dev/null || true
sha256sum peer_2_files/shared_file.dat > "$LOGDIR/peer2.sha256" 2>/dev/null || true
sha256sum peer_3_files/shared_file.dat > "$LOGDIR/peer3.sha256" 2>/dev/null || true

echo "Resultados dos checksums em: $LOGDIR"
cat "$LOGDIR/seeder.sha256" 2>/dev/null || true
cat "$LOGDIR/peer2.sha256" 2>/dev/null || true
cat "$LOGDIR/peer3.sha256" 2>/dev/null || true

if [ -f "$LOGDIR/seeder.sha256" ] && [ -f "$LOGDIR/peer2.sha256" ] && [ -f "$LOGDIR/peer3.sha256" ]; then
  cmp --silent "$LOGDIR/seeder.sha256" "$LOGDIR/peer2.sha256" && cmp --silent "$LOGDIR/seeder.sha256" "$LOGDIR/peer3.sha256"
  if [ $? -eq 0 ]; then
    echo "Verificação OK: arquivos idênticos ao Seeder.";
  else
    echo "Verificação FALHOU: arquivos diferentes.";
  fi
else
  echo "Não foi possível gerar checksums para todos os peers.";
fi

echo "Finalizando peers (se ainda estiverem ativos)..."
kill $PID1 $PID2 $PID3 2>/dev/null || true

echo "Logs: $LOGDIR"
#!/bin/bash
set -euo pipefail


ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_EXE="$ROOT_DIR/build/peer"

if [ ! -x "$BUILD_EXE" ]; then
  echo "Compilando projeto..."
  make
fi

LOGDIR="$ROOT_DIR/build/logs"
mkdir -p "$LOGDIR"

echo "Iniciando Peer 1 (Seeder)..."
"$BUILD_EXE" 1 1 > "$LOGDIR/peer1.log" 2>&1 &
PID1=$!
sleep 1

echo "Iniciando Peer 2 (Leecher)..."
"$BUILD_EXE" 2 0 > "$LOGDIR/peer2.log" 2>&1 &
PID2=$!
sleep 1

echo "Iniciando Peer 3 (Leecher)..."
"$BUILD_EXE" 3 0 > "$LOGDIR/peer3.log" 2>&1 &
PID3=$!

echo "Peers iniciados (PIDs): $PID1 $PID2 $PID3"
echo "Aguardando conclusão dos downloads (timeout 300s)..."

TIMEOUT=300
START=$SECONDS
while true; do
  if grep -q "DOWNLOAD COMPLETO" "$LOGDIR/peer2.log" && grep -q "DOWNLOAD COMPLETO" "$LOGDIR/peer3.log"; then
    echo "Downloads completos detectados nos leechers.";
    break
  fi
  if [ $((SECONDS - START)) -ge $TIMEOUT ]; then
    echo "Timeout atingido ($TIMEOUT s) esperando downloads.";
    break
  fi
  sleep 2
done

echo "Gerando checksums..."
sha256sum peer_1_files/shared_file.dat > "$LOGDIR/seeder.sha256" 2>/dev/null || true
sha256sum peer_2_files/shared_file.dat > "$LOGDIR/peer2.sha256" 2>/dev/null || true
sha256sum peer_3_files/shared_file.dat > "$LOGDIR/peer3.sha256" 2>/dev/null || true

echo "Resultados dos checksums em: $LOGDIR"
cat "$LOGDIR/seeder.sha256" 2>/dev/null || true
cat "$LOGDIR/peer2.sha256" 2>/dev/null || true
cat "$LOGDIR/peer3.sha256" 2>/dev/null || true

if [ -f "$LOGDIR/seeder.sha256" ] && [ -f "$LOGDIR/peer2.sha256" ] && [ -f "$LOGDIR/peer3.sha256" ]; then
  cmp --silent "$LOGDIR/seeder.sha256" "$LOGDIR/peer2.sha256" && cmp --silent "$LOGDIR/seeder.sha256" "$LOGDIR/peer3.sha256"
  if [ $? -eq 0 ]; then
    echo "Verificação OK: arquivos idênticos ao Seeder.";
  else
    echo "Verificação FALHOU: arquivos diferentes.";
  fi
else
  echo "Não foi possível gerar checksums para todos os peers.";
fi

echo "Finalizando peers (se ainda estiverem ativos)..."
kill $PID1 $PID2 $PID3 2>/dev/null || true

echo "Logs: $LOGDIR"
