#include "../include/aviation_system.h"

void* frame_simulator_thread(void* arg) {
    SharedMemory* shm = (SharedMemory*)arg;
    
    printf("[FrameSimulator] Started - Simulating %d frames at %d FPS\n", TOTAL_FRAMES, FPS);
    
    for (int i = 1; i <= TOTAL_FRAMES && shm->system_active; i++) {
        // Update current frame
        pthread_mutex_lock(&shm->frame_mutex);
        shm->current_frame = i;
        shm->total_frames_processed = i;
        pthread_mutex_unlock(&shm->frame_mutex);
        
        // Progress updates every 30 frames
        if (i % 30 == 0) {
            printf("[FrameSimulator] Frame %d/%d (%.1f%%) - Dashboard updated\n", 
                   i, TOTAL_FRAMES, (i * 100.0) / TOTAL_FRAMES);
        }
        
        // Simulate 8 FPS (125ms per frame)
        usleep(125000);
    }
    
    printf("[FrameSimulator] ✓ Completed all %d frames\n", TOTAL_FRAMES);
    printf("[FrameSimulator] Loop mode: Restarting from frame 1...\n");
    
    // Loop mode - keep cycling through frames
    while (shm->system_active) {
        for (int i = 1; i <= TOTAL_FRAMES && shm->system_active; i++) {
            pthread_mutex_lock(&shm->frame_mutex);
            shm->current_frame = i;
            pthread_mutex_unlock(&shm->frame_mutex);
            usleep(125000);
        }
    }
    
    printf("[FrameSimulator] Stopped\n");
    return NULL;
}

