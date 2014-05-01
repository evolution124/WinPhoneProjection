#pragma once

#include "stdafx.h"

class CWasapiWaveRecorder
{
private:
	IMMDevice* pDevice = NULL;
	IAudioClient* pAudioClient = NULL;
	IAudioCaptureClient* pAudioCaptureClient = NULL;
	IMFMediaBuffer* pPrevDataBuffer = NULL;
	CRITICAL_SECTION cs;
private:
	PWAVEFORMATEX pwfx = NULL;
	HANDLE hThreadRecord = NULL;
	HANDLE hEventRecord = NULL,hEventNotify = NULL,hEventExit = NULL;
	BOOL bNeedToExit = FALSE;
public:
	CWasapiWaveRecorder(HANDLE hEvent);
	~CWasapiWaveRecorder();
public:
	HRESULT InitializeFromDefault();
	HRESULT InitializeFromDeviceName(LPCWSTR lpszDevName);
	HRESULT InitializeFromWinPhone();
	HRESULT StartRecorder();
	HRESULT StopRecorder();
	PWAVEFORMATEX GetWaveFormatEx();
	IMFMediaBuffer* GetSoundData();
protected:
	virtual UINT RecordThread();
protected:
	static VOID CALLBACK _StaticRecordThread(PVOID p);
};