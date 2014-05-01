#pragma once

#include "WP81ProjectionCommon.h"

__forceinline BOOL InitWP81UsbPipePolicy(WINUSB_INTERFACE_HANDLE hUsbInterface,PWP_SCRREN_TO_PC_PIPE p)
{
	BOOL bResult = FALSE;
	USB_INTERFACE_DESCRIPTOR usbInterfaceDesc = {};
	WinUsb_QueryInterfaceSettings(hUsbInterface,0,&usbInterfaceDesc);
	if (usbInterfaceDesc.bNumEndpoints == 2) //一个控制，一个读屏幕
	{
		BOOLEAN bOpenState = TRUE;
		WinUsb_QueryPipe(hUsbInterface,0,0,&p->PipeOfData);
		WinUsb_QueryPipe(hUsbInterface,0,1,&p->PipeOfControl);
		if (p->PipeOfControl.PipeType == UsbdPipeTypeBulk && p->PipeOfData.PipeType == UsbdPipeTypeBulk)
		{
			if (WinUsb_SetPipePolicy(hUsbInterface,p->PipeOfControl.PipeId,7,1,&bOpenState) && WinUsb_SetPipePolicy(hUsbInterface,p->PipeOfData.PipeId,7,1,&bOpenState))
				bResult = TRUE;
		}
	}
	return bResult;
}

__forceinline BOOL SendWP81UsbCtlCode(WINUSB_INTERFACE_HANDLE hUsbInterface,UINT code)
{
	WINUSB_SETUP_PACKET usbPacket = {};
	usbPacket.RequestType = 0x21; //magic
	usbPacket.Request = code;
	DWORD dwTemp = 0;
	return WinUsb_ControlTransfer(hUsbInterface,usbPacket,NULL,0,&dwTemp,NULL);
}

typedef enum{
	WP_ProjectionScreenOrientation_Default = 0, //默认（竖）
	WP_ProjectionScreenOrientation_Normal = 1, //竖屏
	WP_ProjectionScreenOrientation_Hori_KeyBack = 2, //横屏（方向向着后退按钮）
	WP_ProjectionScreenOrientation_Hori_KeySearch = 8, //横屏（方向向着搜索按钮）
}WP81ProjectionScreenOrientation,*PWP81ProjectionScreenOrientation; 

class CWP81ProjectionClient : public IUnknown
{
private:
	ULONG RefCount = 1;
	LPWSTR lpstrUsbVid = NULL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hEvent = NULL;
	WINUSB_INTERFACE_HANDLE hUsb = NULL;
	PVOID pBufOfData = NULL;
	DWORD dwBufferSize = 0;
	BOOL bIoPending = FALSE;
	WP_SCRREN_TO_PC_PIPE usbPipe;
	OVERLAPPED ioOverlapped;
	CRITICAL_SECTION cs;
	DWORD dwPhoneWidth = 0,dwPhoneHeight = 0,dwPhoneStride = 0;
public:
	CWP81ProjectionClient(LPCWSTR lpszUsbVid);
	~CWP81ProjectionClient();
public:  //IUnknown
	IFACEMETHODIMP QueryInterface(REFIID iid,void** ppvObject)
	{
		if (ppvObject == NULL)
			return E_POINTER;
		if (iid == IID_IUnknown)
		{
			*ppvObject = static_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}else{
			*ppvObject = NULL;
			return E_NOINTERFACE;
		}
	}

	IFACEMETHODIMP_(ULONG) AddRef()
	{
		return (ULONG)InterlockedIncrement(&RefCount);
	}

	IFACEMETHODIMP_(ULONG) Release()
	{
		ULONG _RefCount = (ULONG)InterlockedDecrement(&RefCount);
		if (_RefCount == 0)
			delete this;
		return _RefCount;
	}
public: //My Methods (Error Query:GetLastError)
	/*
	初始化WP投影仪(手机会出现是否允许投影对话框)
	参数：
		dwMaxBufferSize 最大读数据缓冲区大小
	*/
	virtual BOOL STDMETHODCALLTYPE Initialize(DWORD dwMaxBufferSize = WP_SCREEN_TO_PC_ALIGN512_MAX_SIZE);
	/*
	读取当前的屏幕图像（图像为16bit的BMP数据）
	参数：
		dwBufferSize 缓冲区大小
		pBuffer 数据将被写到这个缓冲区
		pWidth 图像高度
		pHeight 图像宽度
		pdwBits 图像色深
		pOrientation 屏幕方向
	*/
	virtual BOOL STDMETHODCALLTYPE ReadImageAsync();
	/*
	等待异步IO读取完成
	参数：
		dwTimeout 超时时间，默认无限等待
	*/
	virtual BOOL STDMETHODCALLTYPE WaitReadImageComplete(DWORD dwSizeOfBuf,PBYTE pBuffer,PUINT32 pWidth,PUINT32 pHeight,PDWORD pdwBits,PUINT pOrientation,DWORD dwTimeout = INFINITE,BOOL bFastCall = FALSE);
	/*
	获取手机原始显示的分辨率大小
	参数：
		pdwWidth 原始高度
		pdwHeight 原始宽度
	*/
	virtual BOOL STDMETHODCALLTYPE GetPhoneScreenPixel(PDWORD pdwWidth,PDWORD pdwHeight);
	/*
	判断USB接口是否是USB2.0以下的速度
	参数：
		无，返回值为TRUE则是低速
	*/
	virtual BOOL STDMETHODCALLTYPE IsLowSpeedUsbPort(PBOOL pbLowSpeed);
};