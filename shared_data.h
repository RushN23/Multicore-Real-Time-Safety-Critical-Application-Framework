#ifndef SHARED_DATA_H
#define SHARED_DATA_H
#define SHM_NAME "/autodata"
#include <pthread.h>

#ifdef __cplusplus
#include <atomic>
#define ATOMIC_INT std::atomic<int>
#define ATOMIC_LONG std::atomic<long>
#else
#include <stdatomic.h>
#define ATOMIC_INT _Atomic int
#define ATOMIC_LONG _Atomic long
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double wheel_speeds[4];
    double last_valid_speeds[4];
    double throttle;
    double brake;
    int abs_active;
    ATOMIC_INT sensor_failure;
    ATOMIC_LONG monitor_heartbeat;
    pthread_mutex_t mutex;
} shared_data_t;

int create_and_init_shared_data();
shared_data_t* map_shared_data(int *fd);
void unmap_and_cleanup_shared_data(shared_data_t *data, int fd);

#ifdef __cplusplus
}
#endif

#endif // SHARED_DATA_H


