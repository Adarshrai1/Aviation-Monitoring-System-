#ifndef CONFIG_H
#define CONFIG_H

#define VIDEO_PATH "./resources/flightfeed.mp4"
#define FRAMES_DIR "./resources/frames/"
#define TOTAL_FRAMES 240
#define FPS 8
#define VIDEO_DURATION 30
#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240
#define UDP_PORT 8888
#define UDP_BUFFER_SIZE 65536
#define LOCALHOST "127.0.0.1"
#define CLIENT_IP "192.168.1.110"
#define SHM_NAME "/aviation_shm"
#define SEM_FRAME_READY "/sem_frame_ready"
#define SEM_PROCESSING_DONE "/sem_processing_done"
#define BASE_ALTITUDE 3500.0
#define BASE_SPEED 450.0
#define BASE_LAT 28.6139
#define BASE_LON 77.2090
#define OBSTACLE_FRAME_1 59
#define OBSTACLE_FRAME_2 222
#define OBSTACLE_FRAME OBSTACLE_FRAME_1
#define WATCHDOG_CHECK_INTERVAL 2
#define WATCHDOG_MAX_WARNINGS 3
#define NUM_PIPELINE_STAGES 3
#define PIPELINE_STAGE1_NAME "Image stabilization"
#define PIPELINE_STAGE2_NAME "Noise reduction"
#define PIPELINE_STAGE3_NAME "Overlay sensor data"

#endif

