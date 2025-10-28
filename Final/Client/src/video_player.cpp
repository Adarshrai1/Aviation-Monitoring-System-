#include <opencv2/opencv.hpp>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <pthread.h>
#include "../include/client_structures.h"
#define UDP_VIDEO_PORT 9000
#define BUFFER_SIZE 65536

// C linkage for pthread compatibility
extern "C" {
    void* opencv_video_player_thread(void* arg);
}

void* opencv_video_player_thread(void* arg) {
    // Create UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "[OpenCV] ERROR: Socket creation failed" << std::endl;
        return nullptr;
    }
    
    // Configure socket address
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_VIDEO_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    // Bind socket
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[OpenCV] ERROR: Bind failed on port " << UDP_VIDEO_PORT << std::endl;
        close(sock);
        return nullptr;
    }
    
    std::cout << "[OpenCV] ✓ Video player listening on port " << UDP_VIDEO_PORT << std::endl;
    std::cout << "[OpenCV] ✓ Waiting for video stream from server..." << std::endl;
    
    // Create OpenCV window
    cv::namedWindow("Aviation Live Stream", cv::WINDOW_AUTOSIZE);
    cv::moveWindow("Aviation Live Stream", 100, 100);
    
    int frame_count = 0;
    bool first_frame = true;
    
    while (true) {
        char buffer[BUFFER_SIZE];
        ssize_t received = recv(sock, buffer, BUFFER_SIZE, 0);
        
        if (received > 0) {
            // Decode JPEG image from UDP packet
            std::vector<uchar> data(buffer, buffer + received);
            cv::Mat frame = cv::imdecode(data, cv::IMREAD_COLOR);
            
            if (!frame.empty()) {
                frame_count++;
                
                if (first_frame) {
                    std::cout << "[OpenCV] ✓ First frame received! Displaying video..." << std::endl;
                    first_frame = false;
                }
                
                // Add green header text
                cv::putText(frame, "AVIATION LIVE STREAM", 
                           cv::Point(10, 30), 
                           cv::FONT_HERSHEY_SIMPLEX, 
                           1.0, 
                           cv::Scalar(0, 255, 0),  // Green
                           2);
                
                // Add frame counter
                char frame_text[50];
                sprintf(frame_text, "Frame: %d", frame_count);
                cv::putText(frame, frame_text, 
                           cv::Point(10, 60), 
                           cv::FONT_HERSHEY_SIMPLEX, 
                           0.8, 
                           cv::Scalar(255, 255, 255),  // White
                           2);
                
                // Add timestamp
                char timestamp[50];
                time_t now = time(0);
                struct tm* timeinfo = localtime(&now);
                strftime(timestamp, 50, "%H:%M:%S", timeinfo);
                cv::putText(frame, timestamp, 
                           cv::Point(10, frame.rows - 10), 
                           cv::FONT_HERSHEY_SIMPLEX, 
                           0.6, 
                           cv::Scalar(255, 255, 0),  // Yellow
                           1);
                
                // Add alert for obstacle frames (80-81)
                if (frame_count >= OBSTACLE_FRAME_START && frame_count <= OBSTACLE_FRAME_END) {
                    cv::putText(frame, "!! OBSTACLE DETECTED !!", 
                               cv::Point(10, 100), 
                               cv::FONT_HERSHEY_SIMPLEX, 
                               0.9, 
                               cv::Scalar(0, 0, 255),  // Red
                               2);
                }
                
                // Display the frame
                cv::imshow("Aviation Live Stream", frame);
                
                // Check for user input (q or ESC to quit)
                int key = cv::waitKey(1) & 0xFF;
                if (key == 'q' || key == 27) {
                    std::cout << "[OpenCV] User quit video window (pressed 'q' or ESC)" << std::endl;
                    break;
                }
            }
        }
    }
    
    // Cleanup
    cv::destroyAllWindows();
    close(sock);
    std::cout << "[OpenCV] Video window closed cleanly" << std::endl;
    std::cout << "[OpenCV] Total frames displayed: " << frame_count << std::endl;
    
    return nullptr;
}

