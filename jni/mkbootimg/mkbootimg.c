/* tools/mkbootimg/mkbootimg.c
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>

#include "mincrypt/sha.h"
#include "bootimg.h"
#include "utils.h"

static void *load_file(const char *fn, unsigned *_sz)
{
    char *data;
    int sz;
    FILE *fd;

    data = 0;
    fd = fopen(fn, "rb");
    if(fd == 0) return 0;

    if(fseek(fd, 0, SEEK_END) != 0) goto oops;
    sz = ftell(fd);
    if(sz < 0) goto oops;

    if(fseek(fd, 0, SEEK_SET) != 0) goto oops;

    data = (char*) malloc(sz);
    if(data == 0) goto oops;

    if(fread(data, 1, sz, fd) != (size_t) sz) goto oops;
    fclose(fd);

    if(_sz) *_sz = sz;
    return data;

oops:
    fclose(fd);
    if(data != 0) free(data);
    return 0;
}

static void *mtk_load_ramdisk(const char *fn, unsigned *_sz)
{
    char *data;
    int sz;
    FILE *fd;

    data = 0;
    fd = fopen(fn, "rb");
    if(fd == 0) return 0;

    if(fseek(fd, 0, SEEK_END) != 0) goto oops;
    sz = ftell(fd);
    if(sz < 0) goto oops;

    if(fseek(fd, 0, SEEK_SET) != 0) goto oops;

    data = (char*) malloc(sz + 512);
    if(data == 0) goto oops;

    {
        int i;
        memcpy(data, "\x88\x16\x88\x58", 4);
        memcpy(data + 4, &sz, 4);
        strcpy(data + 8, "ROOTFS");
        for(i=40; i<512; i++) {
            data[i] = '\xFF';
        }
    }

    if(fread(data + 512, 1, sz, fd) != (size_t) sz) goto oops;
    fclose(fd);

    if(_sz) *_sz = (sz + 512);
    return data;

oops:
    fclose(fd);
    if(data != 0) free(data);
    return 0;
}

static int usage(void)
{
    fprintf(stderr,"usage: mkbootimg\n"
            "       --kernel <filename>\n"
            "       --ramdisk <filename>\n"
            "       [ --second <2ndbootloader-filename> ]\n"
            "       [ --cmdline <kernel-commandline> ]\n"
            "       [ --board <boardname> ]\n"
            "       [ --base <address> ]\n"
            "       [ --pagesize <pagesize> ]\n"
            "       [ --id ]\n"
            "       -o|--output <filename>\n"
            );
    return 1;
}



static unsigned char padding[16384] = { 0, };

static void print_id(const uint8_t *id, size_t id_len) {
    printf("0x");
    for (unsigned i = 0; i < id_len; i++) {
        printf("%02x", id[i]);
    }
    printf("\n");
}

int write_padding(FILE *fd, unsigned pagesize, unsigned itemsize)
{
    unsigned pagemask = pagesize - 1;
    ssize_t count;

    if((itemsize & pagemask) == 0) {
        return 0;
    }

    count = pagesize - (itemsize & pagemask);

    if(fwrite(padding, 1, count, fd) != (size_t) count) {
        return -1;
    } else {
        return 0;
    }
}

int repack_main(struct boot_img_hdr *hdr, int isMTKSpecialMode)
{
    void *kernel_data = NULL;
    void *ramdisk_data = NULL;
    char *kernel_fn = "kernel";
    char *ramdisk_fn = "ramdisk.cpio.gz";
    char *second_fn = "second_bootloader";
    char *bootimg = "boot.img";
    void *second_data = NULL;

    FILE *fd;
    SHA_CTX ctx;
    const uint8_t* sha;
    int isMTKModel = isMTKSpecialMode;

    kernel_data = load_file(kernel_fn, &hdr->kernel_size);
    if(kernel_data == 0) {
        fprintf(stderr,"error: could not load kernel '%s'\n", kernel_fn);
        return 1;
    }
    
    if (isMTKModel)
        ramdisk_data = mtk_load_ramdisk(ramdisk_fn, &hdr->ramdisk_size);
    else
        ramdisk_data = load_file(ramdisk_fn, &hdr->ramdisk_size);
    if(ramdisk_data == 0) {
        fprintf(stderr,"error: could not load ramdisk '%s'\n", ramdisk_fn);
        return 1;
    }

    second_data = load_file(second_fn, &hdr->second_size);
    if(second_data == 0) {
        hdr->second_size = 0;
    }

    /* put a hash of the contents in the header so boot images can be
     * differentiated based on their first 2k.
     */
    SHA_init(&ctx);
    SHA_update(&ctx, kernel_data, hdr->kernel_size);
    SHA_update(&ctx, &hdr->kernel_size, sizeof(hdr->kernel_size));
    SHA_update(&ctx, ramdisk_data, hdr->ramdisk_size);
    SHA_update(&ctx, &hdr->ramdisk_size, sizeof(hdr->ramdisk_size));
    SHA_update(&ctx, second_data, hdr->second_size);
    SHA_update(&ctx, &hdr->second_size, sizeof(hdr->second_size));
    sha = SHA_final(&ctx);
    memcpy(hdr->id, sha,
           SHA_DIGEST_SIZE > sizeof(hdr->id) ? sizeof(hdr->id) : SHA_DIGEST_SIZE);
    fd = fopen(bootimg, "wb");
    if(fd == 0) {
        fprintf(stderr,"error: could not create '%s'\n", bootimg);
        return 1;
    }

    if(fwrite(hdr, 1, sizeof(struct boot_img_hdr), fd) != sizeof(struct boot_img_hdr)) goto fail;
    if(write_padding(fd, hdr->page_size, sizeof(struct boot_img_hdr))) goto fail;

    if(fwrite(kernel_data, 1, hdr->kernel_size, fd) != (ssize_t) hdr->kernel_size) goto fail;
    if(write_padding(fd, hdr->page_size, hdr->kernel_size)) goto fail;

    if(fwrite(ramdisk_data, 1, hdr->ramdisk_size, fd) != (ssize_t) hdr->ramdisk_size) goto fail;
    if(write_padding(fd, hdr->page_size, hdr->ramdisk_size)) goto fail;

    if(second_data) {
        if(fwrite(second_data, 1, hdr->second_size, fd) != (ssize_t) hdr->second_size) goto fail;
        if(write_padding(fd, hdr->page_size, hdr->second_size)) goto fail;
    }

    fclose(fd);

    return 0;

fail:
    unlink(bootimg);
    fclose(fd);
    fprintf(stderr,"error: failed writing '%s': %s\n", bootimg,
            strerror(errno));
    return 1;
}
