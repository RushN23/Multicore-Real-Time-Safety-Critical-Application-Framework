#include <iostream>
#include <chrono>
#include <thread>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "shared_data.h"

#define RAMP_DURATION 5.0
#define SHM_NAME "/autodata"

template<typename T>
T clamp(T v, T lo, T hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

int main() {
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open failed");
        return 1;
    }

    size_t size = sizeof(shared_data_t);
    shared_data_t* data = static_cast<shared_data_t*>(
        mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (data == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return 1;
    }

    data->monitor_heartbeat.store(std::time(nullptr), std::memory_order_release);

    while (true) {
        long monitor_hb = data->monitor_heartbeat.load(std::memory_order_acquire);
        auto now = std::chrono::system_clock::now();
        time_t now_time = std::chrono::system_clock::to_time_t(now);

        if (now_time - monitor_hb > 2) {
            std::cout << "[MONITOR][PID " << getpid() << "] Initiating gradual halt..." << std::endl;
            pthread_mutex_lock(&data->mutex);
            double initial_throttle = data->throttle;
            double initial_brake = data->brake;
            pthread_mutex_unlock(&data->mutex);

            auto fail_time = std::chrono::steady_clock::now();
            while (true) {
                auto elapsed = std::chrono::steady_clock::now() - fail_time;
                double t = std::chrono::duration<double>(elapsed).count();
                double ramp_factor = clamp(t / RAMP_DURATION, 0.0, 1.0);

                pthread_mutex_lock(&data->mutex);
                data->throttle = initial_throttle * (1 - ramp_factor);
                data->brake = initial_brake + (1.0 - initial_brake) * ramp_factor;
                pthread_mutex_unlock(&data->mutex);

                std::cout << "Ramp progress: " << ramp_factor*100 << "% | "
                          << "Throttle: " << data->throttle
                          << " Brake: " << data->brake << std::endl;

                if (t >= RAMP_DURATION) {
                    std::cout << "[MONITOR] System halted" << std::endl;
                    while (true) std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        // Normal operation
        data->monitor_heartbeat.store(std::time(nullptr), std::memory_order_release);

        bool sensor_failed = data->sensor_failure.load(std::memory_order_acquire);

        pthread_mutex_lock(&data->mutex);
        double throttle = data->throttle;
        double brake = data->brake;
        double torque = 200.0 * throttle - 150.0 * brake;
        std::cout << "[MONITOR][PID " << getpid() << "] Status | "
                  << "Throttle: " << throttle
                  << " Brake: " << brake
                  << " Torque: " << torque
                  << " Sensor OK: " << std::boolalpha << !sensor_failed << std::endl;
        pthread_mutex_unlock(&data->mutex);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    munmap(data, size);
    close(fd);
    return 0;
}
