#include "../include/aviation_system.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define UDP_FRAME_PORT 8889
#define CHUNK_SIZE 1024
#define MAX_CHUNKS 300

typedef struct {
    int frame_num;
    int chunk_id;
    int total_chunks;
    int chunk_size;
    char data[CHUNK_SIZE];
} FrameChunk;

void* frame_sender_thread(void* arg) {
    SystemState* state = (SystemState*)arg;
    SharedMemory* shm = state->shm;
    
    while (!shm->frames_extracted && shm->system_active) {
        usleep(100000);
    }
    
    if (!shm->system_active) return NULL;
    
    printf("[FrameSender] Starting UDP frame transmission on port %d\n", UDP_FRAME_PORT);
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[FrameSender] Socket creation failed");
        return NULL;
    }
    
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_FRAME_PORT);
    dest_addr.sin_addr.s_addr = inet_addr(CLIENT_IP);
    
    printf("[FrameSender] Sending frames to %s:%d\n", CLIENT_IP, UDP_FRAME_PORT);
    
    for (int frame = 1; frame <= TOTAL_FRAMES && shm->system_active; frame++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "resources/frames/frame_%03d.jpg", frame);
        
        int fd = open(filename, O_RDONLY);
        if (fd < 0) {
            printf("[FrameSender] Warning: Cannot open %s\n", filename);
            continue;
        }
        
        struct stat st;
        fstat(fd, &st);
        int filesize = st.st_size;
        
        // Calculate number of chunks needed
        int total_chunks = (filesize + CHUNK_SIZE - 1) / CHUNK_SIZE;
        if (total_chunks > MAX_CHUNKS) {
            printf("[FrameSender] Warning: Frame %d too large, skipping\n", frame);
            close(fd);
            continue;
        }
        
        // Send frame in chunks
        char buffer[CHUNK_SIZE];
        int chunk_id = 0;
        int bytes_read;
        
        while ((bytes_read = read(fd, buffer, CHUNK_SIZE)) > 0 && chunk_id < total_chunks) {
            FrameChunk chunk;
            chunk.frame_num = frame;
            chunk.chunk_id = chunk_id;
            chunk.total_chunks = total_chunks;
            chunk.chunk_size = bytes_read;
            memcpy(chunk.data, buffer, bytes_read);
            
            sendto(sock, &chunk, sizeof(FrameChunk), 0,
                   (struct sockaddr*)&dest_addr, sizeof(dest_addr));
            
            chunk_id++;
            usleep(1000);  // Small delay between chunks
        }
        
        close(fd);
        printf("[FrameSender] Sent frame %d (%d chunks)\n", frame, total_chunks);
        
        usleep(125000);  // 8 FPS delay
    }
    
    printf("[FrameSender] All frames sent\n");
    close(sock);
    return NULL;
}

