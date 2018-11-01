/******************************************************************************
 *
 * AUTHOR
 *      lihouchen <lihouchen@handscape.com.cn>
 *
 *****************************************************************************/
#ifndef _LOG_H
#define _LOG_H

#include <android/log.h>

// #define SHOW_DETAIL_STEP 1

// #define DEBUG
// #define ANDROID_LOG 1

#ifndef LOG_TAG
#define LOG_TAG "zroot"
#endif


#ifdef DEBUG
#ifdef ANDROID_LOG
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , LOG_TAG, __VA_ARGS__)
#else   // #ifdef ANDROID_LOG
#define LOGV(...) printf(__VA_ARGS__)
#define LOGD(...) printf(__VA_ARGS__)
#define LOGI(...) printf(__VA_ARGS__)
#define LOGW(...) printf(__VA_ARGS__)
#define LOGE(...) printf(__VA_ARGS__)
#endif  // #ifdef ANDROID_LOG
#else   // #ifdef DEBUG
#define LOGV(...)
#define LOGD(...)
#define LOGI(...)
#define LOGW(...)
#define LOGE(...)
#endif  // #ifdef DEBUG

//char logbuff[256];

#endif // _LOG_H
