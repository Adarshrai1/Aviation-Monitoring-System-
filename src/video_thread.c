#include "../include/aviation_system.h"
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgcodecs.hpp>
#include <sys/stat.h>

using namespace cv;

extern "C" bool extract_frames_from_video(SharedMemory* shm) {
    printf("\n[VideoThread] Extracting frames at %d FPS from video...\n", FPS);
    
    struct stat st = {0};
    if (stat(FRAMES_DIR, &st) == -1) {
        mkdir(FRAMES_DIR, 0700);
    }
    
    VideoCapture capture(VIDEO_PATH);
    if (!capture.isOpened()) {
        fprintf(stderr, "[ERROR] Cannot open: %s\n", VIDEO_PATH);
        return false;
    }
    
    double video_fps = capture.get(CAP_PROP_FPS);
    int total_video_frames = (int)capture.get(CAP_PROP_FRAME_COUNT);
    int frame_skip = (int)(video_fps / FPS);  // Extract every Nth frame for 8 FPS
    
    printf("[VideoThread] Video FPS: %.2f | Target FPS: %d\n", video_fps, FPS);
    printf("[VideoThread] Frame skip interval: %d (extract every %d frames)\n", 
           frame_skip, frame_skip);
    printf("[VideoThread] Total frames to extract: %d\n", TOTAL_FRAMES);
    
    int extracted = 0;
    int frame_pos = 0;
    
    while (extracted < TOTAL_FRAMES && frame_pos < total_video_frames) {
        capture.set(CAP_PROP_POS_FRAMES, frame_pos);
        
        Mat frame;
        capture >> frame;
        
        if (frame.empty()) break;
        
        // Resize to 320x240 for UDP transfer
        Mat small_frame;
        resize(frame, small_frame, Size(320, 240));
        
        // Save as both PPM (for UDP) and JPG (for viewing)
        char ppm_file[256], jpg_file[256];
        snprintf(ppm_file, sizeof(ppm_file), "%sframe_%03d.ppm", FRAMES_DIR, extracted + 1);
        snprintf(jpg_file, sizeof(jpg_file), "%sframe_%03d.jpg", FRAMES_DIR, extracted + 1);
        
        imwrite(ppm_file, small_frame);
        imwrite(jpg_file, small_frame);
        
        // Progress update every 20 frames
        if ((extracted + 1) % 20 == 0) {
            printf("[VideoThread] ✓ Extracted %d/%d frames...\n", extracted + 1, TOTAL_FRAMES);
        }
        
        extracted++;
        frame_pos += frame_skip;
    }
    
    printf("[VideoThread] ✓ Total frames extracted: %d (PPM + JPG)\n\n", extracted);
    
    capture.release();
    shm->frames_extracted = true;
    return true;
}

void* video_acquisition_thread(void* arg) {
    SystemState* state = (SystemState*)arg;
    SharedMemory* shm = state->shm;
    
    printf("[VideoThread] Started - Transmitting %d frames via UDP at %d FPS\n", 
           TOTAL_FRAMES, FPS);
    
    // Send all frames sequentially at 8 FPS
    for (int i = 1; i <= TOTAL_FRAMES; i++) {
        if (!shm->system_active) break;
        
        pthread_mutex_lock(&shm->frame_mutex);
        shm->current_frame = i;
        shm->total_frames_processed++;
        pthread_mutex_unlock(&shm->frame_mutex);
        
        // Create UDP packet with frame info
        VideoPacket packet;
        packet.frame_id = i;
        snprintf(packet.frame_path, sizeof(packet.frame_path), 
                "%sframe_%03d.ppm", FRAMES_DIR, i);
        packet.frame_width = 320;
        packet.frame_height = 240;
        
        // Get sensor data for this frame
        pthread_mutex_lock(&shm->sensor_mutex);
        if (i - 1 < TOTAL_FRAMES) {
            packet.sensor = shm->frame_sensors[i - 1];
        }
        pthread_mutex_unlock(&shm->sensor_mutex);
        
        // Send frame via UDP
        send_video_packet_udp(state->udp_socket, &packet);
        
        // Progress update every 20 frames
        if (i % 20 == 0) {
            printf("[VideoThread] Transmitted %d/%d frames via UDP\n", i, TOTAL_FRAMES);
        }
        
        // Wait for next frame (125ms = 1000ms/8fps)
        usleep(125000);
    }
    
    printf("[VideoThread] All %d frames transmitted. Entering monitoring mode...\n", 
           TOTAL_FRAMES);
    
    // Keep thread alive but quiet
    while (shm->system_active) {
        sleep(10);
    }
    
    printf("[VideoThread] Stopped\n");
    return NULL;
}

