# Reverse debugging

Reverse debugging allows "executing" the program in reverse direction.
GDB remote protocol supports "reverse step", "reverse continue" and "reverse-roll"
commands. The first one steps single instruction backwards in time,
and the second one finds the last breakpoint in the past. Reverse-roll command allows stepping several instructions backwards in time.

Recorded executions may be used to enable reverse debugging. QEMU can't
execute the code in backwards direction, but can load a snapshot and
replay forward to find the desired position or breakpoint. The reverse-roll
command allows you to roll back the number of instructions specified in the
parameter.

The following GDB commands are supported:
 - reverse-stepi (or rsi) - step one instruction backwards
 - reverse-continue (or rc) - find last breakpoint in the past
 - reverse-roll <N_steps> - step specified in the parameter number of instruction
backwards

Reverse step loads the nearest snapshot and replays the execution until
the required instruction is met. In some cases it is necessary to step a
few (more then one) instructions backward. For this goal can be used the
reverse-stepi command with the parameter - the number of instructions. However,
the protocol for executing this command will simply repeat it as many times as
needed. This leads to unnecessary overhead for executing the command, because
every time execution step one instruction backward, algorithm need to look for the
closest snapshot and go forward to the desired instruction. In such cases, the
reverse-roll command should be used.

Reverse roll command takes parameter (number of instructions), loads the nearest
snapshot and replays the execution until the required instruction is met without
splitting the command into sequence. Reverse roll command without parameter equal the reverse step command

Reverse continue may include several passes of examining the execution
between the snapshots. Each of the passes include the following steps:
 1. loading the snapshot
 2. replaying to examine the breakpoints
 3. if breakpoint or watchpoint was met
    - loading the snaphot again
    - replaying to the required breakpoint
 4. else
    - proceeding to the p.1 with the earlier snapshot

Therefore usage of the reverse debugging requires at least one snapshot
created in advance. See the "Snapshotting" section to learn about running
record/replay and creating the snapshot in these modes.


## Examples:
### Case #1: Use recorded executions for save and replay behavior of the operating system.
    - Guest operating system: Debian x86_64
    - Path to guest operating system image: ~/imgs/debian_x86_64.qcow2
    - Path to qemu binary file: ~/qemu/x86_64-softmmu/qemu-system-x86_64
    - Memory size for guest operating system: 2G
    - Journal filename: replay.bin
    - Start snapshot name: init_snapshot

Record command:  
`~/qemu/x86_64-softmmu/qemu-system-x86_64   
-drive file=~/imgs/debian_x86_64.qcow,if=none,id=drv   
-drive driver=blkreplay,if=none,image=drv,id=drv_replay   
-device ide-hd,drive=drv_replay   
-m 2G    
-icount shift=5,rr=record,rrfile=replay.bin,rrsnapshot=init_snapshot`

Replay command:  
`~/qemu/x86_64-softmmu/qemu-system-x86_64   
-drive file=~/imgs/debian_x86_64.qcow,if=none,id=drv   
-drive driver=blkreplay,if=none,image=drv,id=drv_replay   
-device ide-hd,drive=drv_replay   
-m 2G    
-icount shift=5,rr=replay,rrfile=replay.bin,rrsnapshot=init_snapshot`

### Case #2: Use recorded executions and reverse debugging for undesired behavior investigation.

Start simulator and gdb server for interaction with GDB:  
`~/qemu/x86_64-softmmu/qemu-system-x86_64   
-drive file=~/imgs/debian_x86_64.qcow,if=none,id=drv   
-drive driver=blkreplay,if=none,image=drv,id=drv_replay   
-device ide-hd,drive=drv_replay   
-m 2G    
-S
-s
-icount shift=5,rr=replay,rrfile=replay.bin,rrsnapshot=init_snapshot`

After executing the command, the simulator will be launched in the mode, waiting for the debugger connection.

![Qemu into standby mode](./imgs/replay.png)


After that, the debugger should be running and connect to the simulator:   
`gdb -ex 'target remote :1234'`   

![Remote connection of debugger](./imgs/debugger_remote_connection.png)

Breakpoint to address or to func name (if you have symbolic information) can be set now. For example the address in which the undesirable behavior appears equal 0x000000007fe61fe9. Set breakpoint and go forward:   
`b *(void*)0x7fe61fe9`  
`c`   
Simulator will pass to the breakpoint and stop. Now if you need to move to one step backward to examine the state of the system (x /20i $pc OR disas $pc, $pc+20), you can use the reverse-stepi command inside GNU debugger console:   
`reverse-stepi`   

If you want need to move to several steps backward (for example 10 steps), the reverse-roll command should be used:   
`reverse-roll 10`   

In most cases you don't know exactly count of commands should be stepped in backward direction. In this situatioun the reverse-continue command should be used. But first of all you must set a watch point to intrested data. After that run reverse-continue into gdb console:   
`reverse-continue`   
After that the simulator will stop its execution at the point of the last data change.

### Case #3: Debugging USB kernel driver

In this section we will examine buggy kernel driver
using reverse debugging.

To check whether reverse debugging is capable to help examining bugs in drivers,
we created Linux driver for USB stick.
Driver code contains a bug - when request from user is processed 
the kernel panics and user has to reboot the machine.

```long usbInfoIoctl(struct file *f, 
    unsigned int cmd, unsigned long arg)
{
  int i;
  struct urb *urb;
  char *buf, result[24];
  struct usb_device *device;
  ...
  /* Receive data from USB */
  transmit_bulk_package(&urb, device, &buf, 36,
      usb_rcvbulkpipe(device, 0x82), 0x00000201);
  /* Some result processing */
  memcpy(result, buf, rand());
  for (i = 0; i < 24; i++)
    printk(KERN_INFO "result[%i] = %c\n", i, result[i]);
  ...
  return 0;
}
```

This function is meant to receive the user's requests
through a memory-mapped file and process them.

Received data is copied to the buffer ```buf``` and processed - we copy it to another
buffer and output to the log using ```printk``` function.

When we load the module and send a request, it triggers kernel panic.
Guest system shows the call trace, which
reports that stack was smashed.
Other details about origin of the failure are not available.

```user@debian:~/kernelModule$ sudo ./test
[938.289683] Kernel panic - not syncing; stack-protector:
Kernel stack is corrupted in: c89e93d0 
[938.289741]
[938.293354] Pid: 2768, comm: test Tainted: G 0 3.2.0-4-686-pae
#1 Debian 3.2.65-l+deb7ul 
[938.293853] Call Trace:
[938.296274]  [<c12c0c2a>] ? panic+0x4d/0xl41
[938.298932]  [<c1038576>] ? __stack_chk_fail+Oxd/Oxd
[938.301428]  [<c89e93d0>] ? usbInfoIoctl+0x217/0x21d [usb_info]
[938.301782]  [<c89e93d0>] ? usbInfoIoctl+0x217/0x21d [usb_info]
[938.302003]  [<c10291ff>] ? kmap_atomic_prot+0x2f/OxeO
[938.302583]  [<c10d9857>] ? do_vfs.ioctl+0x459/0x48f
[938.302784]  [<c12c85a7>] ? do_page_fault+0x342/0x35e
[938.302972]  [<c12c8594>] ? do_page_fault+0x32f/0x35e
[938.303173]  [<c10cIf85>] ? kmem_cache_free+0xle/0x4a
[938.303445]  [<c10cd5e7>] ? do_sys_open+0xc3/0xcd
[938.303642]  [<c10d98d1>] ? sys.ioct1+0x44/0x67
[938.303829]  [<c12c9edf>] ? sysenter_do_call+0x!2/0xl2
```

At first, we have to run QEMU in recording mode with USB stick attached.
Then we have
to figure out the addresses in the memory where our module resides.
Address of the specific section ```name``` can be obtained with the command

```cat /sys/module/usb\_info/sections/.name```

The next step is triggering the failure. We created test
program, which uses ```ioctl``` function to
send a message to the loaded kernel module. After starting that program
the system immediately goes to panic.

Now we've got an recorded event log, which can be used for replaying kernel panic scenario.
This event log should be passed into replay engine to start reverse
debugging process. Then we connect to QEMU with GDB 
through remote debugging protocol and load symbol information for our module. 
Symbols can be loaded with the command ```add-symbol-file``` with the offsets
returned by ```cat``` command before.

After examining the panic log we
decided to check the corruption of the return address stored in stack,
because we know that stack was smashed.
We set the breakpoint to the last line of ```usbInfoIoctl``` function 
and continued the execution. 
Then we figured out the memory cell where return address,
set a watchpoint at that address and issued 
```reverse-continue``` command. 

Debugger "continued execution" in backward direction
and stopped at the following line of ```usbInfoIoctl``` function:

```memcpy(result, buf, rand());```
 
Stopping at this function call obviously means buffer overrun with stack
overwriting
and corruption of return address by the unsafe version 
of ```memcpy``` function. ```rand``` function call here
does not allow examining of this failure with traditional cyclic
debugging.

```
(gdb) break 378
Breakpoint 1 at 0xc89e93ba: file 
    /home/user/kernelModule/usb_info.c, line 378.
(gdb) continue
Continuing.

Breakpoint 1, usbInfoIoctl (f=0xc5309f0c, cmd=3349205032,
    arg=3310012224) at /home/user/kernelModule/usb_info.c:378
375	    return 0;
(gdb) info frame 
Stack level 0, frame at 0xc5309f34:
 eip = 0xc89e93ba in usbInfoIoctl (/home/user/kernelModule/
    usb_info.c:271); saved eip 0xc10d9857
 called by frame at 0xc5567bf0
 source language c.
 Arglist at 0xc5309eec, args: f=0xc4f09b80, cmd=2147791873,
    arg=3216111224
 Locals at 0xc5309eec, Previous frame's sp is 0xc5309f34
 Saved registers:
  esi at 0xc5309f28, edi at 0xc5309f2c, eip at 0xc5309f30
(gdb) watch  *0xc5309f30
(gdb) reverse-continue 
Continuing.

Program received signal SIGTRAP, Trace/breakpoint trap.
0xc89e939e in usbInfoIoctl (f=0xc5309f0c, cmd=3349205032,
    arg=3310012224) at /home/user/kernelModule/usb_info.c:371
371	    memcpy(result, buf, size);
```
