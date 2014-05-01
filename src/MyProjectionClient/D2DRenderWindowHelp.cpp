#include "stdafx.h"
#include "D2DRenderWindowHelp.h"
#include "..\WP81ProjectionClient\WP81ProjectionClientImpl.h"

//etc. WICConvertBitmapSource
BOOL ConvertWICBitmapFormat(IWICImagingFactory* pWICFactory,REFWICPixelFormatGUID toFormat,IWICBitmapSource* pSrc,IWICBitmapSource** ppDst)
{
	IWICFormatConverter* pWICFormatConverter;
	if (FAILED(pWICFactory->CreateFormatConverter(&pWICFormatConverter)))
		return FALSE;
	if (SUCCEEDED(pWICFormatConverter->Initialize(pSrc,toFormat,WICBitmapDitherTypeNone,NULL,.0f,WICBitmapPaletteTypeCustom)))
	{
		*ppDst = pWICFormatConverter;
		return TRUE;
	}
	return FALSE;
}

BOOL SyncReadWP16BitScreenImageWithWICBitmap(HANDLE hProjectionClient,PBYTE pBuffer,PUINT pOrientation,IWICImagingFactory* pWICFactory,IWICBitmap** ppWICBitmap,BOOL bReleaseOldWICBitmap)
{
	if (ReadWinPhoneScreenImageAsync(hProjectionClient))
	{
		UINT nWidth = 0,nHeight = 0;
		DWORD dwImageBits;
		if (WaitWinPhoneScreenImageComplete(hProjectionClient,PROJECTION_CLIENT_MAX_IMAGE_BUF_SIZE,pBuffer,&nWidth,&nHeight,&dwImageBits,pOrientation,INFINITE,TRUE))
		{
			DWORD dwStride = nWidth * 2;
			if (dwImageBits == 16)
			{
				if (*ppWICBitmap && bReleaseOldWICBitmap)
				{
					(*ppWICBitmap)->Release();
					*ppWICBitmap = NULL;
				}
				pWICFactory->CreateBitmapFromMemory(nWidth,nHeight,GUID_WICPixelFormat16bppBGR565,dwStride,dwStride * nHeight,pBuffer,ppWICBitmap);
			}
		}
		return TRUE;
	}
	return FALSE;
}