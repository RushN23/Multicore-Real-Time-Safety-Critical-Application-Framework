// ecu_core.c

#include "shared_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#define SHM_NAME "/autodata"

void msleep(int ms) {
    struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000 };
    nanosleep(&ts, NULL);
}

void gradual_halt(shared_data_t *data) {
    printf("[ECU] INITIATING GRADUAL HALT\n");
    double current_brake;
    
    pthread_mutex_lock(&data->mutex);
    current_brake = data->brake;
    pthread_mutex_unlock(&data->mutex);

    while (current_brake < 1.0) {
        pthread_mutex_lock(&data->mutex);
        current_brake = fmin(current_brake + 0.1, 1.0);
        data->brake = current_brake;
        data->throttle = 0.0;
        pthread_mutex_unlock(&data->mutex);
        
        printf("[ECU] Brake pressure: %.1f\n", current_brake);
        msleep(1000);
    }

    printf("[ECU] VEHICLE STOPPED\n");
    while (1) msleep(1000);
}

int main() {
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) { 
        perror("shm_open"); 
        exit(EXIT_FAILURE); 
    }

    shared_data_t *data = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) { 
        perror("mmap"); 
        close(fd); 
        exit(EXIT_FAILURE); 
    }

    // Simulation variables
    int throttle_direction = 1;  // 1 = increasing, -1 = decreasing
    float brake_target = 0.0;

    while (1) {
        time_t hb = atomic_load(&data->monitor_heartbeat);
        if (time(NULL) - hb > 2) gradual_halt(data);

        if (pthread_mutex_lock(&data->mutex) == 0) {
            // Simulate throttle changes when ABS is inactive
            if (!data->abs_active) {
                if (throttle_direction == 1) {
                    data->throttle = fmin(1.0, data->throttle + 0.02);
                    if (data->throttle >= 1.0) throttle_direction = -1;
                } else {
                    data->throttle = fmax(0.0, data->throttle - 0.02);
                    if (data->throttle <= 0.0) throttle_direction = 1;
                }

                // Simulate brake pedal interaction
                data->brake = fmax(0.0, fmin(1.0, data->brake + (brake_target - data->brake) * 0.1));
                if (data->brake < 0.01) brake_target = 0.5;
                else if (data->brake > 0.49) brake_target = 0.0;
            }
           printf("[ECU][PID %d] Updated throttle: %.2f, brake: %.2f\n", getpid(), data->throttle, data->brake);
            // Existing ABS reaction logic
            if (data->abs_active) {
                data->throttle = fmax(0.0, data->throttle - 0.05);
                printf("[ECU] ABS active! Throttle reduced.\n");
            }

            double throttle = data->throttle;
            double brake = data->brake;
            double torque = 200 * throttle - 150 * brake;
            printf("[ECU] Throttle: %.2f, Brake: %.2f, Torque: %.2f, ABS: %d\n", 
                   throttle, brake, torque, data->abs_active);
            
            pthread_mutex_unlock(&data->mutex);
        } else {
            printf("[ECU] Could not acquire lock, retrying...\n");
        }

        msleep(100);
    }

    munmap(data, sizeof(shared_data_t));
    close(fd);
    return 0;
}
