#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdarg.h>
#include <fcntl.h>

#include "minicpio.h"
#include "utils.h"

typedef unsigned char byte;

int check_cpio_magic(const char *magic, const char *file)
{
    int count;
    for(count = 0;count < 6;count++) {
        if (magic[count] != CPIO_MAGIC[count]) {
        fprintf(stderr, "ERROR: unable to parse '%s': unknown file types\n\n",
			file);
            return -1;
        }
    }
    return 0;
}

int usage()
{
    fprintf(stderr,"usage: uncpio\n"
                               "\t[ -c|--cpio   cpio file ]\n"
                               "\t[ -o|--output output directory ]\n"
                               "\t[ -w|--write  output cpio list ]\n");
    return 1;
}

int uncpio_main(const char* cpiofile, const char *directory)
{
    int swrx;
    int namesize;
    int filesize;
    int total_size;
    int if_slink;
    int if_file;
    int ret = 0;
    int uid = 0;
    int gid = 0;

    mini_cpio_hdr header;
    unsigned char tmp[9];
    unsigned char tmp_path[PATH_MAX];
    unsigned char name[PATH_MAX];
    unsigned char slink_path[PATH_MAX];

    swrx = namesize = filesize = total_size = if_slink = if_file = 0;
    FILE *f;

    if ((f = fopen(cpiofile, "rb")) == NULL) {
        fprintf(stderr, "ERROR: unable to open '%s': %s\n\n",
			cpiofile, strerror(errno));
        return -1;
    }

    if(access(directory, R_OK) == -1){
    	mkdir(directory, 0777);
    }
    
    while(!feof(f)) {
        memset(name, 0, sizeof(name));
        memset(tmp_path, 0, sizeof(tmp_path));
        memset(slink_path, 0, sizeof(slink_path));
        tmp[8] = '\0';

        while(total_size & 3) {
            total_size++;
            fseek(f, 1, SEEK_CUR);
        }
        fread(&header, sizeof(header), 1, f);
        if(check_cpio_magic(header.magic, cpiofile) !=0) {
            ret = -2;
            goto clean;
        }
        memcpy(tmp,header.mode,8);
        swrx = (unsigned)strtoul(tmp, 0, 16);
        memcpy(tmp,header.namesize,8);
        namesize = (unsigned)strtoul(tmp, 0, 16);
        memcpy(tmp,header.filesize,8);
        filesize = (unsigned)strtoul(tmp, 0, 16);
        memcpy(tmp,header.uid, 8);
        uid = (unsigned)strtoul(tmp, 0, 16);
        memcpy(tmp,header.gid, 8);
        gid = (unsigned)strtoul(tmp, 0, 16);

        if(namesize < 0 || filesize < 0 || uid < 0 || gid < 0) {
            ret = -1;
            goto clean;
        }

        fread(&name, namesize-1, 1, f);
        fseek(f, 1, SEEK_CUR);
        if(!strcmp(name, "TRAILER!!!"))
            break;

        total_size += 6 + 8*13 + (namesize-1) + 1;

        while(total_size & 3) {
            total_size++;
            fseek(f, 1, SEEK_CUR);
        }
      /*"file <name> <location> <mode> <uid> <gid> [<hard links>]\n"
        "dir <name> <mode> <uid> <gid>\n"
        "nod <name> <mode> <uid> <gid> <dev_type> <maj> <min>\n"
        "slink <name> <target> <mode> <uid> <gid>\n"
        "pipe <name> <mode> <uid> <gid>\n"
        "sock <name> <mode> <uid> <gid>\n"*/
        if (S_ISDIR(swrx)) {
            swrx = swrx & 0777;
            sprintf(tmp_path,"%s/%s",directory,name);
            mkdir(tmp_path,swrx);
            chown(tmp_path, uid, gid);
            // fprintf(stdout, "dir %s 0%o 0 0\n", name, swrx);
            memset(tmp_path, 0, sizeof(tmp_path));
        } else if (S_ISREG(swrx)) {
            swrx = swrx & 0777;
            // fprintf(stdout, "file %s %s/%s 0%o 0 0\n", name, directory, name, swrx);
            if_file = 1;
        } else if (S_ISLNK(swrx)) {
            swrx = swrx & 0777;
            sprintf(tmp_path,"%s/%s",directory,name);
            fread(&slink_path, filesize, 1, f);
            symlink(slink_path, tmp_path);
            fseek(f,-filesize,SEEK_CUR);
            if_slink = 1;
        }
        if(if_file && !if_slink) {
            sprintf(tmp_path,"%s/%s",directory,name);
            FILE *r = fopen(tmp_path, "wb");
            byte* filedata = (byte*)malloc(filesize);
            fread(filedata, filesize, 1, f);
            fwrite(filedata, filesize, 1, r);
            fclose(r);
            chmod(tmp_path, swrx);
            chown(tmp_path, uid, gid);
            //fseek(f,filesize,SEEK_CUR);
            total_size += filesize;
            if_file = 0;
        } else {
            fseek(f,filesize,SEEK_CUR);
            total_size += filesize;
            if_slink = 0;
        }
    }

clean:
    fclose(f);
    return ret;
}
