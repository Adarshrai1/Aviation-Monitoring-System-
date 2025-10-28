#include "../include/aviation_system.h"

void* sensor_data_thread(void* arg) {
    SharedMemory* shm = (SharedMemory*)arg;
    
    printf("[SensorThread] Started - Monitoring current frame\n");
    
    while (!shm->frames_extracted && shm->system_active) {
        usleep(100000);
    }
    
    if (!shm->frames_extracted) {
        printf("[SensorThread] No frames available, exiting\n");
        return NULL;
    }
    
    printf("[SensorThread] ✓ Frames ready, starting frame progression\n");
    
    int last_frame = 0;
    
    // Monitor frames but DON'T set them - video_thread.c does that
    while (shm->system_active) {
        pthread_mutex_lock(&shm->frame_mutex);
        int current_frame = shm->current_frame;
        pthread_mutex_unlock(&shm->frame_mutex);
        
        // Exit when we reach frame 240
        if (current_frame >= TOTAL_FRAMES) {
            printf("[SensorThread] Reached frame %d - STOPPING\n", TOTAL_FRAMES);
            break;
        }
        
        // Update sensor data for current frame
        if (current_frame != last_frame && current_frame > 0 && current_frame <= TOTAL_FRAMES) {
            pthread_mutex_lock(&shm->sensor_mutex);
            shm->current_sensor = shm->frame_sensors[current_frame - 1];
            shm->current_sensor.frame_number = current_frame;
            pthread_mutex_unlock(&shm->sensor_mutex);
            
            if (current_frame % 30 == 0) {
                printf("[SensorThread] Frame %d/%d (%.1f%%)\n", 
                       current_frame, TOTAL_FRAMES, (current_frame * 100.0) / TOTAL_FRAMES);
            }
            
            last_frame = current_frame;
        }
        
        usleep(50000);  // Check every 50ms
    }
    
    // Sleep forever after completion
    while (shm->system_active) {
        sleep(3600);
    }
    
    printf("[SensorThread] Stopped\n");
    return NULL;
}

