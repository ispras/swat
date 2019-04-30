SWAT can trace all API calls within the system.

It currently supports Windows XP and Linux (all possible versions) on i386.

Running API tracing:

    # SWAT installation directory
    dir=/usr/local
    # Run API tracing for Windows XP
    $dir/bin/qemu-system-i386 -D api.log -plugin $dir/lib/libintrospection.so,args="WinXP" -snapshot -hda WinXP.qcow2
    # Run API tracing for Linux
    $dir/bin/qemu-system-i386 -D api.log -plugin $dir/lib/libintrospection.so,args="Linux" -snapshot -hda Linux.qcow2

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
