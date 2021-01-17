#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "livewatcher.h"

#define gettid()  (syscall(__NR_gettid))
typedef void* (*thread_entry_t)(void* args);

int test1 = 0;
int test2 = 0;
int test3 = 0;
int test4 = 0;

void* test_thread1(void* args) {
    (void)args;
    int watcher1 = alloc_watcher();
    int watcher2 = alloc_watcher();
    if (set_watcher(watcher1, &test1, sizeof(int), LW_MODE_W) != 0)
    {
        perror("set watcher1");
    }
     
    if (set_watcher(watcher2, &test2, sizeof(int), LW_MODE_W) != 0)
    {
        perror("set watcher2");
    }
    usleep(5000);
    printf("test_thread1  PID: %d TID: %lu, CPU: %d\n", getpid(), gettid(), sched_getcpu());
    usleep(5000);
  
    test3 = 99;
    usleep(400);
    test4 = 99;
    usleep(800);
    
    rst_watcher(watcher1);
    rst_watcher(watcher2);
    
    usleep(400);
    test3 = 199;
    usleep(400);
    test4 = 199;
    usleep(1000);
    
    return 0;
}
void* test_thread2(void* args) {
    (void)args;
    int watcher3 = alloc_watcher();
    int watcher4 = alloc_watcher();
    if (set_watcher(watcher3, &test3, sizeof(int), LW_MODE_W) != 0)
    {
        perror("set watcher3");
    }
    if (set_watcher(watcher4, &test4, sizeof(int), LW_MODE_W) != 0)
    {
        perror("set watcher4");
    }
    usleep(5000);
    printf("test_thread2  PID: %d TID: %lu, CPU: %d\n", getpid(), gettid(), sched_getcpu());
    usleep(5000);

    test1 = 11;
    usleep(400);
    test2 = 11;
    usleep(800);
    
    rst_watcher(watcher3);
    rst_watcher(watcher4);
    
    usleep(400);
    test1 = 111;
    usleep(400);
    test2 = 111;
    usleep(1000);

    return 0;
}
void* test_thread3(void* args) {
    (void)args;
    printf("test_thread3  PID: %d TID: %lu, CPU: %d\n", getpid(), gettid(), sched_getcpu());
    usleep(10000);
    test1 = 9999;
    test2 = 9999;
    test3 = 9999;
    test4 = 9999;
    return 0;
}

int main() {   
    int i;
    int numberOfProcessors;
    pthread_t      threads[3] = {-1,-1,-1};
    thread_entry_t entries[3] = {test_thread1, test_thread2,test_thread3};
    pthread_attr_t attr;
    cpu_set_t cpus;
    
    numberOfProcessors = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Number of processors: %d\n", numberOfProcessors);
    printf("%p %p %p %p\n", &test1, &test2, &test3, &test4);
 
    

    for (i = 0; i < 3; i++) {
       CPU_ZERO(&cpus);
       CPU_SET(i%numberOfProcessors, &cpus);
       pthread_attr_init(&attr);
       pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
       pthread_create(&threads[i], &attr, entries[i], NULL);
    }

    for (i = 0; i < 3; i++) {
        if (threads[i] != (pthread_t)-1)
            pthread_join(threads[i], NULL);
    }
    
    lw_show_info("this is information from userspace\n");
    
    free_watcher(0);
    free_watcher(1);
    free_watcher(2);
    free_watcher(3);

    return 0;
}