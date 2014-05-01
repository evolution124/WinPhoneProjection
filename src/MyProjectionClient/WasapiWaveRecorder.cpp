#include "stdafx.h"
#include "WasapiWaveRecorder.h"

CWasapiWaveRecorder::CWasapiWaveRecorder(HANDLE hEvent)
{
	hEventNotify = hEvent;
	hEventRecord = CreateEventExW(NULL,NULL,0,EVENT_ALL_ACCESS);
	InitializeCriticalSectionEx(&cs,0,CRITICAL_SECTION_NO_DEBUG_INFO);
}

CWasapiWaveRecorder::~CWasapiWaveRecorder()
{
	DeleteCriticalSection(&cs);
	CloseHandle(hThreadRecord);
	CloseHandle(hEventRecord);
	CloseHandle(hEventExit);
	if (pwfx)
		CoTaskMemFree(pwfx);
	SAFE_RELEASE(pDevice);
	SAFE_RELEASE(pAudioClient);
	SAFE_RELEASE(pAudioCaptureClient);
	SAFE_RELEASE(pPrevDataBuffer);
}

HRESULT CWasapiWaveRecorder::InitializeFromDefault()
{
	if (hEventNotify == NULL)
		return E_FAIL;
	HRESULT hr = S_OK;
	IMMDeviceEnumerator* pDevEnum = NULL;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),NULL,CLSCTX_ALL,IID_PPV_ARGS(&pDevEnum));
	if (SUCCEEDED(hr))
	{
		hr = pDevEnum->GetDefaultAudioEndpoint(eCapture,eConsole,&pDevice);
		if (SUCCEEDED(hr))
			hr = pDevice->Activate(__uuidof(IAudioClient),CLSCTX_ALL,NULL,(void**)&pAudioClient);
		pDevEnum->Release();
	}
	return hr;
}

HRESULT CWasapiWaveRecorder::InitializeFromDeviceName(LPCWSTR lpszDevName)
{
	if (lpszDevName == NULL)
		return E_POINTER;
	HRESULT hr = E_FAIL;
	IMMDeviceEnumerator* pDevEnum = NULL;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),NULL,CLSCTX_ALL,IID_PPV_ARGS(&pDevEnum));
	if (SUCCEEDED(hr))
	{
		IMMDeviceCollection* pDevItems = NULL;
		if (SUCCEEDED(pDevEnum->EnumAudioEndpoints(eCapture,DEVICE_STATE_ACTIVE|DEVICE_STATE_UNPLUGGED,&pDevItems)))
		{
			UINT nDevice = 0;
			pDevItems->GetCount(&nDevice);
			if (nDevice > 0)
			{
				IMMDevice* pDev;
				for (UINT i = 0;i < nDevice;i++)
				{
					BOOL bExitLoop = FALSE;
					if (SUCCEEDED(pDevItems->Item(i,&pDev)))
					{
						IPropertyStore* pPropStore;
						if (SUCCEEDED(pDev->OpenPropertyStore(STGM_READ,&pPropStore)))
						{
							PROPVARIANT prop;
							PropVariantInit(&prop);
							pPropStore->GetValue(PKEY_DeviceInterface_FriendlyName,&prop);
							if (wcsicmp(prop.pwszVal,lpszDevName) == S_OK)
							{
								pDev->AddRef();
								pDevice = pDev;
								bExitLoop = TRUE;
								hr = pDevice->Activate(__uuidof(IAudioClient),CLSCTX_ALL,NULL,(void**)&pAudioClient);
							}
							PropVariantClear(&prop);
							pPropStore->Release();
						}
						pDev->Release();
					}
					if (bExitLoop)
						break;
				}
			}
			pDevItems->Release();
		}
		pDevEnum->Release();
	}
	return hr;
}

HRESULT CWasapiWaveRecorder::InitializeFromWinPhone()
{
	//return ActivateAudioInterface(GetDefaultAudioCaptureId(Default),__uuidof(IAudioClient),(void**)&pAudioClient);
	return E_ABORT;
}

HRESULT CWasapiWaveRecorder::StartRecorder()
{
	if (pAudioClient == NULL)
		return E_UNEXPECTED;
	HRESULT hr = S_OK;
	PWAVEFORMATEX pWaveTemp = NULL;
	WAVEFORMATEX wfxDefaultCheck = {WAVE_FORMAT_PCM,2,48000,192000,4,16,0};
	if (SUCCEEDED(pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED,&wfxDefaultCheck,&pWaveTemp)))
	{
		pwfx = (PWAVEFORMATEX)CoTaskMemAlloc(sizeof WAVEFORMATEX);
		RtlCopyMemory(pwfx,&wfxDefaultCheck,sizeof WAVEFORMATEX);
	}else
		pAudioClient->GetMixFormat(&pwfx);
	if (pWaveTemp)
		CoTaskMemFree(pWaveTemp);
	hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,AUDCLNT_STREAMFLAGS_EVENTCALLBACK,0,0,pwfx,NULL);
	if (SUCCEEDED(hr))
	{
		pAudioClient->SetEventHandle(hEventRecord);
		hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient),(void**)&pAudioCaptureClient);
		if (SUCCEEDED(hr))
		{
			hr = pAudioClient->Start();
			hThreadRecord = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)&CWasapiWaveRecorder::_StaticRecordThread,this,0,NULL);
			SetThreadPriority(hThreadRecord,THREAD_PRIORITY_TIME_CRITICAL);
		}
	}
	return hr;
}

HRESULT CWasapiWaveRecorder::StopRecorder()
{
	if (pAudioCaptureClient == NULL)
		return E_UNEXPECTED;
	hEventExit = CreateEventExW(NULL,NULL,0,EVENT_ALL_ACCESS);
	InterlockedExchange((PULONG)&bNeedToExit,TRUE);
	if (WaitForSingleObjectEx(hEventExit,2100,FALSE) != WAIT_OBJECT_0)
		TerminateThread(hThreadRecord,-1);
	WaitForSingleObjectEx(hThreadRecord,INFINITE,FALSE);
	CloseHandle(hEventExit);
	hEventExit = NULL;
	return pAudioClient->Stop();
}

PWAVEFORMATEX CWasapiWaveRecorder::GetWaveFormatEx()
{
	return pwfx;
}

IMFMediaBuffer* CWasapiWaveRecorder::GetSoundData()
{
	EnterCriticalSection(&cs);
	IMFMediaBuffer* p = pPrevDataBuffer;
	LeaveCriticalSection(&cs);
	return p;
}

UINT CWasapiWaveRecorder::RecordThread()
{
	_FOREVER_LOOP{
		if (WaitForSingleObjectEx(hEventRecord,INFINITE,FALSE) == WAIT_OBJECT_0)
		{
			if (bNeedToExit)
			{
				SetEvent(hEventExit);
				break;
			}
			UINT32 nNextBufSize = 0;
			pAudioCaptureClient->GetNextPacketSize(&nNextBufSize);
			if (nNextBufSize > 0)
			{
				PBYTE pWaveData = NULL;
				UINT32 nWaveFrames = 0;
				DWORD dwFlags = 0;
				if (SUCCEEDED(pAudioCaptureClient->GetBuffer(&pWaveData,&nWaveFrames,&dwFlags,NULL,NULL)))
				{
					if (!(dwFlags & AUDCLNT_BUFFERFLAGS_SILENT))
					{
						EnterCriticalSection(&cs);
						SAFE_RELEASE(pPrevDataBuffer);
						MFCreateMemoryBuffer(nWaveFrames * pwfx->nBlockAlign,&pPrevDataBuffer);
						if (pPrevDataBuffer)
						{
							PBYTE pBuffer = NULL;
							pPrevDataBuffer->Lock(&pBuffer,NULL,NULL);
							if (pBuffer)
								RtlCopyMemory(pBuffer,pWaveData,nWaveFrames * pwfx->nBlockAlign);
							pPrevDataBuffer->Unlock();
							LeaveCriticalSection(&cs);
							SetEvent(hEventNotify);
						}else
							LeaveCriticalSection(&cs);
					}
					pAudioCaptureClient->ReleaseBuffer(nWaveFrames);
				}
			}
		}
	}
	return NULL;
}

VOID CALLBACK CWasapiWaveRecorder::_StaticRecordThread(PVOID p)
{
	UINT ExitCode = ((CWasapiWaveRecorder*)p)->RecordThread();
	ExitThread(ExitCode);
}