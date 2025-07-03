#include "shared_data.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Create and initialize the shared memory segment
int create_and_init_shared_data() {
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return -1;
    }
    if (ftruncate(fd, sizeof(shared_data_t)) == -1) {
        perror("ftruncate");
        close(fd);
        return -1;
    }
    shared_data_t *data = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }
    memset(data, 0, sizeof(shared_data_t));

    // Initialize the mutex for interprocess use
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&data->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    // Initialize atomics
    atomic_store(&data->sensor_failure, 0);
    atomic_store(&data->monitor_heartbeat, time(NULL));

    // Optionally set initial values for other fields as needed

    munmap(data, sizeof(shared_data_t));
    close(fd);
    return 0;
}

// Map the shared memory segment for use in each process
shared_data_t* map_shared_data(int *fd) {
    *fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (*fd == -1) {
        perror("shm_open");
        return NULL;
    }
    shared_data_t *data = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(*fd);
        return NULL;
    }
    return data;
}

// Unmap and optionally delete the shared memory segment
void unmap_and_cleanup_shared_data(shared_data_t *data, int fd) {
    munmap(data, sizeof(shared_data_t));
    close(fd);
    // To delete the shared memory segment (call only when you want to remove it system-wide):
    // shm_unlink(SHM_NAME);
}

int main() {
    if (create_and_init_shared_data() == 0) {
        printf("Shared memory initialized.\n");
        return 0;
    }
    return 1;
}
