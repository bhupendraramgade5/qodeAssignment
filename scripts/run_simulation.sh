#!/bin/bash
set -e

# ---------------- CONFIG ----------------
ROOT_DIR="$(cd .. && pwd)"
BIN_DIR="$ROOT_DIR"
CONFIG_DIR="$ROOT_DIR/configs"

SIM_BIN="$BIN_DIR/main"
FEED_BIN="$BIN_DIR/feedhandler"

SIM_CPU=2
CLIENT_CPUS=(3 4 5 6)

LOG_DIR="$ROOT_DIR/scripts/logs"
mkdir -p "$LOG_DIR"

# ---------------- CLEANUP ----------------
cleanup() {
    echo
    echo "[INFO] Stopping all processes..."
    pkill -P $$ 2>/dev/null || true
    exit 0
}
trap cleanup SIGINT SIGTERM

# ---------------- START SIMULATOR ----------------
echo "[INFO] Starting Exchange Simulator on CPU $SIM_CPU"

cd "$ROOT_DIR"
taskset -c "$SIM_CPU" "$SIM_BIN" \
    > "$LOG_DIR/simulator.log" 2>&1 &

sleep 2

# ---------------- START FEED HANDLERS ----------------
echo "[INFO] Starting FeedHandlers..."

for CPU in "${CLIENT_CPUS[@]}"; do
    echo "  → FeedHandler on CPU $CPU"
    cd "$ROOT_DIR"
    taskset -c "$CPU" "$FEED_BIN" \
        > "$LOG_DIR/feedhandler_cpu${CPU}.log" 2>&1 &
    sleep 0.3
done

# ---------------- STATUS ----------------
echo
echo "[INFO] All processes running"
echo "[INFO] Logs in $LOG_DIR"
echo "[INFO] Press Ctrl+C to stop"

wait
