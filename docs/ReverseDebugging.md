# Reverse debugging

Reverse debugging allows "executing" the program in reverse direction.
GDB remote protocol supports "reverse step" and "reverse continue"
commands. The first one steps single instruction backwards in time,
and the second one finds the last breakpoint in the past.

Recorded executions may be used to enable reverse debugging. QEMU can't
execute the code in backwards direction, but can load a snapshot and
replay forward to find the desired position or breakpoint.

The following GDB commands are supported:
 - reverse-stepi (or rsi) - step one instruction backwards
 - reverse-continue (or rc) - find last breakpoint in the past

Reverse step loads the nearest snapshot and replays the execution until
the required instruction is met.

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
