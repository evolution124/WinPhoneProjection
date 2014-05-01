#pragma once

#include "stdafx.h"
#include "InterfaceQueueList.h"
#include "Win_MPEG4_H264_FileContainer.h"

extern IComInterfaceQueue<IWICBitmapSource>* pSharedBitmapQueue;
extern CWinMPEG4FileEncoder* pMP4Encoder;
extern IWICImagingFactory* pWICFactory;
extern HANDLE hEventRender;
extern HANDLE hThreadVideoRecorder,hThreadVideoRecorder2;
extern DWORD dwRecImgWidth,dwRecImgHeight;

VOID CALLBACK QueueWriteVideoSampleThreadProc(PBOOL pbNeedToExit);