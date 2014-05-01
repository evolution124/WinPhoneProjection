#include "stdafx.h"
#include "D2DRenderWindow.h"
#include "AsyncD2DRecorder.h"
#include <gdiplus.h>
#pragma region
#ifdef _X86_
#pragma comment(lib,"..\\Release\\WP81ProjectionClient.lib")
#endif
#ifdef _ARM_
#pragma comment(lib,"..\\ARM\\Release\\WP81ProjectionClient.lib")
#endif
#ifdef _AMD64_
#pragma comment(lib,"..\\x64\\Release\\WP81ProjectionClient.lib")
#endif
#pragma endregion

#define CORE_LIB_FILE_NAME "WP81ProjectionClient.dll"
#define CORE_MF_SUPPORT_COM_LIB_FILE_NAME L"mfWP81RealtimeSource.dll"

#define DEFAULT_CENTER_TEXT L"Click the \"Device\" Menu to Select the Windows Phone to Play."
#define DEFAULT_MAIN_WIDTH 400
#define DEFAULT_MAIN_HEIGHT 666

#define MENU_CMD 100
#define MENU_CMD_EXIT (MENU_CMD + 1)
#define MENU_CMD_ABOUT (MENU_CMD + 2)
#define MENU_CMD_DISPLAY_AUTO (MENU_CMD + 3)
#define MENU_CMD_DISPLAY_NORMAL (MENU_CMD + 4)
#define MENU_CMD_DISPLAY_HORI_LEFT (MENU_CMD + 5)
#define MENU_CMD_DISPLAY_HORI_RIGHT (MENU_CMD + 6)
#define MENU_CMD_CAPTURE_TO_IMAGE_FILE (MENU_CMD + 7)
#define MENU_CMD_CAPTURE_TO_VIDEO_FILE_START (MENU_CMD + 8)
#define MENU_CMD_CAPTURE_TO_VIDEO_FILE_STOP (MENU_CMD + 9)
#define MENU_CMD_CAPTURE_TO_VIDEO_FILE_USE_HARDWARE (MENU_CMD + 10)
#define MENU_CMD_REGISTER_MFSOURCE_TO_WMP (MENU_CMD + 11)
#define MENU_CMD_PHONE_DEVICE_SELECT (MENU_CMD + 1000)

LPWSTR lpszPhoneDevName;
CD2DRenderWindow* RenderWindow;
HWND MainWindow;
WNDPROC RenderWindowProc;
HMENU MainMenu;
HMENU hMenuDevice,hMenuDisplay,hMenuCapture,hMenuHelper;
DWORD dwImageWidth,dwImageHeight;
UINT nDevCount;
LPWSTR* ppstrDevPath;
BOOL bPlayState = TRUE;
BOOL bExitVideoRec = FALSE;
WP81ProjectionScreenOrientation ForceOrientation = WP_ProjectionScreenOrientation_Default,CurrentOrientation = WP_ProjectionScreenOrientation_Default;

DECLSPEC_NOINLINE BOOL IsKeyCLSIDExists(LPCWSTR lpszClsid)
{
	if (lpszClsid == NULL)
		return FALSE;
	BOOL bResult = FALSE;
	HKEY hKey = NULL;
	RegOpenKeyExW(HKEY_CLASSES_ROOT,L"CLSID",0,KEY_READ,&hKey);
	if (hKey)
	{
		HKEY hKey2 = NULL;
		RegOpenKeyExW(hKey,lpszClsid,0,KEY_QUERY_VALUE,&hKey2);
		bResult = hKey2 ? TRUE:FALSE;
		RegCloseKey(hKey2);
		RegCloseKey(hKey);
	}
	return bResult;
}

DECLSPEC_NOINLINE VOID MoveWindowClientArea(HWND hWnd,DWORD dwWidth,DWORD dwHeight)
{
	RECT rc = {};
	GetWindowRect(hWnd,&rc);
	int x = rc.left,y = rc.top;
	MoveWindow(hWnd,x,y,dwWidth,dwHeight,FALSE);
	GetClientRect(hWnd,&rc);
	MoveWindow(hWnd,x,y,dwWidth + (dwWidth - rc.right),dwHeight + (dwHeight - rc.bottom),TRUE);
}

DECLSPEC_NOINLINE VOID MoveWindowClientArea(HWND hWnd,DWORD dwWidth,DWORD dwHeight,int x,int y)
{
	MoveWindow(hWnd,x,y,dwWidth,dwHeight,FALSE);
	RECT rc = {};
	GetClientRect(hWnd,&rc);
	MoveWindow(hWnd,x,y,dwWidth + (dwWidth - rc.right),dwHeight + (dwHeight - rc.bottom),TRUE);
}

DECLSPEC_NOINLINE HBITMAP HBitmapFromHIcon32bpp(HICON hIcon)
{
	if (hIcon == NULL)
		return NULL;
	ICONINFO icoInfo = {};
	if (!GetIconInfo(hIcon,&icoInfo))
		return NULL;
	icoInfo.xHotspot *= 2;
	icoInfo.yHotspot *= 2;
	HDC hDC = GetDC(NULL);
	HDC hMemDC = CreateCompatibleDC(hDC);
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = icoInfo.xHotspot;
	bmi.bmiHeader.biHeight = -icoInfo.yHotspot;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	PVOID pvBitmapBits = NULL;
	HBITMAP hBitmap = CreateDIBSection(hDC,&bmi,DIB_RGB_COLORS,&pvBitmapBits,NULL,0);
	SelectObject(hMemDC,hBitmap);
	DrawIconEx(hMemDC,0,0,hIcon,icoInfo.xHotspot,icoInfo.yHotspot,0,NULL,DI_NORMAL);
	DestroyIcon(hIcon);
	DeleteDC(hMemDC);
	ReleaseDC(NULL,hDC);
	return hBitmap;
}

DECLSPEC_NOINLINE HBITMAP CaptureWindowToHBitmap(HWND hWnd)
{
	RECT rc = {};
	GetWindowRect(hWnd,&rc);
	UINT nWidth = rc.right - rc.left,nHeight = rc.bottom - rc.top;
	HDC hWindowDC = GetWindowDC(hWnd);
	HDC hMemDC = CreateCompatibleDC(hWindowDC);
	HBITMAP hBitmap = CreateCompatibleBitmap(hWindowDC,nWidth,nHeight);
	SelectObject(hMemDC,hBitmap);
	BitBlt(hMemDC,0,0,nWidth,nHeight,hWindowDC,0,0,SRCCOPY);
	DeleteDC(hMemDC);
	ReleaseDC(hWnd,hWindowDC);
	return hBitmap;
}

DECLSPEC_NOINLINE BOOL GetOpenSaveDialogFileName(HWND hWnd,LPCWSTR lpszItemDesc,LPCWSTR lpszItemExt,LPWSTR lpstrBuffer,UINT cchBuffer,BOOL bOpen = TRUE)
{
	if (lpstrBuffer == NULL)
		return FALSE;
	BOOL bResult = FALSE;
	COMDLG_FILTERSPEC cf = {lpszItemDesc,lpszItemExt};
	IFileDialog* pFileDialog;
	if (SUCCEEDED(CoCreateInstance(bOpen ? CLSID_FileOpenDialog:CLSID_FileSaveDialog,NULL,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&pFileDialog))))
	{
		IShellItem* psiResult = NULL;
		ULONG_PTR dwFlags = 0;
		pFileDialog->GetOptions(&dwFlags);
		pFileDialog->SetOptions(dwFlags|FOS_FORCESHOWHIDDEN|FOS_DONTADDTORECENT|FOS_HIDEMRUPLACES|FOS_FORCEFILESYSTEM);
		pFileDialog->SetFileTypes(1,&cf);
		pFileDialog->SetDefaultExtension(PathFindExtensionW(lpszItemExt));
		pFileDialog->Show(hWnd);
		pFileDialog->GetResult(&psiResult);
		if (psiResult)
		{
			PWCH pszFilePath = NULL;
			psiResult->GetDisplayName(SIGDN_FILESYSPATH,&pszFilePath);
			if (pszFilePath)
			{
				StrCpyNW(lpstrBuffer,pszFilePath,cchBuffer);
				CoTaskMemFree(pszFilePath);
				bResult = TRUE;
			}
			psiResult->Release();
		}
		pFileDialog->Release();
	}
	return bResult;
}

LRESULT CALLBACK RenderWindowMessageProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if (uMsg == WM_LBUTTONDBLCLK)
		SendMessageW(GetParent(hWnd),WM_KEYDOWN,VK_SPACE,NULL);
	if (uMsg == WM_PAINT)
	{
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hWnd,&ps);
		HBRUSH hBursh = CreateSolidBrush(0);
		RECT rc;
		GetClientRect(hWnd,&rc);
		FillRect(hDC,&rc,hBursh);
		DeleteObject(hBursh);
		SetBkMode(hDC,TRANSPARENT);
		SetTextColor(hDC,RGB(255,255,255));
		SelectObject(hDC,GetStockObject(DEFAULT_GUI_FONT));
		if (lpszPhoneDevName == NULL)
			TabbedTextOutW(hDC,1,1,DEFAULT_CENTER_TEXT,-1,0,NULL,0);
		EndPaint(hWnd,&ps);
		return DefWindowProcW(hWnd,uMsg,wParam,lParam);
	}
	return CallWindowProcW(RenderWindowProc,hWnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK MainWindowMessageProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static ULONG64 ulTickCountOK = 0;
	RECT rc;
	switch (uMsg)
	{
	case WM_CLOSE:
	case WM_DESTROY:
		if (pMP4Encoder)
			SendMessageW(hWnd,WM_COMMAND,MENU_CMD_CAPTURE_TO_VIDEO_FILE_STOP,NULL);
		PostQuitMessage(0);
		break;
	case WM_INITDIALOG:
		if (lParam == NULL)
		{
			WCHAR szLibFileName[MAX_PATH];
			GetFullPathNameW(CORE_MF_SUPPORT_COM_LIB_FILE_NAME,MAX_PATH,szLibFileName,NULL);
			if (!PathFileExistsW(szLibFileName))
				DeleteMenu(hMenuHelper,0,MF_BYPOSITION),
				DeleteMenu(hMenuHelper,0,MF_BYPOSITION);
			return NULL;
		}
		break;
	case WM_GETMINMAXINFO:
		PMINMAXINFO pmmInfo;
		if (lParam)
		{
			pmmInfo = (PMINMAXINFO)lParam;
			pmmInfo->ptMinTrackSize.x = 250;
			pmmInfo->ptMinTrackSize.y = 300;
		}
		break;
	case WM_WINDOWPOSCHANGING:
	case WM_WINDOWPOSCHANGED:
		GetClientRect(hWnd,&rc);
		RenderWindow->MoveWindow(0,0,rc.right,rc.bottom);
		break;
	case WM_KEYDOWN:
		if (wParam == VK_SPACE)
		{
			if (bPlayState)
			{
				bPlayState = FALSE;
				RenderWindow->Pause();
			}else{
				 bPlayState = TRUE;
				 RenderWindow->Resume();
			}
		}
		break;
	case WM_COMMAND:
		switch (wParam)
		{
		case MENU_CMD_EXIT:
			SendMessageW(hWnd,WM_CLOSE,NULL,NULL);
			break;
		case MENU_CMD_ABOUT:
			ShellMessageBoxA(GetModuleHandle(NULL),hWnd,
				"Author:ShanYe [K.F Yang]\nMail:chinatb361@hotmail.com\nThanks:SiYuan Lin.\nSource Code:GitHub.com/amamiya/WinPhoneProjection\n\nThis Project Implement by Reverse Engineering. (Project My Screen App - 8.10.12349)","About - rev.1",MB_OK);
			break;
		case MENU_CMD_DISPLAY_AUTO:
			CheckMenuRadioItem(hMenuDisplay,0,3,0,MF_BYPOSITION);
			ForceOrientation = WP81ProjectionScreenOrientation::WP_ProjectionScreenOrientation_Default;
			break;
		case MENU_CMD_DISPLAY_NORMAL:
			CheckMenuRadioItem(hMenuDisplay,0,3,1,MF_BYPOSITION);
			ForceOrientation = WP81ProjectionScreenOrientation::WP_ProjectionScreenOrientation_Normal;
			MoveWindowClientArea(hWnd,dwImageWidth,dwImageHeight);
			RenderWindow->MoveWindow(0,0,dwImageWidth,dwImageHeight);
			RenderWindow->ForceChangeOrientation(WP_ProjectionScreenOrientation_Normal);
			break;
		case MENU_CMD_DISPLAY_HORI_LEFT:
			CheckMenuRadioItem(hMenuDisplay,0,3,2,MF_BYPOSITION);
			ForceOrientation = WP81ProjectionScreenOrientation::WP_ProjectionScreenOrientation_Hori_KeyBack;
			MoveWindowClientArea(hWnd,dwImageHeight,dwImageWidth);
			RenderWindow->MoveWindow(0,0,dwImageHeight,dwImageWidth);
			RenderWindow->ForceChangeOrientation(WP_ProjectionScreenOrientation_Hori_KeyBack);
			break;
		case MENU_CMD_DISPLAY_HORI_RIGHT:
			CheckMenuRadioItem(hMenuDisplay,0,3,3,MF_BYPOSITION);
			ForceOrientation = WP81ProjectionScreenOrientation::WP_ProjectionScreenOrientation_Hori_KeySearch;
			MoveWindowClientArea(hWnd,dwImageHeight,dwImageWidth);
			RenderWindow->MoveWindow(0,0,dwImageHeight,dwImageWidth);
			RenderWindow->ForceChangeOrientation(WP_ProjectionScreenOrientation_Hori_KeySearch);
			break;
		case MENU_CMD_CAPTURE_TO_IMAGE_FILE:
			WCHAR szImageFile[MAX_PATH];
			if (GetOpenSaveDialogFileName(hWnd,L"JPEG File",L"*.jpg",szImageFile,ARRAYSIZE(szImageFile),FALSE))
			{
				SetFileAttributesW(szImageFile,FILE_ATTRIBUTE_NORMAL);
				DeleteFileW(szImageFile);
				HBITMAP hRenderBitmap = CaptureWindowToHBitmap(RenderWindow->operator HWND());
				if (hRenderBitmap)
				{
					ULONG ulGdipToken = NULL;
					Gdiplus::GdiplusStartupInput gdipInit = {};
					gdipInit.GdiplusVersion = 1;
					Gdiplus::GdiplusStartup(&ulGdipToken,&gdipInit,NULL);
					Gdiplus::GpBitmap* pGdipBitmap = NULL;
					Gdiplus::DllExports::GdipCreateBitmapFromHBITMAP(hRenderBitmap,NULL,&pGdipBitmap);
					if (pGdipBitmap)
					{
						BYTE jpgQuality = 100;
						Gdiplus::EncoderParameters encoderParameters;
						encoderParameters.Count = 1;
						encoderParameters.Parameter[0].NumberOfValues = 1;
						encoderParameters.Parameter[0].Type = 4;
						encoderParameters.Parameter[0].Value = &jpgQuality;
						CLSIDFromString(L"{1D5BE4B5-FA4A-452D-9CDD-5DB35105E7EB}",&encoderParameters.Parameter[0].Guid);
						CLSID encoderGuid;
						CLSIDFromString(L"{557CF401-1A04-11D3-9A73-0000F81EF32E}",&encoderGuid);
						Gdiplus::DllExports::GdipSaveImageToFile(pGdipBitmap,szImageFile,&encoderGuid,&encoderParameters);
						Gdiplus::DllExports::GdipDisposeImage(pGdipBitmap);
					}
					Gdiplus::GdiplusShutdown(ulGdipToken);
					DeleteObject(hRenderBitmap);
				}
			}
			break;
		case MENU_CMD_CAPTURE_TO_VIDEO_FILE_START:
			WCHAR szVideoFile[MAX_PATH];
			if (GetOpenSaveDialogFileName(hWnd,L"MPEG4 File",L"*.mp4",szVideoFile,ARRAYSIZE(szVideoFile),FALSE))
			{
				SetFileAttributesW(szVideoFile,FILE_ATTRIBUTE_NORMAL);
				DeleteFileW(szVideoFile);
				pMP4Encoder = new CWinMPEG4FileEncoder(szVideoFile);
				HRESULT hr = pMP4Encoder->InitializeSinkWriter(GetMenuState(hMenuCapture,MENU_CMD_CAPTURE_TO_VIDEO_FILE_USE_HARDWARE,MF_BYCOMMAND)
					== MF_CHECKED ? TRUE:FALSE);
				if (SUCCEEDED(hr))
				{
					if (CurrentOrientation == WP_ProjectionScreenOrientation_Default || CurrentOrientation == WP_ProjectionScreenOrientation_Normal)
					{
						dwRecImgWidth = dwImageWidth;
						dwRecImgHeight = dwImageHeight;
					}else{
						dwRecImgWidth = dwImageHeight;
						dwRecImgHeight = dwImageWidth;
					}
					hr = pMP4Encoder->InitializeVideoStream(dwRecImgWidth,dwRecImgHeight,25,dwRecImgWidth * 4 * dwRecImgHeight * 2);
					if (SUCCEEDED(hr))
						hr = pMP4Encoder->StartWriteStreams();
				}
				if (FAILED(hr))
				{
					wsprintfW(szVideoFile,L"Load the MPEG-4 Encoder Error Code:0x%08X",hr);
					MessageBoxW(hWnd,szVideoFile,NULL,MB_ICONERROR);
					delete pMP4Encoder;
				}else{
					CoCreateInstance(CLSID_WICImagingFactory1,NULL,CLSCTX_ALL,IID_PPV_ARGS(&pWICFactory));
					hEventRender = CreateEventW(NULL,FALSE,FALSE,NULL);
					pSharedBitmapQueue = new IComInterfaceQueue<IWICBitmapSource>;
					hThreadVideoRecorder = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)&QueueWriteVideoSampleThreadProc,&bExitVideoRec,0,NULL);
					hThreadVideoRecorder2 = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)&QueueWriteVideoSampleThreadProc,&bExitVideoRec,0,NULL);
					SetThreadPriority(hThreadVideoRecorder,THREAD_PRIORITY_HIGHEST);
					SetThreadPriority(hThreadVideoRecorder2,THREAD_PRIORITY_BELOW_NORMAL);
					RenderWindow->SetInformationRecorder(25,hEventRender,pSharedBitmapQueue);
					EnableMenuItem(hMenuCapture,MENU_CMD_CAPTURE_TO_VIDEO_FILE_START,MF_GRAYED);
					EnableMenuItem(hMenuCapture,MENU_CMD_CAPTURE_TO_VIDEO_FILE_STOP,MF_ENABLED);
					EnableMenuItem(hMenuCapture,MENU_CMD_CAPTURE_TO_VIDEO_FILE_USE_HARDWARE,MF_GRAYED);
				}
			}
			break;
		case MENU_CMD_CAPTURE_TO_VIDEO_FILE_STOP:
			ITaskbarList3* pTaskbarList3;
			CoCreateInstance(CLSID_TaskbarList,NULL,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&pTaskbarList3));
			pTaskbarList3->HrInit();
			pTaskbarList3->SetProgressState(hWnd,TBPF_INDETERMINATE);
			RenderWindow->ClearRecorder();
			_FOREVER_LOOP{
				SetEvent(hEventRender);
				if (pSharedBitmapQueue->empty())
					break;
				SwitchToThread();
			}
			Sleep(100);
			bExitVideoRec = TRUE;
			_FOREVER_LOOP{
				SetEvent(hEventRender);
				DWORD ExitCode0,ExitCode1;
				GetExitCodeThread(hThreadVideoRecorder,&ExitCode0);
				GetExitCodeThread(hThreadVideoRecorder2,&ExitCode1);
				if (ExitCode0 != STILL_ACTIVE && ExitCode1 != STILL_ACTIVE)
					break;
				SwitchToThread();
			}
			pMP4Encoder->ShutdownAndSaveFile();
			CloseHandle(hEventRender);
			CloseHandle(hThreadVideoRecorder);
			CloseHandle(hThreadVideoRecorder2);
			pSharedBitmapQueue->Release();
			pWICFactory->Release();
			SAFE_DELETE(pMP4Encoder);
			pTaskbarList3->SetProgressState(hWnd,TBPF_NOPROGRESS);
			pTaskbarList3->Release();
			EnableMenuItem(hMenuCapture,MENU_CMD_CAPTURE_TO_VIDEO_FILE_START,MF_ENABLED);
			EnableMenuItem(hMenuCapture,MENU_CMD_CAPTURE_TO_VIDEO_FILE_STOP,MF_GRAYED);
			EnableMenuItem(hMenuCapture,MENU_CMD_CAPTURE_TO_VIDEO_FILE_USE_HARDWARE,MF_ENABLED);
			bExitVideoRec = FALSE;
			break;
		case MENU_CMD_CAPTURE_TO_VIDEO_FILE_USE_HARDWARE:
			CheckMenuItem(hMenuCapture,MENU_CMD_CAPTURE_TO_VIDEO_FILE_USE_HARDWARE,
				GetMenuState(hMenuCapture,MENU_CMD_CAPTURE_TO_VIDEO_FILE_USE_HARDWARE,MF_BYCOMMAND)
				== MF_CHECKED ? MF_UNCHECKED:MF_CHECKED);
			break;
		case MENU_CMD_REGISTER_MFSOURCE_TO_WMP:
			WCHAR szDllFilePath[MAX_PATH];
			GetModuleFileNameW(NULL,szDllFilePath,ARRAYSIZE(szDllFilePath));
			PathRemoveFileSpecW(szDllFilePath);
			PathAppendW(szDllFilePath,CORE_MF_SUPPORT_COM_LIB_FILE_NAME);
			if (PathFileExistsW(szDllFilePath))
			{
				PathQuoteSpacesW(szDllFilePath);
				SHELLEXECUTEINFOW exeInfo = {};
				exeInfo.cbSize = sizeof(exeInfo);
				exeInfo.nShow = SW_SHOW;
				exeInfo.fMask = SEE_MASK_UNICODE;
				exeInfo.hwnd = hWnd;
				exeInfo.lpFile = L"regsvr32.exe";
				exeInfo.lpParameters = szDllFilePath;
				exeInfo.lpVerb = L"runas";
				ShellExecuteExW(&exeInfo);
			}
			break;
		}
		if (LOWORD(wParam) >= MENU_CMD_PHONE_DEVICE_SELECT)
		{
			UINT nIndex = LOWORD(wParam) - MENU_CMD_PHONE_DEVICE_SELECT;
			WCHAR szDevPath[MAX_PATH] = {},szDevName[MAX_PATH] = {};
			lstrcpyW(szDevPath,ppstrDevPath[nIndex]);
			GetMenuStringW(hMenuDevice,nIndex,szDevName,ARRAYSIZE(szDevName),MF_BYPOSITION);
			for (UINT i = 0;i < nDevCount;i++)
			{
				DeleteMenu(hMenuDevice,i,MF_BYPOSITION);
				GlobalFree(ppstrDevPath[i]);
			}
			GlobalFree(ppstrDevPath);
			DeleteMenu(hMenuDevice,0,MF_BYPOSITION);
			if (RenderWindow->Initialize(szDevPath))
			{
				lpszPhoneDevName = StrDupW(szDevName);
				wsprintfW(szDevPath,L"%s - WP8.1 Projection Client",szDevName);
				SetWindowTextW(hWnd,szDevPath);
			}else{
				wsprintfW(szDevPath,L"Unrecoverable Exception:Need to quit. (0x%08X)",RenderWindow->operator HRESULT());
				MessageBoxW(hWnd,szDevPath,NULL,MB_ICONERROR);
				PostMessageW(hWnd,WM_CLOSE,NULL,NULL);
			}
		}
		break;
	case WM_TIMER:
		if (lpszPhoneDevName)
		{
			WCHAR szBuffer[MAX_PATH] = {};
			WCHAR szTimeStr[MAX_PATH] = {};
			StrFromTimeIntervalW(szTimeStr,ARRAYSIZE(szTimeStr),GetTickCount64() - ulTickCountOK,3);
			wsprintfW(szBuffer,L"%s - WP8.1 Projection Client [%s - FPS:%d]",lpszPhoneDevName,szTimeStr,RenderWindow->GetFps());
			SetWindowTextW(hWnd,szBuffer);
		}
		break;
	case WM_WP81_PC_HANGUP:
		KillTimer(hWnd,0);
		ShellMessageBoxA(GetModuleHandle(NULL),hWnd,"Device has been disconnected!","Warning",MB_ICONERROR);
		PostMessageW(hWnd,WM_CLOSE,NULL,NULL);
		break;
	case WM_WP81_PC_SHOW:
		OutputDebugStringA("Show Screen Projection Image.");
		EnableMenuItem(MainMenu,1,MF_BYPOSITION|MF_ENABLED);
		EnableMenuItem(MainMenu,2,MF_BYPOSITION|MF_ENABLED);
		DrawMenuBar(hWnd);
		SetTimer(hWnd,0,1000,NULL);
		ulTickCountOK = GetTickCount64();
		dwImageWidth = wParam;
		dwImageHeight = lParam;
		if (IsKeyCLSIDExists(L"{4BE8D3C0-0515-4A37-AD55-E4BAE19AF471}")) //Intel QuickSync Supported.
			PostMessageW(hWnd,WM_COMMAND,MENU_CMD_CAPTURE_TO_VIDEO_FILE_USE_HARDWARE,NULL);
		break;
	case WM_WP81_PC_ORI_CHANGED:
		CurrentOrientation = (WP81ProjectionScreenOrientation)wParam;
		if (ForceOrientation == WP_ProjectionScreenOrientation_Default && dwImageWidth > 0 && dwImageHeight > 0)
		{
			switch (wParam)
			{
			case WP_ProjectionScreenOrientation_Default:
			case WP_ProjectionScreenOrientation_Normal:
				MoveWindowClientArea(hWnd,dwImageWidth,dwImageHeight);
				RenderWindow->MoveWindow(0,0,dwImageWidth,dwImageHeight);
				RenderWindow->ForceChangeOrientation(WP_ProjectionScreenOrientation_Normal);
				break;
			case WP_ProjectionScreenOrientation_Hori_KeyBack:
			case WP_ProjectionScreenOrientation_Hori_KeySearch:
				MoveWindowClientArea(hWnd,dwImageHeight,dwImageWidth);
				RenderWindow->MoveWindow(0,0,dwImageHeight,dwImageWidth);
				RenderWindow->ForceChangeOrientation((WP81ProjectionScreenOrientation)wParam);
				break;
			}
		}
		break;
	}
	return DefWindowProcW(hWnd,uMsg,wParam,lParam);
}

void true_main(int nCmdShow)
{
	timeBeginPeriod(1);
	WCHAR szBuffer[MAX_PATH] = {};
	SHGetSpecialFolderPathW(NULL,szBuffer,CSIDL_PROGRAM_FILESX86,FALSE);
	PathAppendW(szBuffer,L"Windows Media Player\\setup_wm.exe");
	HICON hPhoneIcon = (HICON)LoadImage(GetModuleHandleA(CORE_LIB_FILE_NAME),MAKEINTRESOURCE(100),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
	HBITMAP hPhoneBitmap = HBitmapFromHIcon32bpp(hPhoneIcon);
	HICON hWMPIcon = (HICON)LoadImage(LoadLibraryExW(szBuffer,NULL,LOAD_LIBRARY_AS_IMAGE_RESOURCE),MAKEINTRESOURCE(70),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
	HBITMAP hWMPBitmap = HBitmapFromHIcon32bpp(hWMPIcon);

	MainMenu = CreateMenu();
	hMenuDevice = CreateMenu();
	hMenuDisplay = CreateMenu();
	hMenuCapture = CreateMenu();
	hMenuHelper = CreateMenu();

	ppstrDevPath = (LPWSTR*)GlobalAlloc(GPTR,32 * sizeof(LPWSTR)); //max 32 items.
	RtlZeroMemory(szBuffer,ARRAYSIZE(szBuffer));
	HANDLE hFindUsb = FindFirstUsbBusDev();
	if (hFindUsb)
	{
		do{
			if (FindUsbBusGetDevPath(hFindUsb,szBuffer,ARRAYSIZE(szBuffer)))
			{
				if (!FindUsbBusGetDisplayName(hFindUsb,szBuffer,ARRAYSIZE(szBuffer)))
				{
					if (nDevCount == 0)
						lstrcpyW(szBuffer,L"Windows Phone");
					else
						wsprintfW(szBuffer,L"Windows Phone %d",nDevCount + 1);
				}
				LPWSTR lpstrDevPath = (LPWSTR)GlobalAlloc(GPTR,MAX_PATH * 2);
				FindUsbBusGetDevPath(hFindUsb,lpstrDevPath,MAX_PATH);
				ppstrDevPath[nDevCount] = lpstrDevPath;
				AppendMenuW(hMenuDevice,MF_BYCOMMAND,MENU_CMD_PHONE_DEVICE_SELECT + nDevCount,szBuffer);
				if (hPhoneBitmap)
					SetMenuItemBitmaps(hMenuDevice,MENU_CMD_PHONE_DEVICE_SELECT + nDevCount,MF_BYCOMMAND,hPhoneBitmap,hPhoneBitmap);
				nDevCount++;
				if (nDevCount > 30)
					break;
			}
		}while (FindNextUsbBusDev(hFindUsb));
		FindUsbBusClose(hFindUsb);

		if (nDevCount == 1) //one phone.
			SetMenuDefaultItem(hMenuDevice,0,MF_BYPOSITION);
		else if (nDevCount == 0)
			AppendMenuA(hMenuDevice,MF_BYCOMMAND|MF_GRAYED,0,"Device Not Detected.");
	}

	AppendMenuA(hMenuDevice,MF_BYPOSITION,0,NULL);
	AppendMenuA(hMenuDevice,MF_BYCOMMAND,MENU_CMD_EXIT,"Exit");
	AppendMenuA(hMenuDisplay,MF_BYCOMMAND,MENU_CMD_DISPLAY_AUTO,"Orientation: Auto");
	AppendMenuA(hMenuDisplay,MF_BYCOMMAND,MENU_CMD_DISPLAY_NORMAL,"Orientation: Force to portrait up.");
	AppendMenuA(hMenuDisplay,MF_BYCOMMAND,MENU_CMD_DISPLAY_HORI_LEFT,"Orientation: Force to landscape left.");
	AppendMenuA(hMenuDisplay,MF_BYCOMMAND,MENU_CMD_DISPLAY_HORI_RIGHT,"Orientation: Force to landscape right.");
	AppendMenuA(hMenuCapture,MF_BYCOMMAND,MENU_CMD_CAPTURE_TO_IMAGE_FILE,"Save Screen to Image File...");
	AppendMenuA(hMenuCapture,MF_BYCOMMAND,0,NULL);
	AppendMenuA(hMenuCapture,MF_BYCOMMAND,MENU_CMD_CAPTURE_TO_VIDEO_FILE_START,"Start Capture Screen to Video File...");
	AppendMenuA(hMenuCapture,MF_BYCOMMAND|MF_GRAYED,MENU_CMD_CAPTURE_TO_VIDEO_FILE_STOP,"Stop Capture Screen to Video File");
	AppendMenuA(hMenuCapture,MF_BYCOMMAND,0,NULL);
	AppendMenuA(hMenuCapture,MF_BYCOMMAND,MENU_CMD_CAPTURE_TO_VIDEO_FILE_USE_HARDWARE,"Use Hardware H.264 Encoder to Encode Video");
	AppendMenuA(hMenuHelper,MF_BYCOMMAND,MENU_CMD_REGISTER_MFSOURCE_TO_WMP,"Install to Microsoft Media Foundation (for Windows Media Player)");
	AppendMenuA(hMenuHelper,MF_BYCOMMAND,0,NULL);
	AppendMenuA(hMenuHelper,MF_BYCOMMAND,MENU_CMD_ABOUT,"About...");
	AppendMenuA(MainMenu,MF_POPUP,(UINT_PTR)hMenuDevice,"Device");
	AppendMenuA(MainMenu,MF_POPUP,(UINT_PTR)hMenuDisplay,"Display");
	AppendMenuA(MainMenu,MF_POPUP,(UINT_PTR)hMenuCapture,"Capture");
	AppendMenuA(MainMenu,MF_POPUP,(UINT_PTR)hMenuHelper,"Help");
	SetMenuItemBitmaps(hMenuHelper,MENU_CMD_REGISTER_MFSOURCE_TO_WMP,MF_BYCOMMAND,hWMPBitmap,hWMPBitmap);
	SetMenuDefaultItem(hMenuHelper,MENU_CMD_ABOUT,MF_BYCOMMAND);
	CheckMenuRadioItem(hMenuDisplay,0,3,0,MF_BYPOSITION);
	EnableMenuItem(MainMenu,1,MF_BYPOSITION|MF_GRAYED);
	EnableMenuItem(MainMenu,2,MF_BYPOSITION|MF_GRAYED);

	HMODULE hModuleIcon = LoadLibraryExA("explorer.exe",NULL,LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	HICON hIcon = LoadIcon(hModuleIcon,MAKEINTRESOURCE(261));
	FreeLibrary(hModuleIcon);

	MainWindow = CreateWindowExW(WS_EX_APPWINDOW,L"#32770",L"WP8.1 Projection Client - 1.0 [ShanYe]",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,0,0,NULL,MainMenu,GetModuleHandle(NULL),NULL);
	RenderWindow = new CD2DRenderWindow(MainWindow);
	RenderWindowProc = RenderWindow->SetNewWndProc(&RenderWindowMessageProc);
	SetWindowLongPtrW(MainWindow,GWLP_WNDPROC,(LONG_PTR)&MainWindowMessageProc);
	MoveWindowClientArea(MainWindow,DEFAULT_MAIN_WIDTH,DEFAULT_MAIN_HEIGHT);
	SendMessageW(MainWindow,WM_SETICON,ICON_SMALL,(LPARAM)hIcon);
	SendMessageW(MainWindow,WM_INITDIALOG,NULL,NULL);
	ShowWindow(MainWindow,nCmdShow);
	DestroyIcon(hIcon);

	MSG msg;
	while (GetMessageW(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	delete RenderWindow;
	timeEndPeriod(1);
}

int APIENTRY wWinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPWSTR lpCmdLine,int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	CoInitialize(NULL);
	MFStartup(MF_VERSION);
	true_main(nCmdShow);
	MFShutdown();
	CoUninitialize();
	return NULL;
}