#include "stdafx.h"
#include "AsyncD2DRecorder.h"

IComInterfaceQueue<IWICBitmapSource>* pSharedBitmapQueue;
CWinMPEG4FileEncoder* pMP4Encoder;
IWICImagingFactory* pWICFactory;
HANDLE hEventRender;
HANDLE hThreadVideoRecorder,hThreadVideoRecorder2;
DWORD dwRecImgWidth,dwRecImgHeight;

VOID CALLBACK QueueWriteVideoSampleThreadProc(PBOOL pbNeedToExit)
{
	_FOREVER_LOOP{
		if (WaitForSingleObject(hEventRender,INFINITE) == WAIT_OBJECT_0)
		{
			IWICBitmapSource* pRenderBitmap = NULL;
			_FOREVER_LOOP{
				pRenderBitmap = pSharedBitmapQueue->PopIUnknownSafe();
				if (pRenderBitmap == NULL)
					break;
				DWORD dwImageSize = dwRecImgWidth * 4 * dwRecImgHeight;
				IMFMediaBuffer* pBuffer = NULL;
				if (SUCCEEDED(MFCreateMemoryBuffer(dwImageSize,&pBuffer)))
				{
					PBYTE pData = NULL;
					pBuffer->Lock(&pData,NULL,NULL);
					if (pData)
					{
						IWICBitmapFlipRotator *pRenderRotator = NULL,*pRenderRotator2 = NULL;
						pWICFactory->CreateBitmapFlipRotator(&pRenderRotator);
						pWICFactory->CreateBitmapFlipRotator(&pRenderRotator2);
						pRenderRotator->Initialize(pRenderBitmap,dwRecImgWidth < dwRecImgHeight ? WICBitmapTransformFlipVertical:WICBitmapTransformRotate270);
						SAFE_RELEASE(pRenderBitmap);
						if (dwRecImgWidth < dwRecImgHeight)
						{
							pRenderRotator->CopyPixels(NULL,dwRecImgWidth * 4,dwImageSize,pData);
						}else{
							pRenderRotator2->Initialize(pRenderRotator,WICBitmapTransformFlipHorizontal);
							pRenderRotator2->CopyPixels(NULL,dwRecImgWidth * 4,dwImageSize,pData);
						}
						pBuffer->Unlock();
						pRenderRotator2->Release();
						pRenderRotator->Release();
						pMP4Encoder->WriteVideoSample(pBuffer,0);
					}
					pBuffer->Release();
				}
				SAFE_RELEASE(pRenderBitmap);
			}
		}
		if (pbNeedToExit)
				if (*pbNeedToExit)
					break;
	}
}