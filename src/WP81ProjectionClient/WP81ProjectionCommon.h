#pragma once

#ifndef _INC_WINDOWS
#include <Windows.h>
#endif
#ifndef _INC_SHLWAPI
#include <Shlwapi.h>
#endif
#ifndef __WUSB_H__
#include <winusb.h>
#endif

#define WP_SCREEN_TO_PC_FLAG_FOURCC 0x44555608
#define WP_SCREEN_TO_PC_ALIGN_BUFFER_SIZE 512
#define WP_SCREEN_TO_PC_ALIGN512_MAX_SIZE 0xC8600 //854 * 480
#define WP_SCREEN_TO_PC_IMAGE_DATA_OFFSET 0x200

#define WP_SCREEN_TO_PC_CTL_CODE_NOTIFY 2
#define WP_SCREEN_TO_PC_CTL_CODE_READY 3
#define WP_SCREEN_TO_PC_CTL_CODE_FORCE 4

#define WP_SCR_ORI_DEFAULT 0
#define WP_SCR_ORI_NORMAL 1
#define WP_SCR_ORI_HORI_KEY_BACK 2
#define WP_SCR_ORI_HORI_KEY_SEARCH 8

typedef struct tagWP_SCRREN_TO_PC_DATA{
	DWORD dwFourcc; //0x44555608
	union{
		DWORD dwFlags; //Normal flag is 2
		DWORD dwVersion;
	};
	UINT32 nImageDataLength;
	DWORD dwUnk0;
	DWORD dwUnk1;
	DWORD dwUnk2;
	DWORD dwImageBits; //Only set to 16 (565)
	DWORD dwUnk3;
	WORD wRawWidth; //手机原本的分辨率
	WORD wRawHeight;
	WORD wDisplayWidth; //传输到电脑的图像分辨率
	WORD wDisplayHeight;
	WORD wScrOrientation;
}WP_SCRREN_TO_PC_DATA,*PWP_SCRREN_TO_PC_DATA;
//Image data offset in the PWP_SCRREN_TO_PC_DATA+0x200

typedef struct tagWP_SCRREN_TO_PC_PIPE{
	WINUSB_PIPE_INFORMATION PipeOfControl;
	WINUSB_PIPE_INFORMATION PipeOfData;
}WP_SCRREN_TO_PC_PIPE,*PWP_SCRREN_TO_PC_PIPE;