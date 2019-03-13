# Reverse debugging

Reverse debugging allows "executing" the program in reverse direction.
GDB remote protocol supports "reverse step", "reverse continue" and "reverse-roll"
commands. The first one steps single instruction backwards in time,
and the second one finds the last breakpoint in the past. Reverse-roll command

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

![Qemu into standby mode](/imgs/replay.png)

After that, the debugger should be running and connect to the simulator:   
`gdb -ex 'tar rem :1234'`   

![Remote connection of debugger](/imgs/debugger_remote_connection.png)

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
