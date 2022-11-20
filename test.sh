lsmod | grep vtagfs

mkdir -p /mnt/vtagfs2

insmod vtagfs.ko
mount -t vtagfs none /mnt/vtagfs2


touch /mnt/vtagfs/f1
# echo "hello world" > /mnt/vtagfs/f1
cat /mnt/vtagfs/f1

mkdir -p /mnt/vtagfs/ab1/ert2/e3
# touch /mnt/vtagfs/d1/f1
ls -lai /mnt/vtagfs/d2

# ls -lai /mnt/vtagfs

umount /mnt/vtagfs
rmmod vtagfs

dmesg | tail -10



# dmesg | tail -100 | grep -i file_mmu


make clean
cd ./include/ramfs/
rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.mod modules.order *.symvers .*.o.d .*.o.cmd

cd ../dcache/
rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.mod modules.order *.symvers .*.o.d .*.o.cmd
cd ../..

# my_dcache_dir_open
# my_dcache_readdir
# my_dcache_dir_close