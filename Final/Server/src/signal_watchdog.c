#include "../include/aviation_system.h"

SharedMemory* g_shm_global = NULL;

void signal_handler(int signum) {
    switch(signum) {
        case SIGUSR1:
            printf("\n╔════════════════════════════════════════╗\n");
            printf("║ [SIGNAL] EMERGENCY ALERT - SIGUSR1    ║\n");
            printf("╚════════════════════════════════════════╝\n");
            if (g_shm_global) {
                pthread_mutex_lock(&g_shm_global->sensor_mutex);
                printf("[EMERGENCY] Altitude: %.2fm\n", g_shm_global->current_sensor.altitude);
                printf("[EMERGENCY] Speed: %.2fkm/h\n", g_shm_global->current_sensor.speed);
                pthread_mutex_unlock(&g_shm_global->sensor_mutex);
            }
            break;
            
        case SIGUSR2:
            printf("\n╔════════════════════════════════════════╗\n");
            printf("║ [SIGNAL] WARNING - SIGUSR2             ║\n");
            printf("╚════════════════════════════════════════╝\n");
            printf("[WARNING] System recovery initiated\n");
            break;
            
        case SIGINT:
            printf("\n[SIGNAL] Shutdown (Ctrl+C)\n");
            if (g_shm_global) {
                g_shm_global->system_active = false;
            }
            exit(0);
            break;
    }
}

void init_signal_handlers(SharedMemory* shm) {
    g_shm_global = shm;
    
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    
    printf("[SignalHandler] Initialized\n");
}

void* watchdog_thread(void* arg) {
    SharedMemory* shm = (SharedMemory*)arg;
    int last_processed = 0;
    int stall_count = 0;
    
    printf("[Watchdog] System health monitoring started\n");
    
    while (shm->system_active) {
        sleep(5);
        
        pthread_mutex_lock(&shm->frame_mutex);
        int current = shm->total_frames_processed;
        pthread_mutex_unlock(&shm->frame_mutex);
        
        if (current == last_processed) {
            stall_count++;
            
            // Don't kill system - just monitor
            if (stall_count >= 3) {
                // System has sent all frames and is now stable
                stall_count = 0;  // Reset counter
            }
        } else {
            stall_count = 0;
        }
        
        last_processed = current;
    }
    
    printf("[Watchdog] Stopped\n");
    return NULL;
}

