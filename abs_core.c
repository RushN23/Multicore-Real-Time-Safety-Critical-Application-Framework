// abs_core.c
#include "shared_data.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include "shared_data.h"
// Remove local struct definitions

#define SHM_NAME "/autodata"

void msleep(int ms) {
    struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000 };
    nanosleep(&ts, NULL);
}

void gradual_halt(shared_data_t *data) {
    printf("[ABS] INITIATING GRADUAL HALT\n");
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
        printf("[ABS] Brake pressure: %.1f\n", current_brake);
        msleep(1000);
    }
    printf("[ABS] VEHICLE STOPPED\n");
    while (1) msleep(1000);
}

int main() {
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    shared_data_t *data = mmap(NULL, sizeof(shared_data_t), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

   while (1) {
    time_t hb = atomic_load(&data->monitor_heartbeat);
    if (time(NULL) - hb > 2) gradual_halt(data);

    int sensor_failed = atomic_load(&data->sensor_failure);
    if (pthread_mutex_trylock(&data->mutex) == 0) {
        double ws[4];
        for (int i = 0; i < 4; ++i)
            ws[i] = sensor_failed ? data->last_valid_speeds[i] : data->wheel_speeds[i];

        // Calculate max slip between any two wheels
        double max_slip = 0.0;
        for (int i = 0; i < 4; ++i)
            for (int j = i + 1; j < 4; ++j)
                if (fabs(ws[i] - ws[j]) > max_slip)
                    max_slip = fabs(ws[i] - ws[j]);

        if (max_slip > 2.5) {
            data->brake = fmax(0.0, data->brake - 0.1);
            data->abs_active = 1;
            printf("[ABS] Slip detected! ABS Active. Max slip: %.2f\n", max_slip);
        } else {
            // Optionally, restore brake pressure if below target
            if (data->brake < 1.0)
                data->brake = fmin(1.0, data->brake + 0.05);
            data->abs_active = 0;
            printf("[ABS] No slip. ABS Inactive. Max slip: %.2f\n", max_slip);
        }
        pthread_mutex_unlock(&data->mutex);
    }
    msleep(50);
}

}
