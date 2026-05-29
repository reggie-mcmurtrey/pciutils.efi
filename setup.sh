#install any dependancies
sudo apt update
sudo apt install build-essential git uuid-dev iasl nasm python3 python-is-python3

#clone edk2
git clone https://github.com/tianocore/edk2.git
cd edk2
git submodule update --init --recursive
git checkout edk2-stable202605
cd ..

#clone edk2-libc
git clone https://github.com/tianocore/edk2-libc.git
cd edk2-libc
git checkout v3.6.8.2
cd ..

#symlinks from edk2-libc to edk2
cd edk2
ln -s ../edk2-libc/AppPkg AppPkg
ln -s ../edk2-libc/StdLib StdLib
ln -s ../edk2-libc/StdLibPrivateInternalFiles StdLibPrivateInternalFiles
cd ..

#clone pciutils.efi
git clone https://github.com/reggie-mcmurtrey/pciutils.efi.git

#symlinks from pciutils.efi to edk2
cd edk2
ln -s ../pciutils.efi/PciUtilsPkg PciUtilsPkg
cd ..

#clone pciutils
git clone https://github.com/pciutils/pciutils.git
cd pciutils
git checkout v3.15.0
cd ..

#symlinks from pciutils to pciutils.efi
cd pciutils.efi
cd PciUtilsPkg
ln -s ../../pciutils pciutils
cd ..
cd ..

#build everything
cd edk2
source edksetup.sh
make -C BaseTools
build -p PciUtilsPkg/PciUtilsPkg.dsc -a X64 -t GCC -b RELEASE
