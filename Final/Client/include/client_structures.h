#ifndef CLIENT_STRUCTURES_H
#define CLIENT_STRUCTURES_H

#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#define TOTAL_FRAMES 240
#define OBSTACLE_FRAME_START 59
#define OBSTACLE_FRAME_END 222



// Sensor data structure
typedef struct {
    int frame_number;
    double altitude;
    double speed;
    double latitude;
    double longitude;
    time_t timestamp;
    bool is_valid;
} SensorData;

// Video packet structure (matches server)
typedef struct {
    int frame_id;
    char frame_path[256];
    int frame_width;
    int frame_height;
    SensorData sensor;
} VideoPacket;

// Client state for managing received data
typedef struct {
    VideoPacket latest_packet;
    int total_received;
    bool new_data;
    bool system_active;
    pthread_mutex_t data_mutex;
} ClientState;

// Configuration
#define UDP_PORT 8888

#endif

