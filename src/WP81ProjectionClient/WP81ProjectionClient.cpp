#include "WP81ProjectionClient.h"
#include <mfapi.h>

//构造函数
CWP81ProjectionClient::CWP81ProjectionClient(LPCWSTR lpszUsbVid)
{
	lpstrUsbVid = StrDupW(lpszUsbVid);
	hFile = CreateFileW(lpstrUsbVid,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,NULL);
	hEvent = CreateEventExW(NULL,NULL,CREATE_EVENT_MANUAL_RESET,EVENT_ALL_ACCESS);
	InitializeCriticalSectionEx(&cs,0,CRITICAL_SECTION_NO_DEBUG_INFO);
	RtlZeroMemory(&usbPipe,sizeof(usbPipe));
	RtlZeroMemory(&ioOverlapped,sizeof(ioOverlapped));
	ioOverlapped.hEvent = hEvent;
}

//析构函数
CWP81ProjectionClient::~CWP81ProjectionClient()
{
	if (hUsb)
		WinUsb_Free(hUsb);
	if (pBufOfData)
		GlobalFree(pBufOfData);
	if (lpstrUsbVid)
		CoTaskMemFree(lpstrUsbVid);
	DeleteCriticalSection(&cs);
	SetEvent(hEvent);
	CloseHandle(hEvent);
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
}

BOOL STDMETHODCALLTYPE CWP81ProjectionClient::Initialize(DWORD dwMaxBufferSize)
{
	if (pBufOfData)
		return FALSE;
	if (dwMaxBufferSize == 0)
		return FALSE;
	if (!WinUsb_Initialize(hFile,&hUsb))
		return FALSE;
	if (!InitWP81UsbPipePolicy(hUsb,&usbPipe))
		return FALSE;
	if (!SendWP81UsbCtlCode(hUsb,WP_SCREEN_TO_PC_CTL_CODE_NOTIFY)) //WP8.1手机会显示投影通知
		return FALSE;
	DWORD dwTemp;
	WinUsb_ResetPipe(hUsb,usbPipe.PipeOfControl.PipeId);
	WinUsb_ResetPipe(hUsb,usbPipe.PipeOfData.PipeId);
	WinUsb_GetOverlappedResult(hUsb,&ioOverlapped,&dwTemp,TRUE);
	ResetEvent(hEvent);
	pBufOfData = GlobalAlloc(GPTR,dwMaxBufferSize);
	dwBufferSize = dwMaxBufferSize;
	return TRUE;
}

BOOL STDMETHODCALLTYPE CWP81ProjectionClient::ReadImageAsync()
{
	if (hUsb == NULL)
		return FALSE;

	BOOL bResult = FALSE;
	EnterCriticalSection(&cs);
	if (bIoPending) //正在ReadPipe中。。。
	{
		if (WinUsb_AbortPipe(hUsb,usbPipe.PipeOfData.PipeId))
		{
			DWORD dwTemp;
			WinUsb_GetOverlappedResult(hUsb,&ioOverlapped,&dwTemp,TRUE);
			ResetEvent(hEvent);
		}
	}
	if (SendWP81UsbCtlCode(hUsb,WP_SCREEN_TO_PC_CTL_CODE_READY))
	{
		WinUsb_ReadPipe(hUsb,usbPipe.PipeOfData.PipeId,(PUCHAR)pBufOfData,dwBufferSize,NULL,&ioOverlapped);
		if (GetLastError() == ERROR_IO_PENDING)
			bIoPending = bResult = TRUE;
	}
	LeaveCriticalSection(&cs);
	return bResult;
}

BOOL STDMETHODCALLTYPE CWP81ProjectionClient::WaitReadImageComplete(DWORD dwSizeOfBuf,PBYTE pBuffer,PUINT32 pWidth,PUINT32 pHeight,PDWORD pdwBits,PUINT pOrientation,DWORD dwTimeout,BOOL bFastCall)
{
	if (!bFastCall)
	{
		if (dwSizeOfBuf == 0 || pBuffer == NULL)
			return FALSE;
		if (dwSizeOfBuf > dwBufferSize)
			return FALSE;
		if (hUsb == NULL)
			return FALSE;
	}

	BOOL bResult = FALSE;
	if (!bFastCall)
		EnterCriticalSection(&cs);
	if (WaitForSingleObjectEx(hEvent,dwTimeout,FALSE) == WAIT_OBJECT_0)
	{
		ResetEvent(hEvent);
		bIoPending = FALSE;
		PWP_SCRREN_TO_PC_DATA pData = (PWP_SCRREN_TO_PC_DATA)pBufOfData;
		if (pData->dwFourcc == WP_SCREEN_TO_PC_FLAG_FOURCC && pData->dwFlags == 2 && pData->nImageDataLength > 0)
		{
			if (pWidth)
				*pWidth = pData->wDisplayWidth;
			if (pHeight)
				*pHeight = pData->wDisplayHeight;
			if (pOrientation)
				*pOrientation = pData->wScrOrientation;
			if (pdwBits)
				*pdwBits = pData->dwImageBits;
			if (dwPhoneStride == 0)
			{
				dwPhoneWidth = pData->wRawWidth;
				dwPhoneHeight = pData->wRawHeight;
				dwPhoneStride = dwPhoneWidth * 4;
			}

			DWORD dwBitSize = 1;
			if (pData->dwImageBits == 16)
				dwBitSize = 2;
			else if (pData->dwImageBits == 24)
				dwBitSize = 3;
			else if (pData->dwImageBits == 32)
				dwBitSize = 4;
			DWORD dwStride = pData->wDisplayWidth * dwBitSize;
			PBYTE p = ((PBYTE)pData) + WP_SCREEN_TO_PC_IMAGE_DATA_OFFSET;
			if (pBuffer)
				MFCopyImage(pBuffer,dwStride,p,dwStride,dwStride,pData->wDisplayHeight);
				//RtlCopyMemory(pBuffer,p,dwSizeOfBuf);
			bResult = TRUE;
		}
	}
	if (!bFastCall)
		LeaveCriticalSection(&cs);
	return bResult;
}

BOOL STDMETHODCALLTYPE CWP81ProjectionClient::GetPhoneScreenPixel(PDWORD pdwWidth,PDWORD pdwHeight)
{
	if (hUsb == NULL)
		return FALSE;
	if (pdwWidth)
		*pdwWidth = dwPhoneWidth;
	if (pdwHeight)
		*pdwHeight = dwPhoneHeight;
	return TRUE;
}

BOOL STDMETHODCALLTYPE CWP81ProjectionClient::IsLowSpeedUsbPort(PBOOL pbLowSpeed)
{
	if (hUsb == NULL || pbLowSpeed == NULL)
		return FALSE;
	UCHAR UsbSpeed = 0;
	ULONG Len = sizeof UCHAR;
	if (!WinUsb_QueryDeviceInformation(hUsb,DEVICE_SPEED,&Len,&UsbSpeed))
		return FALSE;
	*pbLowSpeed = UsbSpeed == LowSpeed ? TRUE:FALSE;
	return TRUE;
}