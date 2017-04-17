#coding:utf-8

import time
import os
import msvcrt
import _subprocess
import subprocess

def _make_inheritable(handle):
	"""Return a duplicate of handle, which is inheritable"""
	return _subprocess.DuplicateHandle(_subprocess.GetCurrentProcess(),
						handle, _subprocess.GetCurrentProcess(), 0, 1,
						_subprocess.DUPLICATE_SAME_ACCESS)

	
bufsize = 0			
#创建输入接口
pipein1,pipein1w = _subprocess.CreatePipe(None, 0)
pipein1 = _make_inheritable(pipein1)
#接管进程输入口
startupinfo32 = subprocess.STARTUPINFO()
startupinfo32.dwFlags = subprocess.STARTF_USESHOWWINDOW | subprocess.STARTF_USESTDHANDLES
startupinfo32.wShowWindow = 1
startupinfo32.hStdInput = pipein1
startupinfo32.hStdOut = _make_inheritable(_subprocess.GetStdHandle(_subprocess.STD_OUTPUT_HANDLE))
#显示窗口方式打开新进程
p1 = subprocess.Popen('webrtc-cpp-sample.exe',startupinfo = startupinfo32,creationflags=subprocess.CREATE_NEW_CONSOLE)

#关闭句柄
if p1.stdin:
	p1.stdin.close()
if p1.stdout:
	p1.stdout.close()
if p1.stderr:
	p1.stderr.close()

#关联进程stdin
pipein1w = msvcrt.open_osfhandle(pipein1w.Detach(), 0)
p1.stdin = os.fdopen(pipein1w, 'wb', bufsize)

#创建输入接口
pipein2,pipein2w = _subprocess.CreatePipe(None, 0)
pipein2 = _make_inheritable(pipein2)
#接管进程输入口
startupinfo32 = subprocess.STARTUPINFO()
startupinfo32.dwFlags = subprocess.STARTF_USESHOWWINDOW | subprocess.STARTF_USESTDHANDLES
startupinfo32.wShowWindow = 1
startupinfo32.hStdInput = pipein2
startupinfo32.hStdOut = _make_inheritable(_subprocess.GetStdHandle(_subprocess.STD_OUTPUT_HANDLE))
#显示窗口方式打开新进程
p2 = subprocess.Popen('webrtc-cpp-sample.exe',startupinfo = startupinfo32,creationflags=subprocess.CREATE_NEW_CONSOLE)
#关闭句柄
if p2.stdin:
	p2.stdin.close()
if p2.stdout:
	p2.stdout.close()
if p2.stderr:
	p2.stderr.close()

#关联进程stdin
pipein2w = msvcrt.open_osfhandle(pipein2w.Detach(), 0)
p2.stdin = os.fdopen(pipein2w, 'wb', bufsize)	

#开始执行指令
print "Command Begin"

st = 0.3
time.sleep(1)
p1.stdin.write('sdp1\n')
p1.stdin.flush()
time.sleep(st)
p1.stdin.write('sdps\n')
p1.stdin.flush()
time.sleep(st)
p1.stdin.write('./sdp1.txt\n')
p1.stdin.flush()
time.sleep(st)
p1.stdin.write(';\n')
p1.stdin.flush()
time.sleep(st)

p2.stdin.write('sdpl2\n')
time.sleep(st)
p2.stdin.write('./sdp1.txt\n')
time.sleep(st)
p2.stdin.write(';\n')
time.sleep(st)
p2.stdin.write('sdps\n')
time.sleep(st)
p2.stdin.write('./sdp2.txt\n')
time.sleep(st)
p2.stdin.write(';\n')
time.sleep(st)

p1.stdin.write('sdpl3\n')
time.sleep(st)
p1.stdin.write('./sdp2.txt\n')
time.sleep(st)
p1.stdin.write(';\n')
time.sleep(st)
p1.stdin.write('ice1\n')
time.sleep(st)
p1.stdin.write('ices\n')
time.sleep(st)
p1.stdin.write('./ice1.txt\n')
time.sleep(st)
p1.stdin.write(';\n')
time.sleep(st)

p2.stdin.write('icel2\n')
time.sleep(st)
p2.stdin.write('./ice1.txt\n')
time.sleep(st)
p2.stdin.write(';\n')
time.sleep(st)
p2.stdin.write('ices\n')
time.sleep(st)
p2.stdin.write('./ice2.txt\n')
time.sleep(st)
p2.stdin.write(';\n')
time.sleep(st)

p1.stdin.write('icel2\n')
time.sleep(st)
p1.stdin.write('./ice2.txt\n')
time.sleep(st)
p1.stdin.write(';\n')
time.sleep(st)

#移除临时文件
os.remove('./sdp1.txt');
os.remove('./sdp2.txt');
os.remove('./ice1.txt');
os.remove('./ice2.txt');

print "Command Over"

#取消锁定进程输入口
p1.stdin.write('default\n');
p2.stdin.write('default\n');

