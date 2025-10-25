CC = gcc
CXX = g++
CFLAGS = -Wall -pthread -I./include
CXXFLAGS = -Wall -pthread -I./include
OPENCV = `pkg-config --cflags --libs opencv4`
LIBS = -lpthread -lrt -lm -lncurses

SOURCES = src/main.c \
          src/shared_memory.c \
          src/sensor_thread.c \
          src/processing_pipeline.c \
          src/detection_thread.c \
          src/udp_communication.c \
          src/signal_watchdog.c \
          src/ui_terminal.c \
          src/frame_sender.c \
          src/video_streamer.c

CPP_SOURCES = src/video_thread.c

TARGET = aviation_monitor

all: $(TARGET)

$(TARGET): $(SOURCES) $(CPP_SOURCES)
	@echo "╔════════════════════════════════════════════════════════════╗"
	@echo "║  Compiling Aviation Server with Video Streaming           ║"
	@echo "║  UDP: 8888 (meta) | 8889 (frames) | 9000 (video)          ║"
	@echo "╚════════════════════════════════════════════════════════════╝"
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(CPP_SOURCES) \
	       $(OPENCV) $(LIBS)
	@echo ""
	@echo "✓ Build complete!"
	@echo "✓ Run with: ./$(TARGET)"
	@echo ""

clean:
	rm -f $(TARGET)
	rm -rf ./resources/frames/*.ppm
	rm -rf ./resources/frames/*.jpg
	@echo "Cleaned build files and frames"

.PHONY: all clean

