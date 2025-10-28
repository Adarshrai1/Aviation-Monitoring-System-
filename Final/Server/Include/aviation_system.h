#ifndef AVIATION_SYSTEM_H
#define AVIATION_SYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>

// Configuration constants - UPDATED FOR 240 FRAMES
#define FPS 8
#define TOTAL_FRAMES 240           // Changed from 160 to 240
#define VIDEO_DURATION 30          // Changed from 20 to 30 seconds
#define OBSTACLE_FRAME_1 59        // 10 seconds mark
#define OBSTACLE_FRAME_2 222        // Just after first detection

// Use absolute path to avoid GStreamer issues
#define VIDEO_PATH "/home/sys1/Documents/P.roject/server/resources/flight_feed.mp4"
#define FRAMES_DIR "/home/sys1/Documents/P.roject/server/resources/frames"

#define UDP_PORT 8888
#define CLIENT_IP "192.168.1.110"

// IPC identifiers
#define SHM_NAME "/aviation_shm"
#define SEM_FRAME_READY "/sem_frame_ready"
#define SEM_PROCESSING_DONE "/sem_processing_done"

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

// Detection result structure
typedef struct {
    int frame_number;
    bool obstacle_detected;
    char detection_type[64];
    double confidence;
    SensorData sensor_snapshot;
} DetectionResult;

// Video packet structure for UDP transmission
typedef struct {
    int frame_id;
    char frame_path[256];
    int frame_width;
    int frame_height;
    SensorData sensor;
} VideoPacket;

// Shared memory structure - UPDATED ARRAY SIZE
typedef struct {
    // System control
    bool system_active;
    bool frames_extracted;
    
    // Frame tracking
    int current_frame;
    int total_frames_processed;
    
    // Sensor data - UPDATED ARRAY SIZE
    SensorData current_sensor;
    SensorData frame_sensors[240];  // Changed from [TOTAL_FRAMES] to [240]
    
    // Detection results
    DetectionResult latest_detection;
    bool new_detection;
    
    // Processing flags
    bool frame_ready_for_processing;
    bool processing_complete;
    
    // Synchronization primitives
    pthread_mutex_t sensor_mutex;
    pthread_mutex_t detection_mutex;
    pthread_mutex_t frame_mutex;
    pthread_mutex_t ui_mutex;
    pthread_barrier_t processing_barrier;
    pthread_cond_t ui_update_cond;
    sem_t* sem_frame_ready;
    sem_t* sem_processing_done;
} SharedMemory;

// System state structure
typedef struct {
    SharedMemory* shm;
    int udp_socket;
} SystemState;

// C++ compatibility wrapper
#ifdef __cplusplus
extern "C" {
#endif

// Shared memory functions
SharedMemory* init_shared_memory();
void cleanup_shared_memory(SharedMemory* shm);
void initialize_sensor_data(SharedMemory* shm);

// Thread functions
void* sensor_data_thread(void* arg);
void* video_acquisition_thread(void* arg);
void* detection_thread(void* arg);
void* processing_pipeline_thread(void* arg);
void* watchdog_thread(void* arg);
void* frame_sender_thread(void* arg);
void* video_streamer_thread(void* arg);
void* web_server_thread(void* arg);
void* ui_terminal_thread(void* arg);

// Video extraction function (C++ implemented)
bool extract_frames_from_video(SharedMemory* shm);

// UDP communication functions
int init_udp_socket();
void send_video_packet_udp(int socket_fd, VideoPacket* packet);

// Signal handling
void init_signal_handlers(SharedMemory* shm);

#ifdef __cplusplus
}
#endif

#endif // AVIATION_SYSTEM_H

