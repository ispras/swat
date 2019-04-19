dir=$(pwd)/release-win
mkdir -p build-win
cd build-win

# build qemu

mkdir -p qemu
cd qemu
../../qemu/configure --target-list="i386-softmmu,x86_64-softmmu" --enable-sdl --disable-kvm --disable-capstone --prefix=$dir
make install
cd ..
