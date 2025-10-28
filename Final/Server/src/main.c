#include "../include/aviation_system.h"

int main() {
    printf("\n");
    printf("***********************************************************\n");
    printf(" AVIATION SERVER - COMPLETE STREAMING MODE \n");
    printf(" UDP: 8888 (meta) | 8889 (frames) | 9000 (video) \n");
    printf(" WEB: 8080 (dashboard) - http://localhost:8080 \n");
    printf("***********************************************************\n");
    printf("\n");

    SharedMemory* shm = init_shared_memory();
    if (!shm) {
        fprintf(stderr, "[ERROR] Failed to create shared memory\n");
        return 1;
    }

    init_signal_handlers(shm);
    initialize_sensor_data(shm);

    printf("[Server] Checking for pre-extracted frames...\n");
    struct stat st = {0};
    char first_frame[256];
    snprintf(first_frame, sizeof(first_frame), "%s/frame_001.jpg", FRAMES_DIR);
    
    if (stat(first_frame, &st) == 0) {
        printf("[Server] ✓ Found existing frames, skipping extraction\n");
        printf("[Server] Using frames from: %s\n", FRAMES_DIR);
        shm->frames_extracted = true;
    } else {
        fprintf(stderr, "[ERROR] No frames found in %s\n", FRAMES_DIR);
        fprintf(stderr, "[SOLUTION] Run aviation_monitor first to extract frames:\n");
        fprintf(stderr, "  1. cd ../monitor && ./aviation_monitor\n");
        fprintf(stderr, "  2. Wait for extraction to complete\n");
        fprintf(stderr, "  3. cd ../server && ln -s ../monitor/resources/frames resources/frames\n");
        fprintf(stderr, "  4. ./aviation_server\n");
        cleanup_shared_memory(shm);
        return 1;
    }

    int udp_socket = init_udp_socket();
    if (udp_socket < 0) {
        fprintf(stderr, "[ERROR] UDP socket creation failed\n");
        cleanup_shared_memory(shm);
        return 1;
    }

    SystemState state;
    state.shm = shm;
    state.udp_socket = udp_socket;

    printf("[Server] Starting 8 threads (7 UDP + 1 Web)...\n\n");

    pthread_t threads[8];
    pthread_create(&threads[0], NULL, sensor_data_thread, shm);
    pthread_create(&threads[1], NULL, video_acquisition_thread, &state);
    pthread_create(&threads[2], NULL, detection_thread, shm);
    pthread_create(&threads[3], NULL, processing_pipeline_thread, shm);
    pthread_create(&threads[4], NULL, watchdog_thread, shm);
    pthread_create(&threads[5], NULL, frame_sender_thread, &state);
    pthread_create(&threads[6], NULL, video_streamer_thread, &state);
    pthread_create(&threads[7], NULL, web_server_thread, shm);

    // Wait for all threads
    for (int i = 0; i < 8; i++) {
        pthread_join(threads[i], NULL);
    }

    close(udp_socket);
    cleanup_shared_memory(shm);

    printf("\n[Server] Shutdown complete\n");
    return 0;
}

