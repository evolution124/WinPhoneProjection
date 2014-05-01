#pragma once

#include "stdafx.h"

BOOL ConvertWICBitmapFormat(IWICImagingFactory* pWICFactory,REFWICPixelFormatGUID toFormat,IWICBitmapSource* pSrc,IWICBitmapSource** ppDst);
BOOL SyncReadWP16BitScreenImageWithWICBitmap(HANDLE hProjectionClient,PBYTE pBuffer,PUINT pOrientation,IWICImagingFactory* pWICFactory,IWICBitmap** ppWICBitmap,BOOL bReleaseOldWICBitmap);