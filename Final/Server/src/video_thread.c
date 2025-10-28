#include "../include/aviation_system.h"
#include <opencv2/opencv.hpp>

using namespace cv;

extern "C" void* video_acquisition_thread(void* arg) {
    SystemState* state = (SystemState*)arg;
    SharedMemory* shm = state->shm;

    printf("[VideoThread] Starting - ONE-TIME playback mode\n");

    while (!shm->frames_extracted && shm->system_active) {
        usleep(100000);
    }
    
    if (!shm->frames_extracted) {
        printf("[VideoThread] ERROR: No frames available\n");
        return NULL;
    }

    printf("[VideoThread] Transmitting frames 1-%d (NO LOOP)\n\n", TOTAL_FRAMES);

    for (int i = 1; i <= TOTAL_FRAMES; i++) {
        if (!shm->system_active) break;
        
        pthread_mutex_lock(&shm->frame_mutex);
        shm->current_frame = i;
        shm->total_frames_processed = i;
        pthread_mutex_unlock(&shm->frame_mutex);

        VideoPacket packet;
        packet.frame_id = i;
        snprintf(packet.frame_path, sizeof(packet.frame_path), 
                 "%s/frame_%03d.ppm", FRAMES_DIR, i);
        packet.frame_width = 320;
        packet.frame_height = 240;

        pthread_mutex_lock(&shm->sensor_mutex);
        if (i - 1 < TOTAL_FRAMES) {
            packet.sensor = shm->frame_sensors[i - 1];
        }
        pthread_mutex_unlock(&shm->sensor_mutex);

        send_video_packet_udp(state->udp_socket, &packet);

        if (i % 10 == 0) {
            printf("[VideoThread] %d/%d (%.1f%%)\n", i, TOTAL_FRAMES, (i*100.0)/TOTAL_FRAMES);
        }

        usleep(125000);
    }

    printf("\n");
    printf("════════════════════════════════════════\n");
    printf("  ✓✓✓ COMPLETE - %d FRAMES SENT ✓✓✓\n", TOTAL_FRAMES);
    printf("  Counter locked at: %d\n", TOTAL_FRAMES);
    printf("  NO FURTHER UPDATES\n");
    printf("════════════════════════════════════════\n\n");

    while (shm->system_active) {
        sleep(3600);
    }

    return NULL;
}

