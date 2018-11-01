#ifndef _REPACKIMG_H_
#define _REPACKIMG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _REPACKIMG_STANDALONE_
int main(int argc, char **argv);
#else
int repackimg_main(void);
#endif

#ifdef __cplusplus 
}
#endif


#endif