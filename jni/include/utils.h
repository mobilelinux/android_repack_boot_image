#ifndef __BOOT_IMG_UTILS__
#define __BOOT_IMG_UTILS__

#ifdef __cplusplus
extern "C" {
#endif

int is_mtk_device(void);
int is_gt9308_cmcc(void);

int _mkdir(const char *dir);
int remove_directory(const char *path);

int archive_dir(char *dir, const char *fn);
int uncpio_main(const char* cpiofile, const char *directory);

#ifdef __cplusplus
}
#endif

#endif