#include "../include/aviation_system.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define WEB_PORT 8080
#define BUFFER_SIZE 4096
#define HTML_BUFFER_SIZE 16384

typedef struct {
    int entry_frame;
    int exit_frame;
    double entry_time;
    double exit_time;
    double duration;
    double distance;
    SensorData entry_sensor;
    SensorData exit_sensor;
} ObstacleMetrics;

void generate_dashboard_html(SharedMemory* shm, char* html_buffer, size_t buffer_size) {
    pthread_mutex_lock(&shm->sensor_mutex);
    SensorData current = shm->current_sensor;
    pthread_mutex_unlock(&shm->sensor_mutex);
    
    pthread_mutex_lock(&shm->detection_mutex);
    DetectionResult detection = shm->latest_detection;
    bool has_detection = shm->new_detection;
    pthread_mutex_unlock(&shm->detection_mutex);
    
    pthread_mutex_lock(&shm->frame_mutex);
    int current_frame = shm->current_frame;
    int total_processed = shm->total_frames_processed;
    pthread_mutex_unlock(&shm->frame_mutex);
    
    ObstacleMetrics metrics = {0};
    if (has_detection) {
        metrics.entry_frame = OBSTACLE_FRAME_1;
        metrics.exit_frame = OBSTACLE_FRAME_2;
        metrics.entry_time = (double)OBSTACLE_FRAME_1 / FPS;
        metrics.exit_time = (double)OBSTACLE_FRAME_2 / FPS;
        metrics.duration = metrics.exit_time - metrics.entry_time;
        
        pthread_mutex_lock(&shm->sensor_mutex);
        metrics.entry_sensor = shm->frame_sensors[OBSTACLE_FRAME_1 - 1];
        if (current_frame >= OBSTACLE_FRAME_2) {
            metrics.exit_sensor = shm->frame_sensors[OBSTACLE_FRAME_2 - 1];
        }
        pthread_mutex_unlock(&shm->sensor_mutex);
        
        double avg_speed = (metrics.entry_sensor.speed + metrics.exit_sensor.speed) / 2.0;
        metrics.distance = (avg_speed / 3600.0) * metrics.duration;
    }
    
    const char* status_color = has_detection ? "#dc2626" : "#10b981";
    const char* status_text = has_detection ? "ALERT" : "NORMAL";
    
    bool playback_complete = (current_frame >= TOTAL_FRAMES);
    const char* completion_status = "";
    if (playback_complete) {
        status_text = "CLEAR";
        status_color = "#3b82f6";
        completion_status = " | Complete";
    }
    
    html_buffer[0] = '\0';
    char temp_buffer[8192];
    
    snprintf(temp_buffer, sizeof(temp_buffer),
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <meta charset='UTF-8'>\n"
        "    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n"
        "%s"
        "    <title>Aviation Monitoring%s</title>\n"
        "    <style>\n"
        "        * { margin: 0; padding: 0; box-sizing: border-box; }\n"
        "        body {\n"
        "            font-family: 'Courier New', monospace;\n"
        "            background: linear-gradient(135deg, #1e293b 0%%, #0f172a 100%%);\n"
        "            color: #e2e8f0;\n"
        "            padding: 20px;\n"
        "            min-height: 100vh;\n"
        "        }\n"
        "        .container { max-width: 1400px; margin: 0 auto; }\n"
        "        .header {\n"
        "            background: linear-gradient(135deg, #1e40af 0%%, #3b82f6 100%%);\n"
        "            padding: 30px;\n"
        "            border-radius: 12px;\n"
        "            margin-bottom: 25px;\n"
        "            box-shadow: 0 10px 30px rgba(0,0,0,0.3);\n"
        "            border: 2px solid #3b82f6;\n"
        "        }\n"
        "        .header h1 {\n"
        "            font-size: 32px;\n"
        "            font-weight: bold;\n"
        "            text-align: center;\n"
        "            text-transform: uppercase;\n"
        "            letter-spacing: 3px;\n"
        "            text-shadow: 2px 2px 4px rgba(0,0,0,0.5);\n"
        "        }\n"
        "        .status-bar {\n"
        "            display: flex;\n"
        "            justify-content: space-between;\n"
        "            align-items: center;\n"
        "            margin-top: 15px;\n"
        "            padding-top: 15px;\n"
        "            border-top: 2px solid rgba(255,255,255,0.2);\n"
        "        }\n"
        "        .status-badge {\n"
        "            background: %s;\n"
        "            color: white;\n"
        "            padding: 8px 20px;\n"
        "            border-radius: 20px;\n"
        "            font-weight: bold;\n"
        "            font-size: 14px;\n"
        "            box-shadow: 0 4px 10px rgba(0,0,0,0.3);\n"
        "            %s\n"
        "        }\n"
        "        @keyframes pulse {\n"
        "            0%%, 100%% { opacity: 1; }\n"
        "            50%% { opacity: 0.7; }\n"
        "        }\n"
        "        .grid {\n"
        "            display: grid;\n"
        "            grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));\n"
        "            gap: 20px;\n"
        "            margin-bottom: 25px;\n"
        "        }\n"
        "        .card {\n"
        "            background: rgba(30, 41, 59, 0.9);\n"
        "            border: 2px solid #334155;\n"
        "            border-radius: 10px;\n"
        "            padding: 25px;\n"
        "            box-shadow: 0 8px 20px rgba(0,0,0,0.4);\n"
        "            transition: transform 0.2s, border-color 0.2s;\n"
        "        }\n"
        "        .card:hover {\n"
        "            transform: translateY(-5px);\n"
        "            border-color: #3b82f6;\n"
        "        }\n"
        "        .card-title {\n"
        "            font-size: 18px;\n"
        "            font-weight: bold;\n"
        "            color: #60a5fa;\n"
        "            margin-bottom: 20px;\n"
        "            padding-bottom: 10px;\n"
        "            border-bottom: 2px solid #334155;\n"
        "            text-transform: uppercase;\n"
        "            letter-spacing: 1px;\n"
        "        }\n"
        "        .data-row {\n"
        "            display: flex;\n"
        "            justify-content: space-between;\n"
        "            padding: 12px 0;\n"
        "            border-bottom: 1px solid #334155;\n"
        "        }\n"
        "        .data-row:last-child { border-bottom: none; }\n"
        "        .data-label {\n"
        "            color: #94a3b8;\n"
        "            font-size: 14px;\n"
        "        }\n"
        "        .data-value {\n"
        "            color: #e2e8f0;\n"
        "            font-weight: bold;\n"
        "            font-size: 16px;\n"
        "        }\n"
        "        .alert-card {\n"
        "            background: linear-gradient(135deg, #991b1b 0%%, #dc2626 100%%);\n"
        "            border-color: #ef4444;\n"
        "            animation: alertPulse 1.5s infinite;\n"
        "        }\n"
        "        @keyframes alertPulse {\n"
        "            0%%, 100%% { box-shadow: 0 8px 20px rgba(220, 38, 38, 0.4); }\n"
        "            50%% { box-shadow: 0 8px 30px rgba(220, 38, 38, 0.8); }\n"
        "        }\n"
        "        .metric-large {\n"
        "            font-size: 36px;\n"
        "            font-weight: bold;\n"
        "            color: #fbbf24;\n"
        "            text-align: center;\n"
        "            margin: 15px 0;\n"
        "            text-shadow: 2px 2px 4px rgba(0,0,0,0.5);\n"
        "        }\n"
        "        .distance-highlight {\n"
        "            color: #fbbf24;\n"
        "            font-size: 20px;\n"
        "            font-weight: bold;\n"
        "            animation: blink 1s infinite;\n"
        "        }\n"
        "        @keyframes blink {\n"
        "            0%%, 100%% { opacity: 1; }\n"
        "            50%% { opacity: 0.6; }\n"
        "        }\n"
        "        .no-data {\n"
        "            text-align: center;\n"
        "            color: #64748b;\n"
        "            font-style: italic;\n"
        "            padding: 20px;\n"
        "        }\n"
        "        .footer {\n"
        "            text-align: center;\n"
        "            margin-top: 30px;\n"
        "            padding-top: 20px;\n"
        "            border-top: 2px solid #334155;\n"
        "            color: #64748b;\n"
        "            font-size: 12px;\n"
        "        }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class='container'>\n",
        playback_complete ? "" : "    <meta http-equiv='refresh' content='2'>\n",
        playback_complete ? " - COMPLETED" : "",
        status_color,
        playback_complete ? "" : "animation: pulse 2s infinite;"
    );
    strncat(html_buffer, temp_buffer, buffer_size - strlen(html_buffer) - 1);
    
    snprintf(temp_buffer, sizeof(temp_buffer),
        "        <div class='header'>\n"
        "            <h1>✈️ AVIATION MONITORING SYSTEM</h1>\n"
        "            <div class='status-bar'>\n"
        "                <div>Frame: %d / %d | Processed: %d%s</div>\n"
        "                <div class='status-badge'>STATUS: %s</div>\n"
        "                <div>%s</div>\n"
        "            </div>\n"
        "        </div>\n",
        current_frame, TOTAL_FRAMES, total_processed, completion_status, status_text,
        playback_complete ? "Refresh: STOPPED" : "Auto-refresh: 2s"
    );
    strncat(html_buffer, temp_buffer, buffer_size - strlen(html_buffer) - 1);
    
    snprintf(temp_buffer, sizeof(temp_buffer),
        "\n"
        "        <div class='grid'>\n"
        "            <div class='card'>\n"
        "                <div class='card-title'>📡 CURRENT FLIGHT DATA</div>\n"
        "                <div class='data-row'>\n"
        "                    <span class='data-label'>Altitude:</span>\n"
        "                    <span class='data-value'>%.0f m</span>\n"
        "                </div>\n"
        "                <div class='data-row'>\n"
        "                    <span class='data-label'>Speed:</span>\n"
        "                    <span class='data-value'>%.0f km/h</span>\n"
        "                </div>\n"
        "                <div class='data-row'>\n"
        "                    <span class='data-label'>Latitude:</span>\n"
        "                    <span class='data-value'>%.6f°N</span>\n"
        "                </div>\n"
        "                <div class='data-row'>\n"
        "                    <span class='data-label'>Longitude:</span>\n"
        "                    <span class='data-value'>%.6f°E</span>\n"
        "                </div>\n"
        "                <div class='data-row'>\n"
        "                    <span class='data-label'>Frame Time:</span>\n"
        "                    <span class='data-value'>%.2f s</span>\n"
        "                </div>\n"
        "            </div>\n"
        "        </div>\n",
        current.altitude, current.speed, current.latitude, current.longitude,
        (double)current_frame / FPS
    );
    strncat(html_buffer, temp_buffer, buffer_size - strlen(html_buffer) - 1);
    
    if (has_detection) {
        double remaining_distance = 0.0;
        if (current_frame >= OBSTACLE_FRAME_1 && current_frame < OBSTACLE_FRAME_2) {
            // FIXED LINEAR DISTANCE: Starts at 5 km, decreases slowly
            int total_obstacle_frames = OBSTACLE_FRAME_2 - OBSTACLE_FRAME_1;
            int frames_remaining = OBSTACLE_FRAME_2 - current_frame;
            
            double initial_distance = 5.0;  // Start at 5 km
            remaining_distance = initial_distance * ((double)frames_remaining / total_obstacle_frames);
            
            if (remaining_distance < 0) remaining_distance = 0;
        }
        
        snprintf(temp_buffer, sizeof(temp_buffer),
            "\n"
            "        <div class='grid'>\n"
            "            <div class='card alert-card'>\n"
            "                <div class='card-title'>⚠️ OBSTACLE DETECTED</div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>Detection Type:</span>\n"
            "                    <span class='data-value'>%s</span>\n"
            "                </div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>Frame Number:</span>\n"
            "                    <span class='data-value'>%d</span>\n"
            "                </div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>Detection Time:</span>\n"
            "                    <span class='data-value'>%.2f seconds</span>\n"
            "                </div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>Altitude at Detection:</span>\n"
            "                    <span class='data-value'>%.0f m</span>\n"
            "                </div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>Speed at Detection:</span>\n"
            "                    <span class='data-value'>%.0f km/h</span>\n"
            "                </div>\n",
            detection.detection_type, detection.frame_number,
            (double)detection.frame_number / FPS,
            detection.sensor_snapshot.altitude, detection.sensor_snapshot.speed
        );
        
        if (current_frame >= OBSTACLE_FRAME_1 && current_frame < OBSTACLE_FRAME_2) {
            char distance_row[512];
            snprintf(distance_row, sizeof(distance_row),
                "                <div class='data-row'>\n"
                "                    <span class='data-label'>🎯 Distance of Obstacle:</span>\n"
                "                    <span class='distance-highlight'>%.3f km ⬇</span>\n"
                "                </div>\n",
                remaining_distance
            );
            strncat(temp_buffer, distance_row, sizeof(temp_buffer) - strlen(temp_buffer) - 1);
        }
        
        strncat(temp_buffer, "            </div>\n        </div>\n", sizeof(temp_buffer) - strlen(temp_buffer) - 1);
        
    } else {
        snprintf(temp_buffer, sizeof(temp_buffer),
            "\n"
            "        <div class='grid'>\n"
            "            <div class='card'>\n"
            "                <div class='card-title'>⚠️ OBSTACLE DETECTION</div>\n"
            "                <div class='no-data'>No obstacles detected</div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>Monitoring Frames:</span>\n"
            "                    <span class='data-value'>%d, %d</span>\n"
            "                </div>\n"
            "            </div>\n"
            "        </div>\n",
            OBSTACLE_FRAME_1, OBSTACLE_FRAME_2
        );
    }
    strncat(html_buffer, temp_buffer, buffer_size - strlen(html_buffer) - 1);
    
    if (current_frame >= OBSTACLE_FRAME_1 && has_detection) {
        snprintf(temp_buffer, sizeof(temp_buffer),
            "\n"
            "        <div class='grid'>\n"
            "            <div class='card'>\n"
            "                <div class='card-title'>📍 OBSTACLE ENTRY (Frame %d)</div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>Entry Time:</span>\n"
            "                    <span class='data-value'>%.3f s</span>\n"
            "                </div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>Altitude:</span>\n"
            "                    <span class='data-value'>%.0f m</span>\n"
            "                </div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>Speed:</span>\n"
            "                    <span class='data-value'>%.0f km/h</span>\n"
            "                </div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>GPS:</span>\n"
            "                    <span class='data-value'>%.4f, %.4f</span>\n"
            "                </div>\n"
            "            </div>\n",
            metrics.entry_frame, metrics.entry_time,
            metrics.entry_sensor.altitude, metrics.entry_sensor.speed,
            metrics.entry_sensor.latitude, metrics.entry_sensor.longitude
        );
        strncat(html_buffer, temp_buffer, buffer_size - strlen(html_buffer) - 1);
    }
    
    if (current_frame >= OBSTACLE_FRAME_2 && has_detection) {
        snprintf(temp_buffer, sizeof(temp_buffer),
            "\n"
            "            <div class='card'>\n"
            "                <div class='card-title'>📍 OBSTACLE EXIT (Frame %d)</div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>Exit Time:</span>\n"
            "                    <span class='data-value'>%.3f s</span>\n"
            "                </div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>Altitude:</span>\n"
            "                    <span class='data-value'>%.0f m</span>\n"
            "                </div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>Speed:</span>\n"
            "                    <span class='data-value'>%.0f km/h</span>\n"
            "                </div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>GPS:</span>\n"
            "                    <span class='data-value'>%.4f, %.4f</span>\n"
            "                </div>\n"
            "            </div>\n",
            metrics.exit_frame, metrics.exit_time,
            metrics.exit_sensor.altitude, metrics.exit_sensor.speed,
            metrics.exit_sensor.latitude, metrics.exit_sensor.longitude
        );
        strncat(html_buffer, temp_buffer, buffer_size - strlen(html_buffer) - 1);
        
        double avg_speed = (metrics.entry_sensor.speed + metrics.exit_sensor.speed) / 2.0;
        snprintf(temp_buffer, sizeof(temp_buffer),
            "\n"
            "            <div class='card'>\n"
            "                <div class='card-title'>📊 OBSTACLE METRICS</div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>Duration:</span>\n"
            "                    <span class='data-value'>%.3f seconds</span>\n"
            "                </div>\n"
            "                <div class='metric-large'>%.3f s</div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>Distance Traveled:</span>\n"
            "                    <span class='data-value'>%.3f km</span>\n"
            "                </div>\n"
            "                <div class='data-row'>\n"
            "                    <span class='data-label'>Avg Speed:</span>\n"
            "                    <span class='data-value'>%.0f km/h</span>\n"
            "                </div>\n"
            "            </div>\n"
            "        </div>\n",
            metrics.duration, metrics.duration, metrics.distance, avg_speed
        );
        strncat(html_buffer, temp_buffer, buffer_size - strlen(html_buffer) - 1);
    }
    
    const char* footer = 
        "\n"
        "        <div class='footer'>\n"
        "            Aviation Monitoring System v1.0 | Real-time Dashboard | "
        "UDP Streaming Active on Ports 8888, 8889, 9000\n"
        "        </div>\n"
        "    </div>\n"
        "</body>\n"
        "</html>\n";
    strncat(html_buffer, footer, buffer_size - strlen(html_buffer) - 1);
}

void* web_server_thread(void* arg) {
    SharedMemory* shm = (SharedMemory*)arg;
    
    int server_fd, client_fd;
    struct sockaddr_in addr;
    char buffer[BUFFER_SIZE];
    char* html_response = malloc(HTML_BUFFER_SIZE);
    
    if (!html_response) {
        fprintf(stderr, "[WebServer] Memory allocation failed\n");
        return NULL;
    }
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[WebServer] Socket creation failed");
        free(html_response);
        return NULL;
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(WEB_PORT);
    
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[WebServer] Bind failed");
        close(server_fd);
        free(html_response);
        return NULL;
    }
    
    if (listen(server_fd, 10) < 0) {
        perror("[WebServer] Listen failed");
        close(server_fd);
        free(html_response);
        return NULL;
    }
    
    printf("[WebServer] Dashboard running on http://localhost:%d\n", WEB_PORT);
    
    while (shm->system_active) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;
        
        read(client_fd, buffer, sizeof(buffer));
        generate_dashboard_html(shm, html_response, HTML_BUFFER_SIZE);
        
        char http_header[512];
        snprintf(http_header, sizeof(http_header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "Cache-Control: no-cache, no-store, must-revalidate\r\n"
            "Pragma: no-cache\r\n"
            "Expires: 0\r\n"
            "\r\n",
            strlen(html_response)
        );
        
        write(client_fd, http_header, strlen(http_header));
        write(client_fd, html_response, strlen(html_response));
        
        close(client_fd);
    }
    
    close(server_fd);
    free(html_response);
    printf("[WebServer] Stopped\n");
    return NULL;
}

