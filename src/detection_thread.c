#include "../include/aviation_system.h"

void* detection_thread(void* arg) {
    SharedMemory* shm = (SharedMemory*)arg;
    
    printf("[DetectionThread] Started - Monitoring for obstacles\n");
    printf("[DetectionThread] Obstacle detection frames: %d and %d\n", 
           OBSTACLE_FRAME_1, OBSTACLE_FRAME_2);
    
    bool detected_frame1 = false;
    bool detected_frame2 = false;
    int last_checked_frame = 0;
    
    while (shm->system_active) {
        pthread_mutex_lock(&shm->frame_mutex);
        int current = shm->current_frame;
        pthread_mutex_unlock(&shm->frame_mutex);
        
        // Only process if frame has changed
        if (current != last_checked_frame && current > 0) {
            last_checked_frame = current;
            
            // First obstacle detection at frame 80 (10 seconds)
            if (current == OBSTACLE_FRAME_1 && !detected_frame1) {
                detected_frame1 = true;
                
                pthread_mutex_lock(&shm->detection_mutex);
                shm->latest_detection.frame_number = current;
                shm->latest_detection.obstacle_detected = true;
                strcpy(shm->latest_detection.detection_type, "Aircraft - First Detection");
                //shm->latest_detection.confidence = 0.92;
                
                pthread_mutex_lock(&shm->sensor_mutex);
                shm->latest_detection.sensor_snapshot = shm->current_sensor;
                pthread_mutex_unlock(&shm->sensor_mutex);
                
                shm->new_detection = true;
                pthread_mutex_unlock(&shm->detection_mutex);
                
                printf("\n");
                printf("╔════════════════════════════════════════════════════╗\n");
                printf("║  ⚠  AIRCRAFT #1 DETECTED AT FRAME %d (10.0s)  ⚠  ║\n", current);
                printf("╚════════════════════════════════════════════════════╝\n");
                //printf("  Confidence: 92%% | Type: Aircraft - First Detection\n");
                pthread_mutex_lock(&shm->sensor_mutex);
                printf("  Altitude: %.0fm | Speed: %.0fkm/h\n", 
                       shm->current_sensor.altitude,
                       shm->current_sensor.speed);
                pthread_mutex_unlock(&shm->sensor_mutex);
                printf("\n");
                
                pthread_mutex_lock(&shm->ui_mutex);
                pthread_cond_signal(&shm->ui_update_cond);
                pthread_mutex_unlock(&shm->ui_mutex);
            }
            
            // Second obstacle detection at frame 81 (10.125 seconds)
            if (current == OBSTACLE_FRAME_2 && !detected_frame2) {
                detected_frame2 = true;
                
                pthread_mutex_lock(&shm->detection_mutex);
                shm->latest_detection.frame_number = current;
                shm->latest_detection.obstacle_detected = true;
                strcpy(shm->latest_detection.detection_type, "Aircraft - Confirmed Detection");
                //shm->latest_detection.confidence = 0.97;
                
                pthread_mutex_lock(&shm->sensor_mutex);
                shm->latest_detection.sensor_snapshot = shm->current_sensor;
                pthread_mutex_unlock(&shm->sensor_mutex);
                
                shm->new_detection = true;
                pthread_mutex_unlock(&shm->detection_mutex);
                
                printf("\n");
                printf("╔════════════════════════════════════════════════════╗\n");
                printf("║  ⚠  AIRCRAFT #2 DETECTED AT FRAME %d (10.1s)  ⚠  ║\n", current);
                printf("╚════════════════════════════════════════════════════╝\n");
                printf("  Confidence: 97%% | Type: Aircraft - CONFIRMED\n");
                pthread_mutex_lock(&shm->sensor_mutex);
                printf("  Altitude: %.0fm | Speed: %.0fkm/h\n", 
                       shm->current_sensor.altitude,
                       shm->current_sensor.speed);
                pthread_mutex_unlock(&shm->sensor_mutex);
                printf("  *** IMMEDIATE EVASIVE ACTION REQUIRED ***\n");
                printf("\n");
                
                // Print summary of BOTH detections
                if (detected_frame1 && detected_frame2) {
                    printf("╔═══════════════════════════════════════════════════════╗\n");
                    printf("║         TWO OBSTACLES CONFIRMED - SUMMARY             ║\n");
                    printf("╚═══════════════════════════════════════════════════════╝\n");
                    printf("  Detection #1: Frame %d (10.0s) - Confidence 92%%\n", OBSTACLE_FRAME_1);
                    printf("  Detection #2: Frame %d (10.1s) - Confidence 97%%\n", OBSTACLE_FRAME_2);
                    printf("  Status: DOUBLE CONFIRMATION - CRITICAL ALERT\n");
                    printf("╚═══════════════════════════════════════════════════════╝\n\n");
                }
                
                pthread_mutex_lock(&shm->ui_mutex);
                pthread_cond_signal(&shm->ui_update_cond);
                pthread_mutex_unlock(&shm->ui_mutex);
            }
        }
        
        usleep(10000);  // Check every 10ms (faster polling to not miss frames)
    }
    
    printf("[DetectionThread] Stopped\n");
    return NULL;
}

