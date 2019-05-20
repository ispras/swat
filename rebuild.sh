dir=$(pwd)/release
mkdir -p build
cd build

# build qemu

mkdir -p qemu
cd qemu
../../qemu/configure --target-list="i386-softmmu,x86_64-softmmu,arm-softmmu,aarch64-softmmu,mips-softmmu,mips64-softmmu" --enable-sdl --disable-kvm --enable-plugins --prefix=$dir
make install
cd ..

# build multiarch gdb
mkdir -p gdb
cd gdb
../../binutils-gdb/configure \
    --prefix=$dir \
    --disable-gdbtk \
    --disable-shared \
    --disable-readline \
    --with-system-readline \
    --with-expat \
    --with-system-zlib \
    --without-guile \
    --without-babeltrace \
    --with-babeltrace \
    --enable-tui \
    --with-lzma \
    --with-python=python3 \
    --enable-64-bit-bfd \
    --disable-sim \
    --enable-targets="aarch64-linux-gnu arm-linux-gnu arm-linux-gnueabi arm-linux-gnueabihf i686-linux-gnu ia64-linux-gnu mips-linux-gnu mipsel-linux-gnu mips64-linux-gnu mips64el-linux-gnu x86_64-linux-gnu"

make
make install
cd ..

# build everything else

cmake -DCMAKE_INSTALL_PREFIX=../release ..
make install
