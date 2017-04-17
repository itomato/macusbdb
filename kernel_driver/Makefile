KERNELPATH=/lib/modules/`uname -r`/build
#KERNELPATH=/home/jeroen/dockstar/linux-2.6.35.3

macusbfb.ko: macusbfb.c
	make -C ${KERNELPATH} M=`pwd`

install: macusbfb.ko
	make -C ${KERNELPATH} M=`pwd` modules_install

clean:
	make -C ${KERNELPATH} M=`pwd` clean


