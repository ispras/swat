# WinDbg server

**WinDbg** is a multipurpose debugger for the Microsoft Windows computer operating system, distributed by Microsoft. Recent versions of WinDbg have been and are being distributed as part of the free _Debugging Tools for Windows_ suite.
***

How to start debugging QEMU using WinDbg:
* Run QEMU with next option: ```-windbg pipe:<name>```
* QEMU will start and pause for waiting WinDbg connection.
* Run WinDbg with next options: ```-b -k com:pipe,baud=115200,port=\\.\pipe\<name>,resets=0```
* Wait for debugger connect to kernel.  

You can add _Symbol Search Path_ in WinDbg such as   
`srv*c:\tmp*http://msdl.microsoft.com/download/symbols`
