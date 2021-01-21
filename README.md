# livewatcher

## background

In c/c++ programs, memory corruption is a common kind of problem. In general, the difficulty lies in finding when and where the memory is corrupted. the `watch` command of `GDB` is useful when the problem is easy to reproduce. Otherwise we need to find other solutions. `mprotect` is a solution and here is another one.

## implemention

Some CPUs provide special registers for breakpoints. The x86 architecture provides debug registers. we could ues them to set breakpoints for some memory  addresses and exceptions will be triggered when they are accessed.

Livewatcher consists of three parts:

- `livewatcher.stp`    `stap` compiles it to a kernel module and it runs in kernel space. it hooks several functions to set breakpoints and handle exceptions.
- `liblivewatcher`   it provides userspace API to set breakpoints.
- `watch.py`      it is an optional tool to use livewatcher.

## usage

```
WARNING: The project is under development. One known problem is that endless loop may occur in kernel. Try it in test environment.
```

1. Install `systemtap` and `debuginfo` referring to [this wiki](https://sourceware.org/systemtap/wiki). Here is my system information.

   ```
   //VirtualBox 6.1.16
   [root@mao /]# cat /proc/cpuinfo | grep "model name" | uniq
   model name	: Intel(R) Core(TM) i5-2450M CPU @ 2.50GHz
   [root@mao /]# cat /etc/redhat-release
   CentOS release 6.6 (Final)
   [root@mao /]# uname -a
   Linux mao 2.6.32-504.el6.i686 #1 SMP Wed Oct 15 03:02:07 UTC 2014 i686 i686 i386 GNU/Linux
   [root@mao /]# stap --version
   Systemtap translator/driver (version 2.5/0.164, rpm 2.5-5.el6)
   Copyright (C) 2005-2014 Red Hat, Inc. and others
   This is free software; see the source for copying conditions.
   enabled features: AVAHI LIBRPM LIBSQLITE3 NSS TR1_UNORDERED_MAP NLS
   ```

2. Build and install `liblivewatcher` 

   ```
   [root@mao livewatcher]# make && make install
   ```

3. Refer to the comments in`livewatcher.h`  and the sample code，set break points on memory  addresses which may be corrupted

4. Compile source code with `-g` 

   ```
   [mike@mao livewatcher]$ make sample
   ```

5. Use `watch.py` to start `livewatcher.stp`  

   ```
   [root@mao livewatcher]# ./watch.py -p ./sample/sample 
   stap -g -d /samba/livewatcher/sample/sample --ldd /samba/livewatcher/livewatcher.stp
   loading...
   livewatcher start...
   >>
   ```
   
6. Run the program and  `livewatcher.stp` prints backtraces when breakpoints are accessed.

```
[mike@mao livewatcher]$ ./sample/sample
```

## options

### watch.py

`addr2line`     If `livewatcher.stp` is started by `  watch.py` , `  watch.py` deals with the output of `livewatcher.stp`.  This option makes `watch.py` use `addr2line` to get detailed information about backtraces.

example:

```
[root@mao livewatcher]# ./watch.py -p ./sample/sample --addr2line
```

### livewatcher.stp

At the beginning of the code,  there are two macro definitions:

`STAP_USE_PTRACE_PEEKUSR`    It could be 0 or 1.  When it is 1,  `livewatcher.stp` uses  `stap_arch_ptrace` and `get_user` to read the value of debug registers. Otherwise,  `livewatcher.stp` uses different code for diffrent kernel version to do this. This may cause compatibility issues. It is recommended to set it to 1.

 `STAP_USE_DO_TKILL`   It could be 0 、1 or 2.  After setting a breakpoint,  `livewatcher.stp`  needs to synchronize it to all other threads. When it is 0,  `livewatcher.stp ` sets the debug register directly.  When it is 1, `livewatcher.stp `  uses `stap_do_tkill` to send `SIGUSR1` to threads and the signal handle synchronizes  the breakpoint.  When it is 2,  `livewatcher.stp`  sends `SIGUSR1` if the thread is running.

`livewatcher.stp`  creates several  files in procfs after it starts.

```
//the path name is random
/proc/systemtap/stap_566c2a9eec2e0339f15dd6a2dd39a559__3314
[mike@mao stap_566c2a9eec2e0339f15dd6a2dd39a559__3314]$ ls
lw_ignore_hwbp_sigtrap  lw_record_watcher  lw_runtime  lw_ubacktrace_detail
[mike@mao stap_566c2a9eec2e0339f15dd6a2dd39a559__3314]$ 
```

`lw_ignore_hwbp_sigtrap`     whether to ignore the SIGTRAP signal which caused by debug registers. 

`lw_record_watcher`    whether to record backtraces where the breakpoints are set. 0 means disable. A positive number means the limitation of the callstack level.  A negative number means that there is no limit.

`lw_runtime`    show some runtime information about  `livewatcher.stp`.

`lw_ubacktrace_detail`    whether to show  detailed information about backtraces.