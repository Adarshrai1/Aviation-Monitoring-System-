#include "../include/aviation_system.h"

int init_udp_socket() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[UDP-Server] Socket creation failed");
        return -1;
    }
    
    printf("[UDP-Server] Socket created successfully\n");
    printf("[UDP-Server] Target client: %s:%d\n", CLIENT_IP, UDP_PORT);
    return sock;
}

void send_video_packet_udp(int socket_fd, VideoPacket* packet) {
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);
    dest_addr.sin_addr.s_addr = inet_addr(CLIENT_IP);
    
    ssize_t sent = sendto(socket_fd, packet, sizeof(VideoPacket), 0,
                          (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    
    if (sent < 0) {
        perror("[UDP-Server] Send failed");
    }
}

