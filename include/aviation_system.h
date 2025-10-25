#ifndef AVIATION_SYSTEM_H
#define AVIATION_SYSTEM_H

#include "structures.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Shared memory functions
SharedMemory* init_shared_memory();
void cleanup_shared_memory(SharedMemory* shm);

// Thread functions (C linkage)
#ifdef __cplusplus
extern "C" {
#endif

void* video_acquisition_thread(void* arg);
void* sensor_data_thread(void* arg);
void* frame_sender_thread(void* arg);
void* processing_pipeline_thread(void* arg);
void* detection_thread(void* arg);
void* watchdog_thread(void* arg);
void* video_streamer_thread(void* arg);


// Sensor initialization
void initialize_sensor_data(SharedMemory* shm);

// Frame extraction (C++ function)
bool extract_frames_from_video(SharedMemory* shm);

#ifdef __cplusplus
}
#endif

// UDP functions
int init_udp_socket();
void send_video_packet_udp(int socket_fd, VideoPacket* packet);
void receive_video_packet_udp(int socket_fd, VideoPacket* packet);

// Signal handling
void init_signal_handlers(SharedMemory* shm);
void signal_handler(int signum);

// UI function
void run_terminal_ui(SharedMemory* shm);

#endif


