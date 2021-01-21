#!/usr/bin/python
#coding=utf-8
from __future__ import print_function
import os,re,sys,time,errno
import fcntl,select,argparse
from subprocess import Popen, PIPE
from abc import ABCMeta, abstractmethod

def execute(cmd):
    process = Popen(cmd, shell=True, stdout=PIPE, close_fds=True)
    output = process.communicate()[0]
    process.stdout.close()
    if process.poll() != 0:
        return ''
    return output.strip()

class logger():
    logfile = None
    @staticmethod
    def init():
        logpath = os.path.join(os.getcwd(), 'log/')
        if not os.path.exists(logpath):
            os.mkdir(logpath)
        logfile = 'livewatcher_%s.log'%time.strftime("%Y_%m_%d_%H_%M_%S")
        logfile = os.path.join(os.getcwd(), 'log', logfile)
        logger.logfile = open(logfile, 'a')
    @staticmethod
    def log(*args):
        if logger.logfile == None:
            logger.init()
        msg = ''.join(map(str, args))
        if not msg.endswith('\n'):
            msg += '\n'
        print('\r%s'%msg, end = '')
        msg = '%s  %s'%(time.strftime("%Y.%m.%d %H:%M:%S"), msg)
        logger.logfile.write(msg)
    @staticmethod
    def prompt():
        logger.logfile.write('>>\n')
        logger.logfile.flush()
        print('\r>>', end = '')
        sys.stdout.flush()
    @staticmethod
    def error(*args):
        msg = 'Error:' + ''.join(map(str, args))
        logger.log(msg)
        logger.logfile.close()
        exit(-1)

class cmdargs():
    def __init__(self, arg):
        if not arg.p:
            print('example: %s -p ./a.out'%sys.argv[0])
            print('run %s -h for more information'%sys.argv[0])
            exit(-1)
        self.exe = map(self.getpath, arg.p)
        self.stp = self.getpath('livewatcher.stp')
        self.addr2line = arg.addr2line
    def which(self, process):
        if process == '':
            return ''
        if os.getcwd() not in os.environ['PATH']:
            os.putenv('PATH','%s:%s'%(os.environ['PATH'], os.getcwd()))
        return execute('which %s 2>/dev/null'%process)
    def cmdline(self, pid):
        path = execute('strings /proc/%s/cmdline | head -1'%pid)
        if '/' not in path:
            path = self.which(path)
        return path.strip()
    def getpath(self, exe):
        if exe.isdigit():
            path = self.cmdline(exe)
        elif exe[0] in '/.':
            path = os.path.abspath(exe)
        else:
            info = execute('pidof %s'%exe)
            if len(info) > 0:
                exepaths = set(map(self.cmdline, info.split()))
                if len(exepaths) > 1:
                    logger.error('multiple executable file path for %s'%exe)
                path = os.path.abspath(exepaths.pop())  
            else:
                path = self.which(exe)
        if len(path) == 0:
            logger.error('cannot find the path for %s'%(exe))
        return path
class iostream():
    __metaclass__=ABCMeta
    def __init__(self, file):
        self.file = file
        self.setnonblock()
    def setnonblock(self):
        fd = self.file.fileno()
        oldflags = fcntl.fcntl(fd, fcntl.F_GETFL)
        fcntl.fcntl(fd, fcntl.F_SETFL, oldflags | os.O_NONBLOCK)
    @abstractmethod    
    def procline(self, line):
        return line
    def process(self):
        while 1:
            try:
                line = self.procline(self.file.readline())
                if line != '':
                    logger.log(line)
            except IOError as e:
                if e.errno == errno.EAGAIN:
                    break
                else:
                    logger.error(e)
class std_istream(iostream):
    def procline(self, msg):
        tmp = msg.strip()
        if tmp in ('exit','quit'):
            exit(0)
        return tmp
class stap_ostream(iostream):
    def __init__(self, arg):
        exe = '-d ' + ' -d '.join(arg.exe)
        self.addr2line = arg.addr2line
        self.cmd  = 'stap -g %s --ldd %s'%(exe, arg.stp)
        self.vcmd = 'stap -v -g %s --ldd %s'%(exe, arg.stp)
        self.proc = Popen(self.cmd, shell=True, stdout=PIPE)
        super(stap_ostream, self).__init__(self.proc.stdout)
        logger.log('%s\nloading...'%(self.cmd))
    def __del__(self):
        self.proc.stdout.close()
    @property
    def stdout(self):
        return stap.proc.stdout
    def check(self):
        if self.proc.poll() != None:
            logger.error('stap has terminated due to error.'
                         'run the following command for details:\n',self.vcmd)
    def procline(self, msg):
        if msg == '':
            self.check()
            return ''
        if self.addr2line and msg.startswith(' 0x'):
            msg = msg.split()
            mod = msg[-1]
            mod = mod[1:-1] if len(mod) >= 2 else ''
            if os.access(mod, os.X_OK):
                cmd = 'addr2line -Cf -e %s %s'%(mod, msg[0])
                addr2line = execute(cmd).split('\n')[1]
                if addr2line != '??:0':
                    msg[-1] = addr2line
            return ' '.join(msg)
        return msg
        
if __name__=="__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', nargs='+', type=str, help = 'PID or file name or path')
    parser.add_argument('--addr2line', action='store_true', default=False, help='use addr2line')
    args = cmdargs(parser.parse_args())
    stap = stap_ostream(args)
    stdin= std_istream(sys.stdin)
    ios = {sys.stdin  : stdin, 
           stap.stdout: stap}

    while 1:
        reads = select.select(ios.keys(),[],[])[0]
        for read in reads:
            ios[read].process()
        logger.prompt()
