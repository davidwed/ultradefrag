#define VERSION 7,1,1,0
#define VERSION2 "7, 1, 1, 0\0"
#define VERSIONINTITLE "UltraDefrag 7.1.1"
#define VERSIONINTITLE_PORTABLE "UltraDefrag 7.1.1 Portable"
#define ABOUT_VERSION "Ultra Defragmenter version 7.1.1"
#define wxUD_ABOUT_VERSION "7.1.1"

#ifndef _WIN64
  #define UDEFRAG_ARCH "x86"
#else
 #if defined(_IA64_)
  #define UDEFRAG_ARCH "ia64"
 #else
  #define UDEFRAG_ARCH "x64"
 #endif
#endif

#define _UD_HIDE_PREVIEW_
