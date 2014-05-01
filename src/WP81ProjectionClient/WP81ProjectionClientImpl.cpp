#include "WP81ProjectionClient.h"

//Export Class Impl.
EXTERN_C VOID WINAPI CreateWP81ProjectionClient(LPCWSTR lpstrUsbVid,void** ppv)
{
	if (lpstrUsbVid && ppv)
		*ppv = new CWP81ProjectionClient(lpstrUsbVid);
}

//Export C-Style.
EXTERN_C HANDLE WINAPI InitWinPhoneProjectionClient(LPCWSTR lpstrUsbVid)
{
	if (lpstrUsbVid == NULL)
		return NULL;
	CWP81ProjectionClient* p = new CWP81ProjectionClient(lpstrUsbVid);
	if (p->Initialize())
		return p;
	else
	{
		p->Release();
		return NULL;
	}
}

EXTERN_C VOID WINAPI FreeWinPhoneProjectionClient(HANDLE p)
{
	if (p)
		((CWP81ProjectionClient*)p)->Release();
}

EXTERN_C BOOL WINAPI ReadWinPhoneScreenImageAsync(HANDLE p)
{
	if (p == NULL)
		return FALSE;
	return ((CWP81ProjectionClient*)p)->ReadImageAsync();
}

EXTERN_C BOOL WINAPI WaitWinPhoneScreenImageComplete(HANDLE p,DWORD dwSizeOfBuf,PBYTE pBuffer,PUINT32 pWidth,PUINT32 pHeight,PDWORD pdwBits,PUINT pOrientation,DWORD dwTimeout,BOOL bFastCall)
{
	if (p == NULL)
		return FALSE;
	return ((CWP81ProjectionClient*)p)->WaitReadImageComplete(dwSizeOfBuf,pBuffer,pWidth,pHeight,pdwBits,pOrientation,dwTimeout,bFastCall);
}