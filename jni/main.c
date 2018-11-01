#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>

#include "bmt.h"
#include "utils.h"
#include "bootimg.h"
#include "log.h"

#define CONTENT "\n\
service aee /sbin/test.d\n\
    class main\n\
    user root\n\
    group root\n"

#define REC_CONTENT "service aee /sbin/test.d\n"

#define DAEMON "/sbin/test.d"


//test sample
static const unsigned char content[] = {
	'T', 'e', 's', 't', ' ', 'C', 'o', 'n', 't', 'e', 'n', 't'
};

#define content_LENGTH sizeof(content)

int check_same(const char *fn1, const char *fn2) {
	FILE *f1 = fopen(fn1, "r");
	FILE *f2 = fopen(fn2, "r");

	if (f1 == NULL || f2 == NULL)
		return -1;

	size_t size;
	char *b1[1024];
	char *b2[1024];
	int ret = 0;
	
	do {
		size = fread(b1, 1, 1024, f1);
		if (size > 0) {
			if (fread(b2, 1, size, f2) != size) {
				ret = -2;
				goto clean;
			}
			if (memcmp(b1, b2, size) != 0) {
				ret = -3;
				goto clean;
			}
		}

	} while(size > 0);

clean:
	fclose(f1);
	fclose(f2);
	return ret;
}

int modify_content() {
	char *path_initrc = (BMT_WORK_PATH"ramdisk/init.rc");
	char *path_daemon = (BMT_WORK_PATH"ramdisk/sbin/test.d");
	FILE *fp = fopen(path_initrc, "r"); 

	char buff[256];

    memset(buff, 0, sizeof(buff));

	if (fp == NULL) {
		return -1;
	}

    while(fgets(buff, 256, fp)) {
    	char *already_in = strstr(buff, REC_CONTENT);
        if (already_in) {
        	fclose(fp);
        	
        	if (check_same(path_daemon, "/sbin/test.d"))
        		return -3;

            goto write_content;
        }
    }
    
	fclose(fp);

	fp = fopen(path_initrc, "a+");

	if (fp == NULL) {
		return -1;
	}

	if (fwrite(CONTENT, 1, sizeof(CONTENT), fp) != sizeof(CONTENT)) {
		fclose(fp);
		return -1;
	}

	fclose(fp);

write_content:
	fp = fopen(path_daemon, "w");
	if (fp == NULL) {
		return -1;
	}

	if (fwrite(content, 1, content_LENGTH, fp) != content_LENGTH) {
		fclose(fp);
		return -1;
	}
	fclose(fp);

	chmod(path_daemon, S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP);

	return 0;
}

#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8

#define TYPE_MTD 1
#define TYPE_EMMC 2

int get_device_from_proc_file(int type, char *device)
{	
	char *fn = NULL;

	if (TYPE_EMMC == type) {
		fn = "/proc/emmc";
	} else if(TYPE_MTD == type){
		fn = "/proc/mtd";
	} else {
		return -1;
	}

	FILE *fp = fopen(fn, "r");
	if(fp != NULL) {
		int match;
		char dev[64];
		char size[20];
		char erasesize[20];
		char par_name[32];
		char line[256];
		int c;
		
		do {
			// read one line
		 	int pos = 0;
	        do{ 
	          	c = fgetc(fp);
	          	if(c != EOF) 
	          		line[pos++] = (char)c;
	        }while(c != EOF && c != '\n');
	        
	        if (line[pos] != '\n')
	        	line[pos++] = '\n';

	        line[pos] = '\0';

            if (strncmp(line, "dev", 3) == 0) {
            	continue;
            }

			match = sscanf(line, "%64s %s %s %32s\n",
                       dev, size, erasesize, par_name);

			if(match == 4 && strcmp(par_name,"\"boot\"") == 0) {
				fclose(fp);
				int dev_len = strlen(dev);
				if (dev[dev_len - 1] == ':')
					dev[dev_len - 1] = '\0';
				if(TYPE_MTD == type)
					sprintf(device, "/dev/mtd/%s", dev);
				else {
					sprintf(device, "/dev/block/%s", dev);
				}
				return 0;
			}

		} while(c != EOF);

		fclose(fp);
	}
	return -1;
}

int get_special_boot_device(char *device, int length, int *dev_offset, int *is_mtd_device) {
	DIR *dir;
	struct dirent *de;

	if(is_mtk_device()) {
		if(access("/dev/bootimg", R_OK) ==0) {
			strcpy(device, "/dev/bootimg");
			return 0;
		}
	}

	if (!(dir = opendir("/dev/block/platform/"))) {
		return -1;
	}

	while ((de = readdir(dir))) {
		char path2[256];
		DIR * dir2;
		if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..")==0)
			continue;
		sprintf(path2, "/dev/block/platform/%s/by-name/", de->d_name);

		if (!(dir2 = opendir(path2))) {
			continue;
 		}
 
 		while(de = readdir(dir2)) {
 			if(strcasecmp(de->d_name, "boot") == 0) {
 				sprintf(device,"%s%s", path2, de->d_name);
 				closedir(dir2);
 				closedir(dir);
 				return 0;
 			}
 		}
 		closedir(dir2);
	}

	closedir(dir);

	if(get_device_from_proc_file(TYPE_MTD, device) == 0) {
		// int fd;
		// int offset = 0;
		// int count;
		// char magic[BOOT_MAGIC_SIZE];
		// if ((fd = open(device, O_RDONLY)) < 0) {
		// 	return -1;
		// } else {
		// 	mtd_info_t mtd_info;
		// 	ioctl(fd, MEMGETINFO, &mtd_info); 
		// 	offset = mtd_info.size - 0x1000;
		// 	do {
		// 		count = read(fd, magic, sizeof(magic));
		// 		if (memcmp(magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) == 0) {
		// 			close(fd);
		// 			*dev_offset = offset;
		// 			*is_mtd_device = 1;
		// 			return 0;        	
		// 		}
		// 		offset -= 0x1000;
		// 		lseek(fd, offset, SEEK_SET);
		// 	} while(offset >= 0 && count == sizeof(magic));
		// 	close(fd);
		// 	return -1;
		// }
		*is_mtd_device = 1;
		return 0;
	}

	if(get_device_from_proc_file(TYPE_EMMC, device) == 0)
		return 0;

	return -1;
}

int try_get_boot_device(char *device, int length, DIR *dir) {
	struct dirent *de;

	while ((de = readdir(dir))) {
		int fd;
		char magic[BOOT_MAGIC_SIZE];
		if(strncmp(de->d_name, "mmcblk", strlen("mmcblk")) != 0 &&
			strncmp(de->d_name, "mtd", strlen("mtd")) != 0) {
			continue;
		}
	
		sprintf(device,"/dev/block/%s", de->d_name);

		if ((fd = open(device, O_RDONLY)) < 0) {
			continue;
		} else {
			read(fd, magic, sizeof(magic));
			close(fd);
		}

		if (memcmp(magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) == 0) {
			return 0;        	
        }
	}	
	return -2;
}

static struct boot_img_hdr hdr;

int dump_bootimg(const char *boot_dev, const char * fn) {
	FILE *fr = fopen(boot_dev, "r");
	FILE *fp = fopen(fn, "w");
	size_t size;
	char * buff[4096];
	int ret = 0;

	if (fp == NULL || fr == NULL) {
		return -1;
	}

	do {
		size = fread(buff, 1, 4096, fr);
		if (size > 0) {
			if (fwrite(buff, 1, size, fp) != size) {
				fclose(fp);
				ret = -1;
				goto clean;
			}
		}

	} while(size > 0);

clean:
	fclose(fr);
	fclose(fp);
	return ret;

}

int try_unpack(char *cmd, char *boot_dev, int check_recovery, int is_mtd, int offset) {
		//retrive boot image from device
	int is_mtk_special = 0;
	if (is_mtk_device() && (strcmp(boot_dev, "/dev/bootimg") == 0)) {
		//retrive boot image from device
		is_mtk_special = 1;
		sprintf(cmd, "/system/bin/dd if=%s of=%sboot.img bs=10485760 count=1", boot_dev, BMT_WORK_PATH);
		system(cmd);
	} 
	else {
		if (dump_bootimg(boot_dev, BMT_WORK_PATH"boot.img") !=0) {
			return -1;
		}
	}

	remove_directory(BMT_WORK_PATH"ramdisk");
	mkdir(BMT_WORK_PATH"ramdisk", S_IRWXU);
	
	chdir(BMT_WORK_PATH);

	if(unpack_main(&hdr, is_mtk_special) !=0) {
		return -1;
	}

	if(decompress_main("ramdisk.cpio.gz", "ramdisk.cpio")) {
		// printf("gzip return:%d error:%s\n", retval, strerror(errno));
		return -1;
	}
	if(uncpio_main("ramdisk.cpio", "ramdisk") != 0) {
		return -1;
	}

	rename("ramdisk.cpio.gz", "ramdisk-orig.cpio.gz");
	rename("boot.img", "boot-orig.img");

	if (check_recovery && check_same(BMT_WORK_PATH"ramdisk/init.rc", "/init.rc") != 0) {
		//find a recovery image, re-find boot image
		LOGD("recovery image\n");
		return -2;
	}

	return 0;

}

int main(int argc, char **argv)
{
	char cmd[512];
	char boot_dev[128];
	int ret = 0;
	int offset = 0;
	int is_mtd_device = 0;
	char cwd[PATH_MAX];

	getcwd(cwd, PATH_MAX);
	
	_mkdir(BMT_WORK_PATH);

	if (get_special_boot_device(boot_dev, sizeof(boot_dev), &offset, &is_mtd_device) == 0) {
		if (try_unpack(cmd, boot_dev, 0, is_mtd_device, offset) != 0) {
			ret = 1;
			goto fail;
		}
	}
	else {
		DIR *dir;
		int found = 0;
		if (!(dir = opendir("/dev/block/"))) {
			ret = 2;
			goto fail;
		}
		while (try_get_boot_device(boot_dev, sizeof(boot_dev), dir) == 0) {
			//find a possible device
			LOGD("try boot_dev:%s\n", boot_dev);
			if (try_unpack(cmd, boot_dev, 1, is_mtd_device, offset) == 0) {
				found = 1;
				break;
			}
		}
		closedir(dir);
		if(!found) {
			LOGD("can't find boot dev\n");
			ret = 4;
			goto fail;
		}
	}

	if ((modify_content()) !=0) {
		ret = 3;
		goto fail;
	}
	chdir(BMT_WORK_PATH"ramdisk");
	archive_dir(".", "../ramdisk.cpio");

	chdir(BMT_WORK_PATH);

	if(compress_main("ramdisk.cpio", "ramdisk.cpio.gz")) {
		ret = 5;
		goto fail;
	}
	repack_main(&hdr, (strcmp(boot_dev, "/dev/bootimg")==0)?1:0);

	dump_bootimg("boot.img", boot_dev);
	LOGD("repack to %s", boot_dev);
	chdir(cwd);
	remove_directory(BMT_WORK_PATH);
	return 0;

fail:
	//remove work path
	LOGD("failed, ret=%d\n", ret);
	chdir(cwd);
	remove_directory(BMT_WORK_PATH);
	return ret;
}
