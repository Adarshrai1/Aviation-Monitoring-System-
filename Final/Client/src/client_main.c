#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ncurses.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "../include/client_structures.h"


// External C++ OpenCV function
#ifdef __cplusplus
extern "C" {
#endif
    void* opencv_video_player_thread(void* arg);
#ifdef __cplusplus
}
#endif

#define UDP_FRAME_PORT 8889
#define CHUNK_SIZE 1024
#define MAX_CHUNKS 200

typedef struct {
    int frame_num;
    int chunk_id;
    int total_chunks;
    int chunk_size;
    char data[CHUNK_SIZE];
} FrameChunk;

typedef struct {
    char data[MAX_CHUNKS * CHUNK_SIZE];
    int chunks_received;
    int total_chunks;
    bool complete;
} FrameBuffer;

ClientState client_state;
char latest_frame_path[256] = "Waiting...";
FrameBuffer frame_buffers[TOTAL_FRAMES];

// Calculate distance between two GPS coordinates using Haversine formula
double calculate_distance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000.0; // Earth radius in meters
    
    // Convert to radians
    double lat1_rad = lat1 * M_PI / 180.0;
    double lat2_rad = lat2 * M_PI / 180.0;
    double delta_lat = (lat2 - lat1) * M_PI / 180.0;
    double delta_lon = (lon2 - lon1) * M_PI / 180.0;
    
    // Haversine formula
    double a = sin(delta_lat / 2.0) * sin(delta_lat / 2.0) +
               cos(lat1_rad) * cos(lat2_rad) *
               sin(delta_lon / 2.0) * sin(delta_lon / 2.0);
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    
    return R * c; // Distance in meters
}

void* udp_receiver_thread(void* arg) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    printf("[UDP-Meta] Listening on port %d...\n", UDP_PORT);
    
    while (client_state.system_active) {
        VideoPacket packet;
        recv(sock, &packet, sizeof(VideoPacket), 0);
        
        pthread_mutex_lock(&client_state.data_mutex);
        client_state.latest_packet = packet;
        client_state.total_received++;
        client_state.new_data = true;
        pthread_mutex_unlock(&client_state.data_mutex);
    }
    
    close(sock);
    return NULL;
}

void* udp_frame_receiver_thread(void* arg) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_FRAME_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    printf("[UDP-Frame] Listening on port %d...\n", UDP_FRAME_PORT);
    
    system("mkdir -p received_frames");
    
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        frame_buffers[i].chunks_received = 0;
        frame_buffers[i].total_chunks = 0;
        frame_buffers[i].complete = false;
    }
    
    while (client_state.system_active) {
        FrameChunk chunk;
        ssize_t received = recv(sock, &chunk, sizeof(FrameChunk), 0);
        
        if (received > 0) {
            int frame_idx = chunk.frame_num - 1;
            if (frame_idx >= 0 && frame_idx < TOTAL_FRAMES) {
                FrameBuffer* fb = &frame_buffers[frame_idx];
                
                if (fb->total_chunks == 0) {
                    fb->total_chunks = chunk.total_chunks;
                }
                
                int offset = chunk.chunk_id * CHUNK_SIZE;
                memcpy(fb->data + offset, chunk.data, chunk.chunk_size);
                fb->chunks_received++;
                
                if (fb->chunks_received >= fb->total_chunks && !fb->complete) {
                    fb->complete = true;
                    
                    char filename[256];
                    snprintf(filename, sizeof(filename), "received_frames/frame_%03d.jpg", chunk.frame_num);
                    
                    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
                    if (fd >= 0) {
                        int total_size = (fb->total_chunks - 1) * CHUNK_SIZE + chunk.chunk_size;
                        write(fd, fb->data, total_size);
                        close(fd);
                        
                        pthread_mutex_lock(&client_state.data_mutex);
                        snprintf(latest_frame_path, sizeof(latest_frame_path), "%s", filename);
                        pthread_mutex_unlock(&client_state.data_mutex);
                    }
                }
            }
        }
    }
    
    close(sock);
    return NULL;
}

void* ui_thread(void* arg) {
    printf("[UI] Waiting for first data packet from server...\n");
    bool first_data_received = false;

    // Persistent obstacle coordinates and tracking
    static double obstacle_end_lat = 28.5222;
    static double obstacle_end_lon = 77.2222;
    double last_obstacle_distance = -1.0;

    while (!first_data_received && client_state.system_active) {
        pthread_mutex_lock(&client_state.data_mutex);
        if (client_state.total_received > 0) {
            first_data_received = true;
        }
        pthread_mutex_unlock(&client_state.data_mutex);
        usleep(100000);
    }

    if (!client_state.system_active) return NULL;

    printf("[UI] ✓ Data received, starting UI display...\n");
    sleep(1);

    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_RED, COLOR_BLACK);
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(6, COLOR_WHITE, COLOR_RED);
    }

    double obstacle_start_alt = 1000.0 + (OBSTACLE_FRAME_START * 5.2);
    double obstacle_start_speed = 250.0 + (OBSTACLE_FRAME_START * 1.04);
    double obstacle_start_lat = 28.5000 + (OBSTACLE_FRAME_START * 0.0001);
    double obstacle_start_lon = 77.2000 + (OBSTACLE_FRAME_START * 0.0001);
    double obstacle_end_alt = 1000.0 + (OBSTACLE_FRAME_END * 5.2);
    double obstacle_end_speed = 250.0 + (OBSTACLE_FRAME_END * 1.04);

    time_t obstacle_entry_time = 0;
    time_t obstacle_exit_time = 0;
    bool obstacle_entered = false;
    bool obstacle_exited = false;

    while (client_state.system_active) {
        pthread_mutex_lock(&client_state.data_mutex);
        VideoPacket pkt = client_state.latest_packet;
        int total = client_state.total_received;
        char frame_path[256];
        strcpy(frame_path, latest_frame_path);
        pthread_mutex_unlock(&client_state.data_mutex);

        if (pkt.frame_id == OBSTACLE_FRAME_START && !obstacle_entered) {
            obstacle_entry_time = time(NULL);
            obstacle_entered = true;
        }
        if (pkt.frame_id == OBSTACLE_FRAME_END && !obstacle_exited) {
            obstacle_exit_time = time(NULL);
            obstacle_exited = true;
        }

        erase();

        // Header
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(0, 0, "***********************************************************");
        mvprintw(1, 0, "      AVIATION RECEIVER - OPENCV LIVE STREAM               ");
        mvprintw(2, 0, "***********************************************************");
        attroff(COLOR_PAIR(1) | A_BOLD);

        // Status
        attron(COLOR_PAIR(5));
        mvprintw(4, 2, "✓ OpenCV Window: Live Video Stream Running");
        attroff(COLOR_PAIR(5));

        // Current Flight Data
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(6, 2, "CURRENT FLIGHT DATA:");
        attroff(COLOR_PAIR(2) | A_BOLD);

        attron(COLOR_PAIR(2));
        mvprintw(7, 4, "Frame: %d/240 | Packets: %d", pkt.frame_id, total);
        mvprintw(8, 4, "Altitude: %.1fm | Speed: %.1f km/h", pkt.sensor.altitude, pkt.sensor.speed);
        mvprintw(9, 4, "GPS: %.6f, %.6f", pkt.sensor.latitude, pkt.sensor.longitude);
        mvprintw(10, 4, "Saved Frame: %s", frame_path);
        attroff(COLOR_PAIR(2));

        // Obstacle Zone Information
        if (total >= OBSTACLE_FRAME_START) {
            attron(COLOR_PAIR(3) | A_BOLD);
            mvprintw(12, 2, "OBSTACLE ZONE INFORMATION:");
            attroff(COLOR_PAIR(3) | A_BOLD);

            attron(COLOR_PAIR(6) | A_BOLD);
          //  mvprintw(13, 4, " WARNING: Obstacle detected in frame %d ", OBSTACLE_FRAME_START);
            attroff(COLOR_PAIR(6) | A_BOLD);

            attron(COLOR_PAIR(4));
            mvprintw(15, 4, "Obstacle Entry Point (Frame %d):", OBSTACLE_FRAME_START);
            mvprintw(16, 6, "Altitude: %.1fm", obstacle_start_alt);
            mvprintw(17, 6, "Speed: %.1f km/h", obstacle_start_speed);
            mvprintw(18, 6, "GPS: %.6f, %.6f", obstacle_start_lat, obstacle_start_lon);

            if (obstacle_entered && obstacle_entry_time > 0) {
                struct tm* entry_tm = localtime(&obstacle_entry_time);
                char entry_time_str[32];
                strftime(entry_time_str, sizeof(entry_time_str), "%H:%M:%S", entry_tm);
                mvprintw(19, 6, "Detected at: %s", entry_time_str);
            } else {
                mvprintw(19, 6, "Detected at: --:--:--");
            }

            // Dynamic distance calculation
            bool in_obstacle_zone = pkt.frame_id >= OBSTACLE_FRAME_START && pkt.frame_id <=   OBSTACLE_FRAME_END;
            double current_distance = -1.0;

            if (in_obstacle_zone) {
                double step_size = 0.00005;
                if (obstacle_end_lat > pkt.sensor.latitude) obstacle_end_lat -= step_size;
                else if (obstacle_end_lat < pkt.sensor.latitude) obstacle_end_lat += step_size;
                if (obstacle_end_lon > pkt.sensor.longitude) obstacle_end_lon -= step_size;
                else if (obstacle_end_lon < pkt.sensor.longitude) obstacle_end_lon += step_size;

                current_distance = calculate_distance(pkt.sensor.latitude, pkt.sensor.longitude, obstacle_end_lat, obstacle_end_lon);
                last_obstacle_distance = current_distance;

                mvprintw(27, 4, "Obstacle Zone Distance:");
                mvprintw(28, 6, "%.1f meters (%.2f km)", current_distance, current_distance / 1000.0);
            } else if (pkt.frame_id > OBSTACLE_FRAME_END && last_obstacle_distance > 0) {
                mvprintw(27, 4, "Last Known Obstacle Distance:");
                mvprintw(28, 6, "%.1f meters (%.2f km)", last_obstacle_distance, last_obstacle_distance / 1000.0);
            }

            // Current status
            if (in_obstacle_zone) {
                attron(COLOR_PAIR(3) | A_BOLD | A_BLINK);
                mvprintw(30, 4, " CURRENTLY IN OBSTACLE ZONE - FRAME %d ", pkt.frame_id);
                mvprintw(31, 4, "Check OpenCV window for visual alert!");
                attroff(COLOR_PAIR(3) | A_BOLD | A_BLINK);
            } else {
                attron(COLOR_PAIR(2));
                mvprintw(30, 4, "Status: Clear - No obstacles in current frame");
                attroff(COLOR_PAIR(2));
            }
        }
        if (total >= OBSTACLE_FRAME_START) {
            //mvprintw(13, 4, " WARNING: obstacle was cleared in frame %d ", OBSTACLE_FRAME_END);
            attroff(COLOR_PAIR(6) | A_BOLD);
            }
            if (total >= OBSTACLE_FRAME_END) {
            attron(COLOR_PAIR(4));
            mvprintw(21, 4, "Obstacle Exit Point (Frame %d):", OBSTACLE_FRAME_END);
            mvprintw(22, 6, "Altitude: %.1fm", obstacle_end_alt);
            mvprintw(23, 6, "Speed: %.1f km/h", obstacle_end_speed);
            mvprintw(24, 6, "GPS: %.6f, %.6f", obstacle_end_lat, obstacle_end_lon);

            if (obstacle_exited && obstacle_exit_time > 0) {
                struct tm* exit_tm = localtime(&obstacle_exit_time);
                char exit_time_str[32];
                strftime(exit_time_str, sizeof(exit_time_str), "%H:%M:%S", exit_tm);
                mvprintw(25, 6, "Cleared at: %s", exit_time_str);
            } else {
                mvprintw(25, 6, "Cleared at: --:--:--");
            }
            }
        // Footer
                // Footer
        mvprintw(LINES - 3, 0, "***********************************************************");
        mvprintw(LINES - 2, 0, "OpenCV: Press 'q' or ESC in video window | TUI: 'q' quit, 'v' view  frame");
        mvprintw(LINES - 1, 0, "Obstacle zone: Frames %d-%d | Current: Frame %d", OBSTACLE_FRAME_START, OBSTACLE_FRAME_END, pkt.frame_id);
        
        refresh();

        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            client_state.system_active = false;
        } else if (ch == 'v' || ch == 'V') {
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "eog %s &", frame_path);
            system(cmd);
        }

        usleep(100000);  // Delay to reduce CPU usage
    }

    // Clean up and exit thread
    endwin();
    return NULL;
}

int main() {
    printf("\n***********************************************************\n");
    printf("    AVIATION CLIENT - OPENCV LIVE STREAM               \n");
    printf("    Ports: 8888 (meta) + 8889 (frames) + 9000 (video) \n");
    printf("***********************************************************\n\n");
    
    client_state.system_active = true;
    client_state.new_data = false;
    client_state.total_received = 0;
    pthread_mutex_init(&client_state.data_mutex, NULL);
    
    client_state.latest_packet.frame_id = 0;
    client_state.latest_packet.frame_width = 320;
    client_state.latest_packet.frame_height = 240;
    
    printf("Creating receiver threads...\n");
    
    pthread_t udp_thread, frame_thread, video_player, ui;
    pthread_create(&udp_thread, NULL, udp_receiver_thread, NULL);
    pthread_create(&frame_thread, NULL, udp_frame_receiver_thread, NULL);
    pthread_create(&video_player, NULL, opencv_video_player_thread, NULL);
    pthread_create(&ui, NULL, ui_thread, NULL);
    
    printf("✓ All threads started\n");
    printf("✓ OpenCV window will open shortly...\n");
    printf("✓ TUI will start when data is received...\n\n");
    
    pthread_join(ui, NULL);
    client_state.system_active = false;
    pthread_join(udp_thread, NULL);
    pthread_join(frame_thread, NULL);
    pthread_join(video_player, NULL);
    
    pthread_mutex_destroy(&client_state.data_mutex);
    
    printf("\n***********************************************************\n");
    printf("    AVIATION CLIENT SHUTDOWN COMPLETE                   \n");
    printf("    Total packets received: %-28d\n", client_state.total_received);
    printf("***********************************************************\n\n");
    
    return 0;
}

