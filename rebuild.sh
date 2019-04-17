dir=$(pwd)/release
mkdir -p build
cd build

# build qemu

mkdir -p qemu
cd qemu
../../qemu/configure --target-list="i386-softmmu,x86_64-softmmu,arm-softmmu,aarch64-softmmu,mips-softmmu,mips64-softmmu" --enable-sdl --disable-kvm --enable-plugins --prefix=$dir
make install
cd ..

# build gdb

mkdir -p gdb
cd gdb
../../binutils-gdb/configure --target=i386-pc-pe --prefix=$dir
make
make install
cd ..

# build everything else

cmake -DCMAKE_INSTALL_PREFIX=../release ..
make install
