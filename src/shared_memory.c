#include "../include/aviation_system.h"

SharedMemory* init_shared_memory() {
    printf("[SharedMemory] Initializing POSIX shared memory...\n");

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return NULL;
    }

    if (ftruncate(shm_fd, sizeof(SharedMemory)) == -1) {
        perror("ftruncate failed");
        return NULL;
    }

    SharedMemory* shm = (SharedMemory*)mmap(NULL, sizeof(SharedMemory),
                                           PROT_READ | PROT_WRITE,
                                           MAP_SHARED, shm_fd, 0);

    if (shm == MAP_FAILED) {
        perror("mmap failed");
        return NULL;
    }

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&shm->sensor_mutex, &mutex_attr);
    pthread_mutex_init(&shm->detection_mutex, &mutex_attr);
    pthread_mutex_init(&shm->frame_mutex, &mutex_attr);
    pthread_mutex_init(&shm->ui_mutex, &mutex_attr);

    pthread_mutexattr_destroy(&mutex_attr);

    pthread_barrierattr_t barrier_attr;
    pthread_barrierattr_init(&barrier_attr);
    pthread_barrierattr_setpshared(&barrier_attr, PTHREAD_PROCESS_SHARED);
    pthread_barrier_init(&shm->processing_barrier, &barrier_attr, 3);
    pthread_barrierattr_destroy(&barrier_attr);

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&shm->ui_update_cond, &cond_attr);
    pthread_condattr_destroy(&cond_attr);

    sem_unlink(SEM_FRAME_READY);
    sem_unlink(SEM_PROCESSING_DONE);

    shm->sem_frame_ready = sem_open(SEM_FRAME_READY, O_CREAT, 0644, 1);
    shm->sem_processing_done = sem_open(SEM_PROCESSING_DONE, O_CREAT, 0644, 0);

    if (shm->sem_frame_ready == SEM_FAILED || shm->sem_processing_done == SEM_FAILED) {
        perror("sem_open failed");
        return NULL;
    }

    shm->system_active = true;
    shm->frames_extracted = false;
    shm->current_frame = 0;
    shm->total_frames_processed = 0;
    shm->frame_ready_for_processing = false;
    shm->processing_complete = false;
    shm->new_detection = false;
    shm->current_sensor.is_valid = false;
    shm->latest_detection.obstacle_detected = false;

    printf("[SharedMemory] ✓ Shared memory initialized\n");
    printf("[SharedMemory] ✓ 4 Mutexes created\n");
    printf("[SharedMemory] ✓ Barrier created (3 threads)\n");
    printf("[SharedMemory] ✓ 2 Semaphores created\n");
    printf("[SharedMemory] ✓ Condition variable created\n\n");

    return shm;
}

// ✅ NEW FUNCTION: Initialize sensor data for all frames
void initialize_sensor_data(SharedMemory* shm) {
    printf("[SharedMemory] Initializing sensor data for %d frames...\n", TOTAL_FRAMES);
    
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        shm->frame_sensors[i].frame_number = i + 1;
        
        // Simulate realistic altitude changes (1000m to 2000m)
        shm->frame_sensors[i].altitude = 1000.0 + (i * 5.2);
        
        // Simulate realistic speed changes (250 km/h to 450 km/h)
        shm->frame_sensors[i].speed = 250.0 + (i * 1.04);
        
        // Simulate GPS coordinates changing (simulated flight path)
        shm->frame_sensors[i].latitude = 28.5000 + (i * 0.0001);
        shm->frame_sensors[i].longitude = 77.2000 + (i * 0.0001);
        
        shm->frame_sensors[i].timestamp = time(NULL);
        shm->frame_sensors[i].is_valid = true;
    }
    
    printf("[SharedMemory] ✓ Sensor data initialized for all %d frames\n", TOTAL_FRAMES);
    printf("[SharedMemory]   Altitude range: 1000m - %.0fm\n", 
           1000.0 + (TOTAL_FRAMES * 5.2));
    printf("[SharedMemory]   Speed range: 250 km/h - %.0f km/h\n", 
           250.0 + (TOTAL_FRAMES * 1.04));
    printf("[SharedMemory]   GPS: 28.5000 to %.4f (lat)\n\n", 
           28.5000 + (TOTAL_FRAMES * 0.0001));
}

void cleanup_shared_memory(SharedMemory* shm) {
    if (shm) {
        pthread_mutex_destroy(&shm->sensor_mutex);
        pthread_mutex_destroy(&shm->detection_mutex);
        pthread_mutex_destroy(&shm->frame_mutex);
        pthread_mutex_destroy(&shm->ui_mutex);
        pthread_barrier_destroy(&shm->processing_barrier);
        pthread_cond_destroy(&shm->ui_update_cond);

        sem_close(shm->sem_frame_ready);
        sem_close(shm->sem_processing_done);
        sem_unlink(SEM_FRAME_READY);
        sem_unlink(SEM_PROCESSING_DONE);

        munmap(shm, sizeof(SharedMemory));
        shm_unlink(SHM_NAME);

        printf("[SharedMemory] Cleaned up\n");
    }
}

