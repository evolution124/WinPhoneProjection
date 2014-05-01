#pragma comment(lib,"..\\Release\\WP81ProjectionClient.lib")

#include "stdafx.h"
#include "..\WP81ProjectionClient\WP81ProjectionCommon.h"
#include "..\WP81ProjectionClient\WP81ProjectionClientImpl.h"

void main()
{
	CoInitialize(NULL);
	IWICImagingFactory* pWICFactory = NULL;
	CoCreateInstance(CLSID_WICImagingFactory1,NULL,CLSCTX_ALL,IID_PPV_ARGS(&pWICFactory));

	PBYTE p = (PBYTE)malloc(PROJECTION_CLIENT_MAX_IMAGE_BUF_SIZE); //RAW图像数据
	HANDLE hUsbBusDev = FindFirstUsbBusDev(); //查找USB设备（WP手机）
	if (hUsbBusDev) //如果存在USB设备
	{
		WCHAR szDevPath[MAX_PATH] = {};
		FindUsbBusGetDevPath(hUsbBusDev,szDevPath,ARRAYSIZE(szDevPath)); //取得第一个USB设备路径，可Next查找下个
		FindUsbBusClose(hUsbBusDev); //关闭查找
		if (wcslen(szDevPath) > 0)
		{
			HANDLE h = InitWinPhoneProjectionClient(szDevPath); //初始化投影（WP手机会出现通知）
			if (h)
			{
				_FOREVER_LOOP{
					if (!ReadWinPhoneScreenImageAsync(h)) //发送读请求
						break;
					UINT nWidth = 0,nHeight = 0,nOrientation = WP_SCR_ORI_DEFAULT;
					DWORD dwImageBits = 0;
					if (!WaitWinPhoneScreenImageComplete(h,PROJECTION_CLIENT_MAX_IMAGE_BUF_SIZE,p,&nWidth,&nHeight,&dwImageBits,&nOrientation)) //等待数据读取完成
						break;
					CHAR szBuffer[MAX_PATH] = {};
					wsprintfA(szBuffer,"Image Accept:%d x %d,%d Bits,Orientation:%d",nWidth,nHeight,dwImageBits,nOrientation);
					OutputDebugStringA(szBuffer);
					SYSTEMTIME st;
					GetLocalTime(&st);
					printf("%d:%d:%d:%d %s\n",st.wHour,st.wMinute,st.wSecond,st.wMilliseconds,szBuffer);
					/*
					IWICBitmap* pBitmap = NULL;
					pWICFactory->CreateBitmapFromMemory(nWidth,nHeight,GUID_WICPixelFormat16bppBGR565,nWidth * 2,nWidth * 2 * nHeight,p,&pBitmap);
					IWICFormatConverter* pConverter = NULL;
					pWICFactory->CreateFormatConverter(&pConverter);
					pConverter->Initialize(pBitmap,GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,NULL,.0f,WICBitmapPaletteTypeCustom);
					IWICBitmapEncoder* pWICBitmapEncoder = NULL;
					IWICBitmapFrameEncode* pWICBitmapFrameEncode = NULL;
					CoCreateInstance(CLSID_WICPngEncoder,NULL,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&pWICBitmapEncoder));
					DWORD dwBitmapMemSize = nWidth * 4 * nHeight;
					IStream* pStream = NULL;
					PVOID pvPngMemStream = GlobalAlloc(GPTR,dwBitmapMemSize);
					CreateStreamOnHGlobal(pvPngMemStream,TRUE,&pStream);
					pWICBitmapEncoder->Initialize(pStream,WICBitmapEncoderNoCache);
					pWICBitmapEncoder->CreateNewFrame(&pWICBitmapFrameEncode,NULL);
					GUID guidPixelFormat = GUID_WICPixelFormat32bppBGRA;
					pWICBitmapFrameEncode->Initialize(NULL);
					pWICBitmapFrameEncode->SetSize(nWidth,nHeight);
					pWICBitmapFrameEncode->SetPixelFormat(&guidPixelFormat);
					pWICBitmapFrameEncode->WriteSource(pConverter,NULL);
					pWICBitmapFrameEncode->Commit();
					pWICBitmapEncoder->Commit();
					pStream->Release();
					pWICBitmapFrameEncode->Release();
					pWICBitmapEncoder->Release();
					pConverter->Release();
					pBitmap->Release();
					*/
				}
				FreeWinPhoneProjectionClient(h); //释放投影客户端
			}
		}
	}
	pWICFactory->Release();
	free(p);
	CoUninitialize();
	ExitProcess(0);
}