#ifndef __STDAFX_H_
#define __STDAFX_H_

#pragma once
#pragma message("stdafx Header...")

#define NULL_PTR nullptr

#define OPEN_FLAG(f1,f2) (f1 | f2)
#define CLEAR_FLAG(f1,f2) (f1 ^ f2)
#define OPEN_FLAG_R(f1,f2) (f1 = (f1 | f2))
#define CLEAR_FLAG_R(f1,f2) (f1 = (f1 ^ f2))

#define DLL_EXPORT __declspec(dllexport)
#define DLL_IMPORT __declspec(dllimport)

#define FASTCALL __fastcall
#define STDCALL _stdcall
#define CDECL _cdecl

#define FORCEINLINE __forceinline
#define INLINE __inline

#define _FOREVER_LOOP while(1)
#define _FOR_EACH(v,p,n,i) for (v = p,i = 0;i < n;v++,++i)

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#define _CALC_WSTR_ALLOC_LEN(l) (l * sizeof(wchar_t) + sizeof(wchar_t))

#include <stdio.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <xmllite.h>
#include <mmsyscom.h>
#include <PropIdl.h>
#include <wincodec.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mferror.h>
#include <mfplay.h>
#include <mfmp2dlna.h>
#include <mfmediaengine.h>
#include <mfmediacapture.h>
#include <mfcaptureengine.h>
#include <mfsharingengine.h>
#include <mfobjects.h>
#include <mftransform.h>
#include <mfreadwrite.h>
#include <wmcodecdsp.h>
#include <wmcontainer.h>
#include <codecapi.h>
#include <d2d1.h>
#include <dwrite.h>
#include <avrt.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <functiondiscoverykeys_devpkey.h>

#define _WIN32_CUR_PROCESS (HANDLE)-1
#define _WIN32_CUR_THREAD (HANDLE)-2

#define _HeapAlloc(dwFlags,dwBytes) HeapAlloc(GetProcessHeap(),(dwFlags),(dwBytes))
#define _HeapSize(lpMem) HeapSize(GetProcessHeap(),0,(lpMem))
#define _HeapFree(lpMem) HeapFree(GetProcessHeap(),0,(lpMem))

#define SAFE_DELETE(OBJ_PTR) if ((OBJ_PTR) != NULL) {delete (OBJ_PTR);(OBJ_PTR) = NULL;}
#define SAFE_RELEASE(IUNK_PTR) if ((IUNK_PTR) != NULL) {(IUNK_PTR)->Release();(IUNK_PTR) = NULL;}

#ifdef _WIN_ZERO_ALLOC_
	#define ZeroAlloc(s) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,s)
	#define ZeroRealloc(p,s) HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,s)
	#define ZeroFree(p) HeapFree(GetProcessHeap(),0,p)
#else
	#define ZeroAlloc(s) calloc(1,s)
	#define ZeroRealloc(p,s) realloc(p,s)
	#define ZeroFree(p) free(p)
#endif

#endif