SWAT can trace all API calls within the system.

It currently supports Windows XP and Linux (all possible versions) on i386.

Running API tracing:

    # SWAT installation directory
    dir=/usr/local
    # Run API tracing for Windows XP
    $dir/bin/qemu-system-i386 -D api.log -plugin $dir/lib/libintrospection.so,args="WinXP" -snapshot -hda WinXP.qcow2
    # Run API tracing for Linux
    $dir/bin/qemu-system-i386 -D api.log -plugin $dir/lib/libintrospection.so,args="Linux" -snapshot -hda Linux.qcow2

Execution log is saved into the file api.log.
