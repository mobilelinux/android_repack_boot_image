#ifndef __BOOT_IMG_UTILS__
#define __BOOT_IMG_UTILS__

#ifdef __cplusplus
extern "C" {
#endif

int is_mtk_device();
int _mkdir(const char *dir);
int is_gt9308_cmcc(void);

int write_to_mtd(const char *dev, const char *fn);

int decompress_main(const char *fn, const char *fn_out);
int compress_main(const char *fn, const char *fn_out);

#ifdef __cplusplus
}
#endif

#endif