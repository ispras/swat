# swat
SWAT - System-Wide Analysis Toolkit

## WinDbg server

SWAT supports debugging with WinDbg without switching the guest OS to the
debug mode.

See more details in [documentation](docs/WinDbg.md)

## Reverse debugging

Reverse debugging allows "executing" the program in reverse direction.

See more details in [reverse debugging documentation](docs/ReverseDebugging.md)

## Virtual machine introspection

SWAT supports non-intrusive introspection of the virtual machine.

Non-intrusiveness infers the following features:
* No need in loading any agents into the guest system
* Analysis and introspection can work when execution is replayed

See more details in [introspection documentation](docs/Introspection.md)

## Building the SWAT

    git clone https://github.com/ispras/swat
    cd swat
    git submodule update --init
    ./rebuild.sh
