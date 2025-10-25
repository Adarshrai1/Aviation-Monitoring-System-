#include "../include/aviation_system.h"

// Sensor data thread - monitors current frame and updates sensor readings
void* sensor_data_thread(void* arg) {
    SharedMemory* shm = (SharedMemory*)arg;
    
    printf("[SensorThread] Started - Monitoring current frame\n");
    
    while (shm->system_active) {
        pthread_mutex_lock(&shm->frame_mutex);
        int current = shm->current_frame;
        pthread_mutex_unlock(&shm->frame_mutex);
        
        // Update current sensor reading based on frame number
        if (current >= 0 && current < TOTAL_FRAMES) {
            pthread_mutex_lock(&shm->sensor_mutex);
            shm->current_sensor = shm->frame_sensors[current];
            shm->current_sensor.timestamp = time(NULL);
            pthread_mutex_unlock(&shm->sensor_mutex);
        }
        
        usleep(100000);  // Check every 100ms
    }
    
    printf("[SensorThread] Stopped\n");
    return NULL;
}

