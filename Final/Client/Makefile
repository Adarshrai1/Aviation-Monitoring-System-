# ═══════════════════════════════════════════════════════════
# Aviation Client Makefile with OpenCV Video Streaming
# ═══════════════════════════════════════════════════════════

# Compilers
CC = gcc
CXX = g++

# Compiler flags
CFLAGS = -Wall -Wextra -pthread -I./include -g
CXXFLAGS = -Wall -Wextra -pthread -I./include -std=c++11 -g

# Libraries
OPENCV = `pkg-config --cflags --libs opencv4`
LIBS = -lncurses -lpthread -lm

# Linker flags
LDFLAGS = -pthread

# Target executable
TARGET = client_receiver

# Source files
C_SOURCES = src/client_main.c
CPP_SOURCES = src/video_player.cpp

# Object files (not used in direct compilation, but defined for clarity)
C_OBJECTS = $(C_SOURCES:.c=.o)
CPP_OBJECTS = $(CPP_SOURCES:.cpp=.o)

# ═══════════════════════════════════════════════════════════
# Build Rules
# ═══════════════════════════════════════════════════════════

all: banner $(TARGET) success

banner:
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════╗"
	@echo "║  Compiling Aviation Client with OpenCV Video Stream       ║"
	@echo "║                                                            ║"
	@echo "║  Components:                                               ║"
	@echo "║    • UDP Metadata Receiver (Port 8888)                    ║"
	@echo "║    • UDP Frame Chunks Receiver (Port 8889)                ║"
	@echo "║    • OpenCV Live Video Player (Port 9000)                 ║"
	@echo "║    • ncurses Terminal UI                                  ║"
	@echo "╚════════════════════════════════════════════════════════════╝"
	@echo ""

# Link everything together with g++ (to handle C++ objects)
$(TARGET): $(C_SOURCES) $(CPP_SOURCES)
	@echo "→ Compiling C and C++ sources..."
	@echo "  • client_main.c (C code)"
	@echo "  • video_player.cpp (C++ code with OpenCV)"
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) \
		$(C_SOURCES) $(CPP_SOURCES) \
		$(OPENCV) $(LIBS)
	@echo "  ✓ Linking complete"

success:
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════╗"
	@echo "║  ✓ BUILD SUCCESSFUL                                        ║"
	@echo "╚════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "Run with:"
	@echo "  ./$(TARGET)"
	@echo ""
	@echo "Features:"
	@echo "  • OpenCV window will show live video stream"
	@echo "  • ncurses TUI shows sensor data and alerts"
	@echo "  • Frames saved to received_frames/ folder"
	@echo ""
	@echo "Controls:"
	@echo "  • Press 'q' or ESC in OpenCV window to close video"
	@echo "  • Press 'q' in TUI to quit application"
	@echo "  • Press 'v' in TUI to view saved frames"
	@echo ""

# ═══════════════════════════════════════════════════════════
# Clean Rules
# ═══════════════════════════════════════════════════════════

clean:
	@echo "Cleaning build artifacts..."
	@rm -f $(TARGET)
	@rm -f $(C_OBJECTS) $(CPP_OBJECTS)
	@rm -f src/*.o
	@echo "✓ Build artifacts cleaned"

clean-all: clean
	@echo "Cleaning all generated files..."
	@rm -rf received_frames/
	@rm -rf live_frames/
	@echo "✓ All generated files cleaned"

# ═══════════════════════════════════════════════════════════
# Utility Rules
# ═══════════════════════════════════════════════════════════

install-deps:
	@echo "Installing client dependencies..."
	@echo ""
	@echo "This will install:"
	@echo "  • OpenCV 4 development libraries"
	@echo "  • ncurses development libraries"
	@echo "  • Build essentials (gcc, g++, make)"
	@echo ""
	@read -p "Press Enter to continue or Ctrl+C to cancel..."
	sudo apt update
	sudo apt install -y libopencv-dev libncurses-dev build-essential pkg-config
	@echo ""
	@echo "✓ Dependencies installed successfully"
	@echo ""

test-opencv:
	@echo "Testing OpenCV installation..."
	@pkg-config --modversion opencv4 && echo "✓ OpenCV 4 found" || echo "✗ OpenCV 4 not found"
	@pkg-config --cflags opencv4 > /dev/null && echo "✓ OpenCV headers accessible" || echo "✗ OpenCV headers not found"
	@pkg-config --libs opencv4 > /dev/null && echo "✓ OpenCV libraries accessible" || echo "✗ OpenCV libraries not found"
	@echo ""

test-ncurses:
	@echo "Testing ncurses installation..."
	@ldconfig -p | grep ncurses > /dev/null && echo "✓ ncurses library found" || echo "✗ ncurses not found"
	@echo ""

test-all: test-opencv test-ncurses
	@echo "All dependency checks complete"
	@echo ""

# ═══════════════════════════════════════════════════════════
# Debug Rules
# ═══════════════════════════════════════════════════════════

debug: CFLAGS += -DDEBUG -O0
debug: CXXFLAGS += -DDEBUG -O0
debug: banner $(TARGET)
	@echo ""
	@echo "✓ Debug build complete (with -g -O0 flags)"
	@echo ""

# Run with valgrind for memory checking
valgrind: $(TARGET)
	@echo "Running with valgrind..."
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# ═══════════════════════════════════════════════════════════
# Help
# ═══════════════════════════════════════════════════════════

help:
	@echo "Aviation Client Makefile - Available targets:"
	@echo ""
	@echo "  make              - Build the client application"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make clean-all    - Remove all generated files and folders"
	@echo "  make install-deps - Install required dependencies"
	@echo "  make test-opencv  - Test OpenCV installation"
	@echo "  make test-ncurses - Test ncurses installation"
	@echo "  make test-all     - Test all dependencies"
	@echo "  make debug        - Build with debug symbols"
	@echo "  make valgrind     - Run with valgrind memory checker"
	@echo "  make help         - Show this help message"
	@echo ""

# ═══════════════════════════════════════════════════════════
# Phony Targets
# ═══════════════════════════════════════════════════════════

.PHONY: all clean clean-all install-deps test-opencv test-ncurses test-all \
        debug valgrind help banner success

