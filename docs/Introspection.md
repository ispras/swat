## Using introspection QEMU plugin

SWAT can trace all API calls within the system.

It currently supports the following platforms:
 * i386
   * Windows XP
   * Linux (all versions)
 * ARM
   * Linux (all versions)
 * x86_64
   * Windows 10
   * Linux (all versions)
 * AArch64
   * Linux (all versions)
 
Running API tracing:

    # SWAT installation directory
    dir=/usr/local
    # Run API tracing for Windows XP
    $dir/bin/qemu-system-i386 -D api.log -plugin file=$dir/lib/libintrospection.so,args="WinXP" -snapshot -hda WinXP.qcow2
    # Run API tracing for Windows 10
    $dir/bin/qemu-system-x86_64 -D api.log -plugin file=$dir/lib/libintrospection.so,args="Win10x64" -snapshot -hda WinXP.qcow2
    # Run API tracing for Linux
    $dir/bin/qemu-system-i386 -D api.log -plugin file=$dir/lib/libintrospection.so,args="Linux" -snapshot -hda Linux.qcow2

Execution log is saved into the file api.log. It includes list of the executed system call id's and the list of the executed named functions.

Linux log example:
```
Function b75cee10:__getpid
Function b758ecf0:calloc
Function b75cee10:__getpid
Function b75cee10:__getpid
Function b7592840:strdup
Function b758e300:malloc
Function b758e950:free
Function b76fcc30:__udivdi3
Function b76fcd60:__umoddi3
Function b76138d0:__snprintf_chk
Function b7613900:__vsnprintf_chk
Function b7588710:_IO_setb
Function b755cd30:vfprintf
Function b7596960:strchrnul
Function b7588840:_IO_default_xsputn
Function b7588840:_IO_default_xsputn
```

Windows log example:
```
5633000: sysenter c8
Function 75b44421:CsrValidateMessageBuffer
Function 75b43fc3:CsrQueryApiPort
54ae000: sysenter 1208
54ae000: sysenter 11de
Function 75b45520:CsrDereferenceThread
```

## Introspection plugin internals

Introspection plugin monitors the following instructions:
* int 80h
* iret
* sysenter
* sysexit
* syscall
* sysret

These instructions are used for system call operations on i386.

Plugin intercepts file open/close and file mapping to detect executable module loading in the guest OS.

When CPU starts executing the loaded module, the plugin parses its header and sets tracepoints on every exported function. These tracepoints generate log messages that correspond to the function entries.

## Firmware introspection

We tested SWAT on several Linux-based OS for the routers.
There is the list of the tested firmwares, where we've got a log of the system calls:

| Firmware | Kernel version |
|----------|----------------|
| Alpine Linux 3.7.0 | 4.9.65-1 |
| DD-WRT v24 | 2.6.23.17 |
| Endian Firewall 2.1.2 | 2.6.9-55 |
| floppyfw 3.0.14 | 2.4.37.10 |
| IPCop 2.1.8 | 3.4-3 |
| IPFire 2.19 | 3.14.79 |
| LEAF 3.1 | 2.4.34 |
| LEAF 6.1.1 | 4.9.68 |
| LEDE 17.01.4 | 4.4.92 |
| MikroTIK 6.41 | |
| Openwall 3.1 | 2.6.18-408 |
| OpenWRT 15.05 | 3.18.23 |
| Untangle | 3.16.0.4 |
| ZeroShell 2.0 | 3.4.6 |

