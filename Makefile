ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
obj-m := vtagfs.o
vtagfs-y := vtagfs_mod.o
vtagfs-y +=  include/ramfs/libfs.o include/ramfs/file-mmu.o include/ramfs/inode.o

vtagfs-y += include/dcache/dcache.o


ccflags-y := -I$(src)
ccflags-y := -I$(src)/include/ramfs
ccflags-y := -I$(src)/include/dcache


else
# normal makefile
KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD modules
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.mod modules.order *.symvers .*.o.d .*.o.cmd

endif


# obj-m += vtagfsv3.o
# .PHONY: all clean
# default: all

# all:
# 	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
# clean:
# 	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
