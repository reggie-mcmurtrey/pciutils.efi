export PACKAGES_PATH=${WORKSPACE}/
cd ../edk2
build -p PciUtilsPkg/PciUtilsPkg.dsc -a X64 -t GCC -b RELEASE
