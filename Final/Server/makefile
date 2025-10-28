CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -pthread -I./include
CXXFLAGS = -Wall -Wextra -pthread -I./include `pkg-config --cflags opencv4`
LDFLAGS = -pthread -lrt -lncurses `pkg-config --libs opencv4`

C_SOURCES = src/main.c \
            src/shared_memory.c \
            src/sensor_thread.c \
            src/detection_thread.c \
            src/processing_pipeline.c \
            src/signal_watchdog.c \
            src/udp_communication.c \
            src/ui_terminal.c \
            src/frame_sender.c \
            src/video_streamer.c \
            src/web_server.c

CXX_SOURCES = src/video_thread.c

C_OBJECTS = $(C_SOURCES:.c=.o)
CXX_OBJECTS = $(CXX_SOURCES:.c=.o)
TARGET = aviation_server

all: $(TARGET)

$(TARGET): $(C_OBJECTS) $(CXX_OBJECTS)
	$(CXX) $(C_OBJECTS) $(CXX_OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "✓ Build complete: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

src/video_thread.o: src/video_thread.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(C_OBJECTS) $(CXX_OBJECTS) $(TARGET)
	rm -f /dev/shm/aviation_shm

.PHONY: all clean

