#pragma once

#include "stdafx.h"
#include "D2DRenderWindowHelp.h"
#include "InterfaceQueueList.h"
#include "..\WP81ProjectionClient\WP81ProjectionClientImpl.h"

#define WM_WP81_PC_SHOW (WM_USER + 1)
#define WM_WP81_PC_HANGUP (WM_USER + 2)
#define WM_WP81_PC_ORI_CHANGED (WM_USER + 3)

class CD2DRenderWindow
{
protected:
	HRESULT lastHR = S_OK;
	HWND hRender = NULL;
	HANDLE hThreadReader = NULL,hThreadRender = NULL;
	HANDLE hProjectionClient = NULL;
	HANDLE hSyncEvent = NULL;
	BOOL bPaused = FALSE;
	CRITICAL_SECTION cs;
private:
	PBYTE pBufOfRaw = NULL,pBufOfConverted = NULL;
	IWICBitmap* pRawBitmap = NULL;
	ID2D1Bitmap* pBltBitmap = NULL;
private:
	IWICImagingFactory* pWICFactory = NULL;
	ID2D1Factory* pD2DFactory = NULL;
	ID2D1HwndRenderTarget* pRender = NULL;
private:
	UINT32 nDrawFps = 0,nDrawFrames = 0;
	DWORD dwDrawTime = 0;
	MFTIME DrawTime100ns = 0;
private:
	UINT nWidth = 0,nHeight = 0;
	DWORD dwImageStride = 0;
	DWORD dwImageRawBits = 0; //no used.
	DWORD dwImageRawStride = 0; //no used.
	UINT Orientation = WP_ProjectionScreenOrientation_Default,PrevOrientation = WP_ProjectionScreenOrientation_Default;
private:
	IComInterfaceQueue<IWICBitmapSource>* pNotifySharedQueue = NULL;
	HANDLE hEventRender = NULL;
	DWORD dwFps = 0;
	MFTIME FpsDelayIn100ns = 0;
public:
	CD2DRenderWindow(HWND hWnd);
	~CD2DRenderWindow();
public:
	operator HWND() const;
	operator HRESULT() const;
	BOOL Initialize(LPWSTR lpstrUsbVid);
	BOOL Pause();
	BOOL Resume();
	BOOL MoveWindow(int x,int y,DWORD dwWidth,DWORD dwHeight);
	BOOL ForceChangeOrientation(WP81ProjectionScreenOrientation ori);
	WNDPROC SetNewWndProc(WNDPROC newproc);
	UINT32 GetFps();
public:
	VOID SetInformationRecorder(DWORD dwFps,HANDLE hEvent,IComInterfaceQueue<IWICBitmapSource>* pSharedQueue);
	VOID ClearRecorder();
protected:
	virtual LRESULT WndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	virtual UINT ReaderThread();
	virtual UINT RenderThread();
protected:
	static LRESULT CALLBACK _StaticWndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static VOID CALLBACK _StaticReaderThread(PVOID p);
	static VOID CALLBACK _StaticRenderThread(PVOID p);
};