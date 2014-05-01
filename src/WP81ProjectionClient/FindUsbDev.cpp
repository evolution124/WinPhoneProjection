#include <Windows.h>
#include <SetupAPI.h>

typedef struct _FIND_USB_BUS_DEV_INFO{
	GUID guidClass;
	HDEVINFO hDevInfo;
	SP_DEVICE_INTERFACE_DATA devInterface;
	SP_DEVINFO_DATA devInfoData;
	UINT nIndex;
}FIND_USB_BUS_DEV_INFO,*PFIND_USB_BUS_DEV_INFO;

EXTERN_C DECLSPEC_NOINLINE HANDLE WINAPI FindFirstUsbBusDev()
{
	GUID guidUsbBusClass = {};
	CLSIDFromString(L"{31648E59-6FCB-4D81-9C97-D00999B0459A}",&guidUsbBusClass);
	HDEVINFO hDevInfo = SetupDiGetClassDevsW(&guidUsbBusClass,NULL,NULL,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
	if (hDevInfo == NULL)
		return NULL;
	PFIND_USB_BUS_DEV_INFO p = (PFIND_USB_BUS_DEV_INFO)malloc(sizeof FIND_USB_BUS_DEV_INFO);
	RtlZeroMemory(p,sizeof FIND_USB_BUS_DEV_INFO);
	p->hDevInfo = hDevInfo;
	p->guidClass = guidUsbBusClass;
	p->devInterface.cbSize = sizeof SP_DEVICE_INTERFACE_DATA;
	p->devInfoData.cbSize = sizeof SP_DEVINFO_DATA;
	p->nIndex++;
	SetupDiEnumDeviceInterfaces(hDevInfo,NULL,&guidUsbBusClass,0,&p->devInterface);
	SetupDiEnumDeviceInfo(hDevInfo,0,&p->devInfoData);
	return (HANDLE)p;
}

EXTERN_C DECLSPEC_NOINLINE BOOL WINAPI FindNextUsbBusDev(HANDLE h)
{
	PFIND_USB_BUS_DEV_INFO p = (PFIND_USB_BUS_DEV_INFO)h;
	p->devInterface.cbSize = sizeof SP_DEVICE_INTERFACE_DATA;
	p->devInfoData.cbSize = sizeof SP_DEVINFO_DATA;
	if (!SetupDiEnumDeviceInterfaces(p->hDevInfo,NULL,&p->guidClass,p->nIndex,&p->devInterface))
		return FALSE;
	if (!SetupDiEnumDeviceInfo(p->hDevInfo,p->nIndex,&p->devInfoData))
		return FALSE;
	p->nIndex++;
	return TRUE;
}

EXTERN_C DECLSPEC_NOINLINE BOOL WINAPI FindUsbBusGetDevPath(HANDLE h,LPWSTR lpstrBuffer,DWORD cchBuffer)
{
	PFIND_USB_BUS_DEV_INFO p = (PFIND_USB_BUS_DEV_INFO)h;
	if (lpstrBuffer == NULL || cchBuffer == 0)
		return FALSE;
	DWORD dwDataLength = 0;
	SetupDiGetDeviceInterfaceDetailW(p->hDevInfo,&p->devInterface,NULL,0,&dwDataLength,NULL);
	if (dwDataLength == 0)
		return FALSE;
	PSP_DEVICE_INTERFACE_DETAIL_DATA_W pDevData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)GlobalAlloc(GPTR,dwDataLength + sizeof(WCHAR));
	pDevData->cbSize = 6;
	BOOL bResult = SetupDiGetDeviceInterfaceDetailW(p->hDevInfo,&p->devInterface,pDevData,dwDataLength,NULL,NULL);
	if (bResult)
	{
		if (lstrlenW(pDevData->DevicePath) <= cchBuffer)
			lstrcpyW(lpstrBuffer,pDevData->DevicePath);
		else
			lstrcpynW(lpstrBuffer,pDevData->DevicePath,cchBuffer);
	}
	GlobalFree(pDevData);
	return bResult;
}

EXTERN_C DECLSPEC_NOINLINE BOOL WINAPI FindUsbBusGetDisplayName(HANDLE h,LPWSTR lpstrBuffer,DWORD cchBuffer)
{
	PFIND_USB_BUS_DEV_INFO p = (PFIND_USB_BUS_DEV_INFO)h;
	if (lpstrBuffer == NULL || cchBuffer == 0)
		return FALSE;
	DWORD dwDataLength = 0;
	DEVPROPTYPE propDevType;
	DEVPROPKEY propDevKey;
	propDevKey.pid = 14;
	CLSIDFromString(L"{A45C254E-DF1C-4EFD-8020-67D146A850E0}",&propDevKey.fmtid);
	SetupDiGetDevicePropertyW(p->hDevInfo,&p->devInfoData,&propDevKey,&propDevType,NULL,0,&dwDataLength,0);
	if (dwDataLength == 0)
		return FALSE;
	LPWSTR lpszBuffer = (LPWSTR)GlobalAlloc(GPTR,dwDataLength + sizeof(WCHAR));
	BOOL bResult = SetupDiGetDevicePropertyW(p->hDevInfo,&p->devInfoData,&propDevKey,&propDevType,(PBYTE)lpszBuffer,dwDataLength,NULL,0);
	if (bResult)
	{
		if (lstrlenW(lpszBuffer) <= cchBuffer)
			lstrcpyW(lpstrBuffer,lpszBuffer);
		else
			lstrcpynW(lpstrBuffer,lpszBuffer,cchBuffer);
	}
	GlobalFree(lpszBuffer);
	return bResult;
}

EXTERN_C DECLSPEC_NOINLINE VOID WINAPI FindUsbBusClose(HANDLE h)
{
	PFIND_USB_BUS_DEV_INFO p = (PFIND_USB_BUS_DEV_INFO)h;
	SetupDiDestroyDeviceInfoList(p->hDevInfo);
	free(p);
}