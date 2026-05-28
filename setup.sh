#install any dependancies
sudo apt update
sudo apt install build-essential git uuid-dev iasl nasm python3 python-is-python3

#clone edk2
git clone https://github.com/tianocore/edk2.git

#clone edk2-libc
git clone https://github.com/tianocore/edk2-libc.git

#symlinks from edk2-libc to edk2
cd edk2
ln -s ../edk2-libc/StdLib StdLib
ln -s ../edk2-libc/StdLibPrivateInternalFiles StdLibPrivateInternalFiles
cd ..

#clone pciutils.efi
#FIXME: This need to point to my fork
git clone https://github.com/timotheuslin/pciutils.efi.git

#symlinks from pciutils.efi to edk2
cd edk2
ln -s ../pciutils.efi/PciUtilsPkg PciUtilsPkg
cd ..

#clone pciutils
git clone https://github.com/pciutils/pciutils.git

#symlinks from pciutils to pciutils.efi
cd pciutils.efi/PciUtilsPkg
ln -s ../../pciutils pciutils
cd ..

#build everything
cd edk2
make -C BaseTools
source edksetup.sh
build -p PciUtilsPkg/PciUtilsPkg.dsc -a X64 -t GCC -b RELEASE
