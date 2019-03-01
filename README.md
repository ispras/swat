# swat
SWAT - System-Wide Analysis Toolkit

## WinDbg server

SWAT supports debugging with WinDbg without switching the guest OS to the
debug mode.

See more details in [documentation](docs/WinDbg.md)

## Reverse debugging

Reverse debugging allows "executing" the program in reverse direction.

See more details in [reverse debugging documentation](docs/ReverseDebugging.md)

## Building the SWAT

    git clone https://github.com/ispras/swat
    cd swat
    git submodule update --init
    ./rebuild.sh
