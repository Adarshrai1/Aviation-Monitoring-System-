#include "../include/aviation_system.h"

void* pipeline_stage1(void* arg) {
    SharedMemory* shm = (SharedMemory*)arg;
    
    while (shm->system_active) {
        sleep(5);  // Sleep quietly
    }
    
    return NULL;
}

void* pipeline_stage2(void* arg) {
    SharedMemory* shm = (SharedMemory*)arg;
    
    while (shm->system_active) {
        sleep(5);  // Sleep quietly
    }
    
    return NULL;
}

void* pipeline_stage3(void* arg) {
    SharedMemory* shm = (SharedMemory*)arg;
    
    while (shm->system_active) {
        sleep(5);  // Sleep quietly
        
        pthread_mutex_lock(&shm->frame_mutex);
        shm->processing_complete = true;
        pthread_mutex_unlock(&shm->frame_mutex);
    }
    
    return NULL;
}

void* processing_pipeline_thread(void* arg) {
    SharedMemory* shm = (SharedMemory*)arg;
    
    printf("[ProcessingPipeline] Starting 3-stage pipeline (silent mode)\n");
    
    pthread_t stage_threads[3];
    
    pthread_create(&stage_threads[0], NULL, pipeline_stage1, shm);
    pthread_create(&stage_threads[1], NULL, pipeline_stage2, shm);
    pthread_create(&stage_threads[2], NULL, pipeline_stage3, shm);
    
    for (int i = 0; i < 3; i++) {
        pthread_join(stage_threads[i], NULL);
    }
    
    printf("[ProcessingPipeline] Stopped\n");
    return NULL;
}

