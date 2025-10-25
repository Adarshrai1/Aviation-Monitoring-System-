#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>

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
    float confidence;
    SensorData sensor_snapshot;
} DetectionResult;

// UDP packet for video frame (PPM format)
typedef struct {
    int frame_id;
    char frame_path[256];
    int frame_width;
    int frame_height;
    SensorData sensor;
} VideoPacket;

// Shared memory structure - UPDATED for 160 frames
typedef struct {
    // System state
    bool system_active;
    bool frames_extracted;
    int current_frame;
    int total_frames_processed;
    
    // Sensor data (protected by sensor_mutex) - EXPANDED ARRAY
    SensorData current_sensor;
    SensorData frame_sensors[200];  // Changed from 5 to 160
    
    // Detection data (protected by detection_mutex)
    DetectionResult latest_detection;
    bool new_detection;
    
    // Processing pipeline state
    bool frame_ready_for_processing;
    bool processing_complete;
    
    // Synchronization primitives
    pthread_mutex_t sensor_mutex;
    pthread_mutex_t detection_mutex;
    pthread_mutex_t frame_mutex;
    pthread_barrier_t processing_barrier;
    
    // Semaphores
    sem_t* sem_frame_ready;
    sem_t* sem_processing_done;
    
    // Condition variables
    pthread_cond_t ui_update_cond;
    pthread_mutex_t ui_mutex;
} SharedMemory;

// System state for main
typedef struct {
    SharedMemory* shm;
    int udp_socket;
    pthread_t threads[7];
} SystemState;

#endif

