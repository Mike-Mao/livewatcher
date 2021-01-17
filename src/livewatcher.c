#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <execinfo.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include "livewatcher.h"

#define LW_MAX_BACKTRACE      32
#define LW_LOCKF              "/tmp/livewatcher.lck"
#define LW_RESET_HWBP         0
#define DR_HWBP_NR            4
#define ENCODE_FD(dr,len,rw)  (int)((0x7ffff000) | (((dr)&0xf)<<8) | \
                              (((len)&0xf)<<4) | ((rw)&0xf))
#define DUMMY_FD              ENCODE_FD(0,1,1)

enum LW_IOCTL_OPS {
    LW_IOCTL_MIN = 0x7fffff00,  //MUST be the first one
    LW_IOCTL_SET_HWBP,
    LW_IOCTL_SYNC_HWBP,
    LW_IOCTL_ATINIT,
    LW_IOCTL_ATEXIT,
    LW_IOCTL_DUMP_DR,
    LW_IOCTL_SHOW_INFO,
    LW_IOCTL_IGNORE_HWBP_SIGTRAP,
    LW_IOCTL_RECORD_WATCHER,
    LW_IOCTL_UBACKTRACE_DETAIL,
    LW_IOCTL_TKILL_WEIGHT,
    LW_IOCTL_MAX                //MUST be the last one
};

static pthread_mutex_t lw_mutex = PTHREAD_MUTEX_INITIALIZER;
static int lw_status[DR_HWBP_NR] = {0};

static void sigusr1_handler(int signum) {
    (void)signum;
    ioctl(DUMMY_FD, LW_IOCTL_SYNC_HWBP, 0);
}
static void sigtrap_handler(int signum) {
    (void)signum;
    lw_show_info("-----unexpected SIGTRAP-----\n");
    ioctl(DUMMY_FD, LW_IOCTL_DUMP_DR, 0);
    lw_backtrace("unexpected SIGTRAP %d %ld\n", 
                 getpid(), syscall(__NR_gettid));
}
static void lw_exit() {
    ioctl(DUMMY_FD, LW_IOCTL_ATEXIT, 0);
}
/* set user space memory for systemtap module to read debug register.
 * memory and lock will be released by OS when the process exits.
 */
static void lw_init() {
#define CHECK(a) if (!(a)) {errline=__LINE__; goto fail;}
    static int inited = 0;
    int fd, errline, page;
    size_t size;
    void *umem;
    
    
    if (inited) return;
    fd = open(LW_LOCKF, O_RDWR|O_CREAT|O_CLOEXEC, 0666);
    CHECK(fd >= 0);
    CHECK(lockf(fd, F_TLOCK, 0) == 0);
    size = sysconf(_SC_NPROCESSORS_CONF) * sizeof(long);
    page = sysconf(_SC_PAGESIZE);
    size = (size + page - 1) & (~(page - 1));
    umem = mmap(0, size, PROT_READ|PROT_WRITE, 
                MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    CHECK(umem != NULL);
#if 0
    {
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "cat /proc/%d/maps", getpid());
        /*system should be called before mlock, or put_user will fail*/
        system(cmd);
    }
#endif
    CHECK(mlock(umem, size) == 0);
    CHECK(ioctl(DUMMY_FD, LW_IOCTL_ATINIT, umem) == 0);
    CHECK(signal(SIGUSR1, sigusr1_handler) != SIG_ERR);
    CHECK(signal(SIGTRAP, sigtrap_handler) != SIG_ERR);
    /* functions registered using atexit() are not called if a 
     * process terminates abnormally because of the delivery of
     * a signal.it does no matter here.
     */
    atexit(lw_exit);
    inited = 1;
    return;
#undef CHECK
fail:
    fprintf(stderr,"livewatcher init fail:errline=%d  %s\n",
            errline, strerror(errno));
    exit(-1);
}
int lw_dump_dr() {
    return ioctl(DUMMY_FD, LW_IOCTL_DUMP_DR, 0);
}
int lw_ignore_hwbp_sigtrap(int ignore) {
    return ioctl(DUMMY_FD, LW_IOCTL_IGNORE_HWBP_SIGTRAP, ignore);
}
int lw_record_watcher(int stack_level) {
    return ioctl(DUMMY_FD, LW_IOCTL_RECORD_WATCHER, stack_level);
}
int lw_ubacktrace_detail(int detail) {
    return ioctl(DUMMY_FD, LW_IOCTL_UBACKTRACE_DETAIL, detail);
}
int lw_tkill_weight(int weight) {
    return ioctl(DUMMY_FD, LW_IOCTL_TKILL_WEIGHT, weight);
}
int lw_show_info(const char *fmt, ...) {
    int  len, ret = -1;
    char info[LW_MAX_INFO];
    
    va_list args;
	va_start(args, fmt);
	len = vsnprintf(info, LW_MAX_INFO, fmt, args);
	va_end(args);
    if (len > 0 && len < LW_MAX_INFO) {
        ret = ioctl(DUMMY_FD, LW_IOCTL_SHOW_INFO, (unsigned long)info);
    }
    return ret;
}
int lw_backtrace(const char *fmt, ...) {
    int count;
    void *stack[LW_MAX_BACKTRACE];
    
    if (fmt) {
        char msg[LW_MAX_INFO];
        va_list args;
        va_start(args, fmt);
        vsnprintf(msg, LW_MAX_INFO, fmt, args);
        va_end(args);
        puts(msg);
    }
    memset(stack, 0, sizeof(stack));
    count = backtrace(stack, LW_MAX_BACKTRACE);
    if (count > 0) 
        backtrace_symbols_fd(stack, count, STDOUT_FILENO);
    return count;
}
int alloc_watcher()
{   
    int ret = -1;
    int index;
    if (pthread_mutex_lock(&lw_mutex) != 0) {
        perror("fail to lock lw_mutex");
        return -1;
    }
    lw_init();
    for (index = 0; index < DR_HWBP_NR; ++index) {
        if (lw_status[index] == 0) {
            lw_status[index] = 1;
            ret = index;
            break;
        }
    }
    if (pthread_mutex_unlock(&lw_mutex) != 0) {      
        if (ret != -1) lw_status[ret] = 0;
        ret = -1;
        perror("fail to unlock lw_mutex");
    }
    return ret;
}
int set_watcher(int watcher, void* addr, size_t len, int rw) {   
    static const int len2regflag[] = {0, 0, 1, -1, 3, -1, -1, -1, 2};
    unsigned long tmpaddr = (unsigned long)addr;
    int tmp_fd;

    assert(watcher >= 0 && watcher < DR_HWBP_NR);
    assert(len == 1 || len == 2 || len == 4 || len == 8);
    assert(rw == 0 || rw  == 1 || rw == 3);  // 0--exe 1--write 3--write or read
    assert(lw_status[watcher] == 1);
    
    if (rw == 0) len = 0;
    tmp_fd = ENCODE_FD(watcher, len2regflag[len], rw);
    return ioctl(tmp_fd, LW_IOCTL_SET_HWBP, tmpaddr);
}
int rst_watcher(int watcher) {
    assert(watcher >= 0 && watcher < DR_HWBP_NR);
    assert(lw_status[watcher] == 1);
    return set_watcher(watcher, (void*)LW_RESET_HWBP, 1, 1);
}
int free_watcher(int watcher) {
    int ret = 0;
    assert(watcher >= 0 && watcher < DR_HWBP_NR);
    if (pthread_mutex_lock(&lw_mutex) != 0) {
        perror("fail to lock lw_mutex");
        return -1;
    }
    if (set_watcher(watcher, (void*)LW_RESET_HWBP, 1, 1) != 0) {   
        ret = -1;
        perror("free_watcher: fail to reset watcher");
    }
    lw_status[watcher] = 0;
    if (pthread_mutex_unlock(&lw_mutex) != 0) {
        ret = -1;
        perror("fail to unlock lw_mutex");
    }
    return ret;
}