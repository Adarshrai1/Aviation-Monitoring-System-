#include "../include/aviation_system.h"
#include <ncurses.h>

#define MENU_ITEMS 5

void run_terminal_ui(SharedMemory* shm) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
    init_pair(5, COLOR_BLACK, COLOR_CYAN);
    
    int highlight = 0;
    bool running = true;
    
    const char *menu_items[MENU_ITEMS] = {
        "Show All Frames Data",
        "Show Obstacle Detection",
        "View Extracted Frames",
        "Play Full Video (VLC)",
        "Quit System"
    };
    
    while (running && shm->system_active) {
        clear();
        
        // Header
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(1, 2, "==============================================================");
        mvprintw(2, 2, "       AVIATION MONITORING SYSTEM - TUI INTERFACE            ");
        mvprintw(3, 2, "==============================================================");
        attroff(COLOR_PAIR(1) | A_BOLD);
        
        // Status bar
        pthread_mutex_lock(&shm->frame_mutex);
        int frame = shm->current_frame;
        int total = shm->total_frames_processed;
        pthread_mutex_unlock(&shm->frame_mutex);
        
        pthread_mutex_lock(&shm->sensor_mutex);
        double alt = shm->current_sensor.altitude;
        double speed = shm->current_sensor.speed;
        double lat = shm->current_sensor.latitude;
        double lon = shm->current_sensor.longitude;
        pthread_mutex_unlock(&shm->sensor_mutex);
        
        attron(COLOR_PAIR(2));
        mvprintw(5, 4, "Current Frame: %d/%d  |  Total Processed: %d", 
                 frame, TOTAL_FRAMES, total);
        mvprintw(6, 4, "Altitude: %.0fm  |  Speed: %.0fkm/h  |  GPS: %.4f, %.4f", 
                 alt, speed, lat, lon);
        attroff(COLOR_PAIR(2));
        
        mvprintw(8, 4, "--------------------------------------------------------------");
        attron(COLOR_PAIR(3));
        mvprintw(9, 4, "        Press 1-5 for instant selection, or use arrows       ");
        attroff(COLOR_PAIR(3));
        mvprintw(10, 4, "--------------------------------------------------------------");
        
        // Menu items with numbers
        for (int i = 0; i < MENU_ITEMS; i++) {
            if (i == highlight) {
                attron(COLOR_PAIR(5) | A_BOLD);
                mvprintw(12 + i*2, 8, "  >>  [%d] %s  <<  ", i+1, menu_items[i]);
                attroff(COLOR_PAIR(5) | A_BOLD);
            } else {
                mvprintw(12 + i*2, 8, "      [%d] %s      ", i+1, menu_items[i]);
            }
        }
        
        // Footer
        attron(COLOR_PAIR(3));
        mvprintw(LINES - 3, 2, "--------------------------------------------------------------");
        mvprintw(LINES - 2, 2, " UDP: Active  |  All IPC Mechanisms Running  |  Press 'q' to Quit");
        attroff(COLOR_PAIR(3));
        
        refresh();
        
        int c = getch();
        
        switch(c) {
            case KEY_UP:
                highlight = (highlight == 0) ? MENU_ITEMS - 1 : highlight - 1;
                break;
            case KEY_DOWN:
                highlight = (highlight == MENU_ITEMS - 1) ? 0 : highlight + 1;
                break;
            
            // NUMBER KEY SELECTION - INSTANT EXECUTION
            case '1':
                highlight = 0;
                goto execute_choice;
            case '2':
                highlight = 1;
                goto execute_choice;
            case '3':
                highlight = 2;
                goto execute_choice;
            case '4':
                highlight = 3;
                goto execute_choice;
            case '5':
                highlight = 4;
                goto execute_choice;
            
            case 10:  // ENTER
            execute_choice:
                if (highlight == 0) {
                    // Show all frames - Display summary (160 frames is too many)
                    clear();
                    attron(COLOR_PAIR(1) | A_BOLD);
                    mvprintw(1, 2, "==================== ALL FRAMES DATA SUMMARY ====================");
                    attroff(COLOR_PAIR(1) | A_BOLD);
                    
                    mvprintw(3, 4, "Total Frames: %d at %d FPS", TOTAL_FRAMES, FPS);
                    mvprintw(4, 4, "Duration: 20 seconds");
                    
                    mvprintw(6, 4, "--------------------------------------------------------------");
                    attron(COLOR_PAIR(2));
                    mvprintw(7, 4, "SAMPLE FRAMES:");
                    attroff(COLOR_PAIR(2));
                    mvprintw(8, 4, "--------------------------------------------------------------");
                    
                    pthread_mutex_lock(&shm->sensor_mutex);
                    
                    // Show frame 1
                    SensorData s1 = shm->frame_sensors[0];
                    mvprintw(10, 6, "FRAME 1 (0.0s) - Start");
                    mvprintw(11, 8, "Alt: %.0fm | Speed: %.0fkm/h | GPS: %.4f,%.4f", 
                            s1.altitude, s1.speed, s1.latitude, s1.longitude);
                    
                    // Show frame 80 (first obstacle)
                    SensorData s80 = shm->frame_sensors[79];
                    attron(COLOR_PAIR(4) | A_BOLD);
                    mvprintw(13, 6, "FRAME 59 (10.0s) - OBSTACLE #1 DETECTED");
                    attroff(COLOR_PAIR(4) | A_BOLD);
                    mvprintw(14, 8, "Alt: %.0fm | Speed: %.0fkm/h | GPS: %.4f,%.4f", 
                            s80.altitude, s80.speed, s80.latitude, s80.longitude);
                    
                    // Show frame 81 (second obstacle)
                    SensorData s81 = shm->frame_sensors[80];
                    attron(COLOR_PAIR(4) | A_BOLD);
                    mvprintw(16, 6, "FRAME 222 (10.1s) - OBSTACLE #2 DETECTED");
                    attroff(COLOR_PAIR(4) | A_BOLD);
                    mvprintw(17, 8, "Alt: %.0fm | Speed: %.0fkm/h | GPS: %.4f,%.4f", 
                            s81.altitude, s81.speed, s81.latitude, s81.longitude);
                    
                    // Show frame 160 (end)
                    SensorData s160 = shm->frame_sensors[159];
                    mvprintw(19, 6, "FRAME 160 (20.0s) - End");
                    mvprintw(20, 8, "Alt: %.0fm | Speed: %.0fkm/h | GPS: %.4f,%.4f", 
                            s160.altitude, s160.speed, s160.latitude, s160.longitude);
                    
                    pthread_mutex_unlock(&shm->sensor_mutex);
                    
                    mvprintw(LINES - 2, 4, "Press any key to return to menu...");
                    refresh();
                    getch();
                    
                } else if (highlight == 1) {
                    // Show detection
                    clear();
                    attron(COLOR_PAIR(4) | A_BOLD);
                    mvprintw(1, 2, "================ OBSTACLE DETECTION DETAILS ================");
                    attroff(COLOR_PAIR(4) | A_BOLD);
                    
                    pthread_mutex_lock(&shm->detection_mutex);
                    DetectionResult det = shm->latest_detection;
                    pthread_mutex_unlock(&shm->detection_mutex);
                    
                    if (det.obstacle_detected) {
                        attron(COLOR_PAIR(4) | A_BOLD);
                        mvprintw(3, 4, "*** OBSTACLE DETECTED AT FRAME %d ***", det.frame_number);
                        attroff(COLOR_PAIR(4) | A_BOLD);
                        
                        mvprintw(5, 4, "-------------------------------------------------------");
                        attron(COLOR_PAIR(3));
                        mvprintw(6, 4, "FLIGHT SENSOR DATA (transmitted via UDP):");
                        attroff(COLOR_PAIR(3));
                        mvprintw(7, 4, "-------------------------------------------------------");
                        
                        attron(COLOR_PAIR(2));
                        mvprintw(9, 6, "Altitude:   %.2f meters", det.sensor_snapshot.altitude);
                        mvprintw(10, 6, "Speed:      %.2f km/h", det.sensor_snapshot.speed);
                        mvprintw(11, 6, "GPS:        %.6fN, %.6fE",
                                det.sensor_snapshot.latitude, det.sensor_snapshot.longitude);
                        mvprintw(12, 6, "Type:       %s", det.detection_type);
                       // mvprintw(13, 6, "Confidence: %.0f%%", det.confidence * 100);
                        attroff(COLOR_PAIR(2));
                        
                        attron(COLOR_PAIR(4) | A_BOLD);
                        mvprintw(15, 6, "*** IMMEDIATE EVASIVE ACTION REQUIRED ***");
                        attroff(COLOR_PAIR(4) | A_BOLD);
                        
                        mvprintw(17, 4, "TWO OBSTACLES DETECTED:");
                        mvprintw(18, 6, "Frame %d at 10.0 seconds", OBSTACLE_FRAME_1);
                      //  system("img2txt /home/sys1/Documents/Project(1)/Project/resources/frames/frame_006.jpg");
//system("eog /home/sys1/Documents/Project(1)/Project/resources/frames/frame_001.jpg 2>/dev/null &");
///home/sys1/Documents/Project(1)/Project/resources/frames/frame_001.jpg
mvprintw(10, 5, "hhhhhhhhhhhhhhhhhhhhhhhhhhhhhh");
system("eog resources/frames/frame_001.jpg 2>/dev/null &");



                        mvprintw(19, 6, "Frame %d at 10.1 seconds", OBSTACLE_FRAME_2);
                    } else {
                        mvprintw(3, 4, "No obstacles detected yet.");
                        mvprintw(4, 4, "System monitoring frames %d and %d for obstacles...", 
                                OBSTACLE_FRAME_1, OBSTACLE_FRAME_2);
                        mvprintw(6, 4, "Current frame: %d/%d", 
                                shm->current_frame, TOTAL_FRAMES);
                    }
                    
                    mvprintw(LINES - 2, 4, "Press any key to return to menu...");
                    refresh();
                    getch();
                    
                } else if (highlight == 2) {
                    // View frames
                    endwin();
                    system("eog resources/frames/*.jpg 2>/dev/null &");
                    sleep(1);
                    initscr();
                    
                } else if (highlight == 3) {
                    // Play video
                    endwin();
                    system("vlc resources/flightfeed.mp4 2>/dev/null &");
                    sleep(1);
                    initscr();
                    
                } else if (highlight == 4) {
                    // Quit
                    running = false;
                    shm->system_active = false;
                }
                break;
                
            case 'q':
            case 'Q':
                running = false;
                shm->system_active = false;
                break;
        }
        
        usleep(50000);
    }
    
    endwin();
}

