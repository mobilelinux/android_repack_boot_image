#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>

int write_to_mtd(char *dev, char *fn)
{
    mtd_info_t mtd_info;           // the MTD structure
    erase_info_t ei;               // the erase block structure

    unsigned char read_buf[4096] = {0x00};                // empty array for reading

    int fdr = open(fn, O_RDONLY);
    int fd = open(dev, O_RDWR); 

    if (fdr < 0 || fd < 0)
        return -1;

    ioctl(fd, MEMGETINFO, &mtd_info);   // get the device info

    ei.length = mtd_info.erasesize;   //set the erase block size
    for(ei.start = 0; ei.start < mtd_info.size; ei.start += ei.length)
    {
        ioctl(fd, MEMUNLOCK, &ei);
        ioctl(fd, MEMERASE, &ei);
    }    

    lseek(fd, 0, SEEK_SET);               // go to the first block
    lseek(fdr, 0, SEEK_SET);               // go to the first block

    ssize_t size = read(fdr, read_buf, sizeof(read_buf)); // read 20 bytes
    ssize_t total = 0;
    while(size != -1 && size != 0) {
        printf("total size:%d\n", total);
       total = total + write(fd, read_buf, size); // write our message
       size = read(fdr, read_buf, sizeof(read_buf)); // read 20 bytes
    }

    printf("total size:%d\n", total);
    close(fdr);
    close(fd);
    return 0;
}

