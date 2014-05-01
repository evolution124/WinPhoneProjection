#include "stdafx.h"
#include "Win_MPEG4_H264_FileContainer.h"

CWinMPEG4FileEncoder::CWinMPEG4FileEncoder(LPWSTR lpszSaveFileName)
{
	lpstrFileName = StrDupW(lpszSaveFileName);
	InitializeCriticalSectionEx(&cs,0,CRITICAL_SECTION_NO_DEBUG_INFO);
}

CWinMPEG4FileEncoder::~CWinMPEG4FileEncoder()
{
	DeleteCriticalSection(&cs);
	if (lpstrFileName)
		CoTaskMemFree(lpstrFileName);
	SAFE_RELEASE(pVideoInType);
	SAFE_RELEASE(pAudioInType);
	SAFE_RELEASE(pVideoOutType);
	SAFE_RELEASE(pAudioOutType);
	SAFE_RELEASE(pSinkWriter);
}

HRESULT CWinMPEG4FileEncoder::InitializeSinkWriter(BOOL bUseHardwareEncoder)
{
	if (lpstrFileName == NULL)
		return E_POINTER;
	IMFAttributes* pAttr = NULL;
	MFCreateAttributes(&pAttr,2);
	pAttr->SetGUID(MF_TRANSCODE_CONTAINERTYPE,MFTranscodeContainerType_MPEG4);
	if (bUseHardwareEncoder)
		pAttr->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS,TRUE);
	DeleteFileW(lpstrFileName);
	HRESULT hr = MFCreateSinkWriterFromURL(lpstrFileName,NULL,pAttr,&pSinkWriter);
	pAttr->Release();
	return hr;
}

HRESULT CWinMPEG4FileEncoder::InitializeVideoStream(DWORD dwWidth,DWORD dwHeight,DWORD dwFps,DWORD dwBitRate)
{
	if (pSinkWriter == NULL)
		return E_UNEXPECTED;
	MFCreateMediaType(&pVideoInType);
	MFCreateMediaType(&pVideoOutType);
	pVideoInType->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video);
	pVideoOutType->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video);
	pVideoInType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_RGB32);
	pVideoOutType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_H264);
	pVideoInType->SetUINT32(MF_MT_INTERLACE_MODE,MFVideoInterlace_Progressive);
	pVideoOutType->SetUINT32(MF_MT_INTERLACE_MODE,MFVideoInterlace_Progressive);
	pVideoOutType->SetUINT32(MF_MT_AVG_BITRATE,dwBitRate);
	MFSetAttributeSize(pVideoOutType,MF_MT_FRAME_SIZE,dwWidth,dwHeight);
	MFSetAttributeRatio(pVideoOutType,MF_MT_FRAME_RATE,dwFps,1);
	MFSetAttributeRatio(pVideoOutType,MF_MT_PIXEL_ASPECT_RATIO,1,1);
	MFSetAttributeSize(pVideoInType,MF_MT_FRAME_SIZE,dwWidth,dwHeight);
	MFSetAttributeRatio(pVideoInType,MF_MT_FRAME_RATE,dwFps,1);
	MFSetAttributeRatio(pVideoInType,MF_MT_PIXEL_ASPECT_RATIO,1,1);
	HRESULT hr = pSinkWriter->AddStream(pVideoOutType,&dwVideoStreamIndex);
	if (SUCCEEDED(hr))
		hr = pSinkWriter->SetInputMediaType(dwVideoStreamIndex,pVideoInType,NULL);
	dwImagePitch = dwWidth * 4;
	dwVideoWidth = dwWidth;
	dwVideoHeight = dwHeight;
	dwVideoFps = dwFps;
	nVideoBitRate = dwBitRate;
	nVideoFrameSize = dwImagePitch * dwHeight;
	return hr;
}

HRESULT CWinMPEG4FileEncoder::InitializeAudioStream(PWAVEFORMATEX pwfx,UINT nAvgBytesPerSec)
{
	if (pSinkWriter == NULL)
		return E_UNEXPECTED;
	MFCreateMediaType(&pAudioInType);
	MFCreateMediaType(&pAudioOutType);
	pAudioInType->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Audio);
	pAudioOutType->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Audio);
	HRESULT hr = MFInitMediaTypeFromWaveFormatEx(pAudioInType,pwfx,sizeof WAVEFORMATEX);
	if (FAILED(hr))
		return hr;
	pAudioOutType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_MP3);
	pAudioOutType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND,nAvgBytesPerSec);
	pAudioOutType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND,pwfx->nSamplesPerSec);
	pAudioOutType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS,pwfx->nChannels >= 2 ? 2:1);
	pAudioOutType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,16);
	pAudioOutType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT,1);
	pAudioOutType->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES,TRUE);
	pAudioOutType->SetUINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX,TRUE);
	pAudioOutType->SetUINT32(MF_MT_COMPRESSED,TRUE);
	hr = pSinkWriter->AddStream(pAudioOutType,&dwAudioStreamIndex);
	if (SUCCEEDED(hr))
		hr = pSinkWriter->SetInputMediaType(dwAudioStreamIndex,pAudioInType,NULL);
	RtlCopyMemory(&wfx,pwfx,sizeof WAVEFORMATEX);
	return hr;
}

HRESULT CWinMPEG4FileEncoder::StartWriteStreams()
{
	return pSinkWriter->BeginWriting();
}

HRESULT CWinMPEG4FileEncoder::WriteVideoSample(IMFMediaBuffer* pVideoSample,MFTIME duration)
{
	if (pVideoSample == NULL)
		return E_POINTER;

	HRESULT hr;
	IMFSample* pSample;
	hr = MFCreateSample(&pSample);
	if (FAILED(hr))
		return hr;
	MFTIME tempDuration = duration;
	if (tempDuration == 0)
		MFFrameRateToAverageTimePerFrame(dwVideoFps,1,(PUINT64)&tempDuration);
	pSample->SetSampleFlags(0);
	pSample->SetSampleTime(videoDuration);
	pSample->SetSampleDuration(tempDuration);
	DWORD dwBufLength = 0;
	pVideoSample->GetMaxLength(&dwBufLength);
	IMFMediaBuffer* pMediaBuffer;
	hr = MFCreateAlignedMemoryBuffer(dwBufLength,MF_4_BYTE_ALIGNMENT,&pMediaBuffer);
	if (SUCCEEDED(hr))
	{
		PBYTE pOldData = NULL,pNewData = NULL;
		pMediaBuffer->SetCurrentLength(dwBufLength);
		pVideoSample->Lock(&pOldData,NULL,NULL);
		if (pOldData)
		{
			pMediaBuffer->Lock(&pNewData,NULL,NULL);
			if (pNewData)
			{
				MFCopyImage(pNewData,dwImagePitch,pOldData,dwImagePitch,dwImagePitch,dwVideoHeight);
				pMediaBuffer->Unlock();
				hr = pSample->AddBuffer(pMediaBuffer);
			}
			pVideoSample->Unlock();
		}else
			hr = MF_E_INSUFFICIENT_BUFFER;
		pMediaBuffer->Release();
	}
	EnterCriticalSection(&cs);
	if (SUCCEEDED(hr))
		hr = pSinkWriter->WriteSample(dwVideoStreamIndex,pSample);
	if (SUCCEEDED(hr))
		videoDuration += tempDuration;
	LeaveCriticalSection(&cs);
	pSample->Release();
	return hr;
}

HRESULT CWinMPEG4FileEncoder::WriteAudioSample(IMFMediaBuffer* pAudioSample,MFTIME duration)
{
	if (pAudioSample == NULL)
		return E_POINTER;
	if (duration == 0)
		return E_INVALIDARG;

	HRESULT hr;
	IMFSample* pSample;
	hr = MFCreateSample(&pSample);
	if (FAILED(hr))
		return hr;
	pSample->SetSampleFlags(0);
	pSample->SetSampleTime(audioDuration);
	pSample->SetSampleDuration(duration);
	DWORD dwBufLength = 0;
	pAudioSample->GetMaxLength(&dwBufLength);
	IMFMediaBuffer* pMediaBuffer;
	hr = MFCreateMemoryBuffer(dwBufLength,&pMediaBuffer);
	if (SUCCEEDED(hr))
	{
		PBYTE pOldData = NULL,pNewData = NULL;
		pMediaBuffer->SetCurrentLength(dwBufLength);
		pAudioSample->Lock(&pOldData,NULL,NULL);
		if (pOldData)
		{
			pMediaBuffer->Lock(&pNewData,NULL,NULL);
			if (pNewData)
			{
				RtlCopyMemory(pNewData,pOldData,dwBufLength);
				pMediaBuffer->Unlock();
				hr = pSample->AddBuffer(pMediaBuffer);
			}
			pAudioSample->Unlock();
		}else
			hr = MF_E_INSUFFICIENT_BUFFER;
		pMediaBuffer->Release();
	}
	EnterCriticalSection(&cs);
	if (SUCCEEDED(hr))
		hr = pSinkWriter->WriteSample(dwAudioStreamIndex,pSample);
	if (SUCCEEDED(hr))
		audioDuration += duration;
	LeaveCriticalSection(&cs);
	pSample->Release();
	return hr;
}

HRESULT CWinMPEG4FileEncoder::ShutdownAndSaveFile()
{
	return pSinkWriter->Finalize();
}