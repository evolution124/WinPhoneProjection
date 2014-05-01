#pragma once

#include "stdafx.h"

class CWinMPEG4FileEncoder
{
private:
	LPWSTR lpstrFileName = NULL;
	IMFSinkWriter* pSinkWriter = NULL;
	IMFMediaType *pVideoInType = NULL,*pVideoOutType = NULL;
	IMFMediaType *pAudioInType = NULL,*pAudioOutType = NULL;
	DWORD dwVideoStreamIndex = 0,dwAudioStreamIndex = 0;
	MFTIME videoDuration = 0,audioDuration = 0;
	CRITICAL_SECTION cs;
private:
	DWORD dwVideoWidth = 0,dwVideoHeight = 0,dwImagePitch = 0;
	DWORD dwVideoFps = 0;
	UINT nVideoBitRate = 0;
	UINT nVideoFrameSize = 0;
private:
	WAVEFORMATEX wfx;
public:
	CWinMPEG4FileEncoder(LPWSTR lpszSaveFileName);
	~CWinMPEG4FileEncoder();
public:
	HRESULT InitializeSinkWriter(BOOL bUseHardwareEncoder = FALSE);
	HRESULT InitializeVideoStream(DWORD dwWidth,DWORD dwHeight,DWORD dwFps,DWORD dwBitRate); //H.264
	HRESULT InitializeAudioStream(PWAVEFORMATEX pwfx,UINT nAvgBytesPerSec = 40000); //MP3
	HRESULT StartWriteStreams();
	HRESULT WriteVideoSample(IMFMediaBuffer* pVideoSample,MFTIME duration);
	HRESULT WriteAudioSample(IMFMediaBuffer* pAudioSample,MFTIME duration);
	HRESULT ShutdownAndSaveFile();
};