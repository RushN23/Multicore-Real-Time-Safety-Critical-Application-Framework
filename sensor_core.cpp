#include "shared_data.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define SHM_NAME "/autodata"

int main() {
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open failed");
        return 1;
    }

    shared_data_t* data = static_cast<shared_data_t*>(
        mmap(nullptr, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    
    if (data == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return 1;
    }

    // Simulation variables
    double base_speed = 50.0;
    double t = 0.0;

    while (true) {
        pthread_mutex_lock(&data->mutex);
        
        // Simulate wheel speeds with sinusoidal variations
        t += 0.1;
        for (int i = 0; i < 4; ++i) {
            data->wheel_speeds[i] = base_speed + 5.0 * sin(t + i * 1.5) + (rand() % 100) * 0.01;
        }

        bool sensor_failed = data->sensor_failure.load(std::memory_order_acquire);
        double speeds[4];

        if (sensor_failed) {
            std::copy(std::begin(data->last_valid_speeds), std::end(data->last_valid_speeds), speeds);
            std::cout << "[SENSOR] FAILURE - Using last known good values: ";
        } else {
            std::copy(std::begin(data->wheel_speeds), std::end(data->wheel_speeds), speeds);
            std::copy(std::begin(data->wheel_speeds), std::end(data->wheel_speeds), data->last_valid_speeds);
            std::cout << "[SENSOR] OK - Using current sensor values: ";
        }

        for (int i = 0; i < 4; ++i)
            std::cout << speeds[i] << (i < 3 ? ", " : "");
        std::cout << std::endl;

        pthread_mutex_unlock(&data->mutex);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    munmap(data, sizeof(shared_data_t));
    close(fd);
    return 0;
}
