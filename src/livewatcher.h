#ifndef __LIVE_WATCHER_H__
#define __LIVE_WATCHER_H__

#define LW_MODE_EXE  0
#define LW_MODE_W    1
#define LW_MODE_WR   3
#define LW_MAX_INFO  512

#ifdef __cplusplus
extern "C"
{
#endif

/* 
 * alloc a watcher
 *
 * in
 * out [int]
 *    success:  0/1/2/3
 *    fail:    -1
 */
 int alloc_watcher();
/* 
 * set a breakpoint
 *
 * in  [int]    watcher:  a valid watcher
 *     [void*]  addr:     the start of memory address
 *     [size_t] len:      the length of the memory which could be 1/2/4/8
 *     [int]    mode:     the breakpoint condition for the corresponding breakpoint
 *                        read/read and write/excutable
 * out [int]
 *     success:  0
 *     fail:    -1
 */
int set_watcher(int watcher, void *addr, size_t len, int mode);
/*
 * unset a breakpoint
 *
 * in  [int] watcher:  a valid watcher
 * out [int]
 *     success:  0
 *     fail:    -1
 */
int rst_watcher(int watcher);
/* 
 * free a watcher
 *
 * in  [int] watcher:  a valid watcher
 * out [int]
 *     success:  0
 *     fail:    -1
 */
int free_watcher(int watcher);


/*
 * show debug register
 */
int lw_dump_dr();
/*
 * show information in the kernel module
 */
int lw_show_info(const char *fmt, ...);
/* 
 * show backtrace
 */
int lw_backtrace(const char *fmt, ...);
/*
 * set ignore_hwbp_sigtrap
 */
int lw_ignore_hwbp_sigtrap(int ignore);
/*
 * set record_watcher
 */
int lw_record_watcher(int stack_level);

#ifdef __cplusplus
}
#endif

#endif