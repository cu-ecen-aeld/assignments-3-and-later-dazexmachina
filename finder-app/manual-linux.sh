#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_0
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
TOOLCHAIN_DIR=$(dirname $(dirname $(which ${CROSS_COMPILE}gcc)))

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
    cd ..
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
    mkdir -p rootfs
    cd rootfs
else
    mkdir -p rootfs
    cd rootfs
fi

mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p home/conf
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git config --global core.compression 0
    git clone --depth 1 git://busybox.net/busybox.git
    cd busybox
    git fetch --unshallow
    git pull --all
    git checkout ${BUSYBOX_VERSION}
else
    cd busybox
fi

make distclean
make defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=../rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a busybox | grep "Shared library"

cd ${OUTDIR}/rootfs

cp ${TOOLCHAIN_DIR}/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 lib/ld-linux-aarch64.so.1
cp ${TOOLCHAIN_DIR}/aarch64-none-linux-gnu/libc/lib64/libm.so.6 lib64/libm.so.6
cp ${TOOLCHAIN_DIR}/aarch64-none-linux-gnu/libc/lib64/libresolv.so.2 lib64/libresolv.so.2
cp ${TOOLCHAIN_DIR}/aarch64-none-linux-gnu/libc/lib64/libc.so.6 lib64/libc.so.6

sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/ttyS1 c 5 1

cd ${FINDER_APP_DIR}

make clean
make CROSS_COMPILE=${CROSS_COMPILE} all

cp writer ${OUTDIR}/rootfs/home/writer
cp finder.sh ${OUTDIR}/rootfs/home/finder.sh
cp finder-test.sh ${OUTDIR}/rootfs/home/finder-test.sh
cp conf/username.txt ${OUTDIR}/rootfs/home/conf/username.txt
cp conf/assignment.txt ${OUTDIR}/rootfs/home/conf/assignment.txt
cp autorun-qemu.sh ${OUTDIR}/rootfs/home/autorun-qemu.sh

sudo chown -R root:root ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio


