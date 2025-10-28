#!/bin/bash
# Save as: ~/Documents/P.roject/server/launch_system.sh

set -e

BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}  AVIATION SYSTEM - UNIFIED LAUNCHER           ${NC}"
echo -e "${BLUE}================================================${NC}\n"

# Get directories
SERVER_DIR="$(cd "$(dirname "$0")" && pwd)"
MONITOR_DIR="$(dirname "$SERVER_DIR")/monitor"

# ============================================
# STEP 1: Start Aviation Server (Background)
# ============================================
echo -e "${GREEN}[1/3] Starting Aviation Server...${NC}"
cd "$SERVER_DIR"

if [ ! -f "aviation_server" ]; then
    echo -e "${YELLOW}Building server...${NC}"
    make clean && make
fi

# Start server in background
./aviation_server &
SERVER_PID=$!
echo -e "${GREEN}✓ Server started (PID: $SERVER_PID)${NC}"
echo -e "${GREEN}✓ Web dashboard: http://localhost:8080${NC}\n"

# Wait for server to initialize
sleep 2

# ============================================
# STEP 2: Start Aviation Monitor (Foreground)
# ============================================
echo -e "${GREEN}[2/3] Starting Aviation Monitor...${NC}"
cd "$MONITOR_DIR"

if [ ! -f "aviation_monitor" ]; then
    echo -e "${YELLOW}Building monitor...${NC}"
    make clean && make
fi

echo -e "${GREEN}✓ Monitor starting (video display + UDP streaming)${NC}"
echo -e "${YELLOW}Press Ctrl+C to stop both server and monitor${NC}\n"

# Function to cleanup on exit
cleanup() {
    echo -e "\n${YELLOW}Shutting down system...${NC}"
    kill $SERVER_PID 2>/dev/null || true
    echo -e "${GREEN}✓ System stopped${NC}"
    exit 0
}

trap cleanup SIGINT SIGTERM

# Start monitor in foreground
./aviation_monitor

# If monitor exits, cleanup
cleanup

