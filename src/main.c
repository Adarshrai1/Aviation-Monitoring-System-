#include "../include/aviation_system.h"

int main() {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║   AVIATION SERVER - COMPLETE STREAMING MODE              ║\n");
    printf("║  UDP: 8888 (meta) | 8889 (frames) | 9000 (video)         ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    SharedMemory* shm = init_shared_memory();
    if (!shm) {
        fprintf(stderr, "[ERROR] Failed to create shared memory\n");
        return 1;
    }
    
    init_signal_handlers(shm);
    initialize_sensor_data(shm);
    
    printf("[Server] Extracting frames from video...\n");
    if (!extract_frames_from_video(shm)) {
        fprintf(stderr, "[ERROR] Frame extraction failed\n");
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
    
    printf("[Server] Starting 7 threads...\n\n");
    
    pthread_create(&state.threads[0], NULL, sensor_data_thread, shm);
    pthread_create(&state.threads[1], NULL, video_acquisition_thread, &state);
    pthread_create(&state.threads[2], NULL, detection_thread, shm);
    pthread_create(&state.threads[3], NULL, processing_pipeline_thread, shm);
    pthread_create(&state.threads[4], NULL, watchdog_thread, shm);
    pthread_create(&state.threads[5], NULL, frame_sender_thread, &state);
    pthread_create(&state.threads[6], NULL, video_streamer_thread, &state);
    
    sleep(2);
    
    printf("[Server] All threads started\n");
    printf("[Server] Streaming to %s on 3 ports\n", CLIENT_IP);
    printf("[Server] Press Ctrl+C to stop...\n\n");
    
    while (shm->system_active) {
        sleep(1);
    }
    
    printf("\n[Server] Shutting down...\n");
    shm->system_active = false;
    
    for (int i = 0; i < 7; i++) {
        pthread_join(state.threads[i], NULL);
    }
    
    close(udp_socket);
    cleanup_shared_memory(shm);
    
    printf("[Server] Shutdown complete\n\n");
    return 0;
}

