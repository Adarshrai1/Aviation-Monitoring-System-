#include "../include/aviation_system.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define UDP_VIDEO_PORT 9000
#define MAX_PACKET 65000

void* video_streamer_thread(void* arg) {
    SystemState* state = (SystemState*)arg;
    SharedMemory* shm = state->shm;
    
    while (!shm->frames_extracted && shm->system_active) {
        usleep(100000);
    }
    
    if (!shm->system_active) return NULL;
    
    printf("[VideoStreamer] Starting OpenCV video stream on port %d\n", UDP_VIDEO_PORT);
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[VideoStreamer] Socket creation failed");
        return NULL;
    }
    
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_VIDEO_PORT);
    dest_addr.sin_addr.s_addr = inet_addr(CLIENT_IP);
    
    printf("[VideoStreamer] Streaming to %s:%d\n", CLIENT_IP, UDP_VIDEO_PORT);
    
    for (int frame = 1; frame <= TOTAL_FRAMES && shm->system_active; frame++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "resources/frames/frame_%03d.jpg", frame);
        
        int fd = open(filename, O_RDONLY);
        if (fd < 0) continue;
        
        struct stat st;
        fstat(fd, &st);
        int filesize = st.st_size;
        
        if (filesize > MAX_PACKET) {
            close(fd);
            continue;
        }
        
        char buffer[MAX_PACKET];
        int bytes_read = read(fd, buffer, filesize);
        close(fd);
        
        if (bytes_read > 0) {
            sendto(sock, buffer, bytes_read, 0,
                   (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        }
        
        usleep(125000);  // 8 FPS
    }
    
    printf("[VideoStreamer] Streaming complete\n");
    close(sock);
    return NULL;
}

