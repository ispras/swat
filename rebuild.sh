dir=$(pwd)/release
mkdir build
mkdir -p release/bin
cd build
mkdir qemu-gui
cd qemu-gui
cmake ../../qemu-gui
make
cp qemu-gui $dir/bin
cd ..
mkdir qemu
cd qemu
../../qemu/configure --target-list="i386-softmmu,x86_64-softmmu,arm-softmmu,aarch64-softmmu,mips-softmmu,mips64-softmmu" --enable-sdl --disable-kvm --enable-plugins --prefix=$dir
make install
