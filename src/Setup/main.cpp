#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>

#include "resource.h"
#include "registry.h"

#include <algorithm>

#pragma comment(lib,"shlwapi.lib")

#define VIRTUALDUB_EXE L"VirtualDub2.exe"
#define VDREMOTE_DLL   "vdremote.dll"
#define VDSRVLNK_DLL   L"vdsvrlnk.dll"


HWND g_hwnd;
HINSTANCE g_hInst; // current instance
wchar_t szAppName[] = L"VirtualDub Setup Class"; // The name of this application

wchar_t g_szSystemPath[MAX_PATH];
wchar_t g_szProgPath[MAX_PATH];
wchar_t g_szTempPath[MAX_PATH];

typedef BOOL WINAPI fntype_Wow64DisableWow64FsRedirection(PVOID *OldValue);
typedef BOOL WINAPI fntype_Wow64RevertWow64FsRedirection(PVOID OldValue);
fntype_Wow64DisableWow64FsRedirection* pfnWow64DisableWow64FsRedirection = (fntype_Wow64DisableWow64FsRedirection*)GetProcAddress(GetModuleHandleA("kernel32.dll"), "Wow64DisableWow64FsRedirection");
fntype_Wow64RevertWow64FsRedirection* pfnWow64RevertWow64FsRedirection = (fntype_Wow64RevertWow64FsRedirection*)GetProcAddress(GetModuleHandleA("kernel32.dll"), "Wow64RevertWow64FsRedirection");

typedef WINADVAPI LSTATUS APIENTRY fntype_RegDeleteKeyExA(HKEY hKey, LPCSTR lpSubKey, REGSAM samDesired, DWORD Reserved);
fntype_RegDeleteKeyExA* pfnRegDeleteKeyExA = (fntype_RegDeleteKeyExA*)GetProcAddress(GetModuleHandleA("advapi32.dll"), "RegDeleteKeyExA");

///////////////////

BOOL Init(HINSTANCE, int);
LRESULT APIENTRY WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

BOOL APIENTRY InstallAVIFileDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY UninstallAVIFileDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY RemoveSettingsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY AboutDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

///////////

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	wchar_t* lpszFilePart;

	///////////

	if (SHGetFolderPathW(0, CSIDL_SYSTEM, 0, 0, g_szSystemPath) != S_OK) {
		return FALSE;
	}
	if (!GetModuleFileNameW(NULL, g_szTempPath, std::size(g_szTempPath))) {
		return FALSE;
	}
	if (!GetFullPathNameW(g_szTempPath, std::size(g_szProgPath), g_szProgPath, &lpszFilePart)) {
		return FALSE;
	}
	*lpszFilePart = 0;

	if (!Init(hInstance, nCmdShow)) {
		return FALSE;
	}

	// Main message loop.

	while (GetMessageW(&msg, NULL, 0, 0)) {
		//if (!IsDialogMessage(g_hwnd, &msg))
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return 0;
}

////////////////

BOOL Init(HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASSW wc;

	// Register the window class for my window.
	wc.style         = 0;                // Class style.
	wc.lpfnWndProc   = (WNDPROC)WndProc; // Window procedure for this class.
	wc.cbClsExtra    = 0;                // No per-class extra data.
	wc.cbWndExtra    = DLGWINDOWEXTRA;   // No per-window extra data.
	wc.hInstance     = hInstance;        // Application that owns the class.
	wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_VIRTUALDUB));
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpszMenuName  = NULL;             // Name of menu resource in .RC file. 
	wc.lpszClassName = szAppName;        // Name used in call to CreateWindow.

	if (!RegisterClassW(&wc)) {
		return FALSE;
	}

	g_hInst = hInstance; // Store instance handle in our global variable

	g_hwnd = CreateDialogW(hInstance, MAKEINTRESOURCEW(IDD_MAINWINDOW), NULL, (DLGPROC)NULL);

	if (!g_hwnd) {
		return (FALSE);
	}

#ifdef _WIN64
	SetWindowTextW(g_hwnd, L"VirtualDub2 x64 auxiliary setup");
#else
	SetWindowTextW(g_hwnd, L"VirtualDub2 x86 auxiliary setup");
#endif

	ShowWindow(g_hwnd, nCmdShow);
	UpdateWindow(g_hwnd);

	return (TRUE);
}

void PrintfWindowText(HWND hWnd, wchar_t *format, ...) {
	wchar_t buf[256];
	va_list val;

	va_start(val, format);
	vswprintf_s(buf, format, val);
	va_end(val);
	SetWindowTextW(hWnd, buf);
}

bool FindVirtualDub(wchar_t* buf)
{
	if (!GetModuleFileNameW(g_hInst, buf, MAX_PATH)) {
		return false;
	}
	if (!PathRemoveFileSpecW(buf)) {
		return false;
	}
	wcscat(buf, L"\\" VIRTUALDUB_EXE);
	if (GetFileAttributesW(buf) != INVALID_FILE_ATTRIBUTES) {
		return true;
	}
	if (!PathRemoveFileSpecW(buf)) {
		return false;
	}
	if (!PathRemoveFileSpecW(buf)) {
		return false;
	}
	wcscat(buf, L"\\" VIRTUALDUB_EXE);
	if (GetFileAttributesW(buf) != INVALID_FILE_ATTRIBUTES) {
		return true;
	}
	return false;
}

LRESULT APIENTRY WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) { 
		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
			case IDC_EXECUTE:
			{
				wchar_t vdpath[MAX_PATH];
				if (!FindVirtualDub(vdpath) || (INT_PTR)ShellExecuteW(hWnd, L"open", vdpath, NULL, NULL, SW_SHOWNORMAL) <= 32) {
					MessageBoxW(hWnd, L"Couldn't launch " VIRTUALDUB_EXE ".", L"Oops", MB_OK);
				}
				break;
			}
			case IDC_INSTALL:
				DialogBoxW(g_hInst, MAKEINTRESOURCEW(IDD_ATTEMPT), hWnd, (DLGPROC)InstallAVIFileDlgProc);
				break;
			case IDC_UNINSTALL:
				DialogBoxW(g_hInst, MAKEINTRESOURCEW(IDD_ATTEMPT), hWnd, (DLGPROC)UninstallAVIFileDlgProc);
				break;
			case IDC_REMOVE:
				DialogBoxW(g_hInst, MAKEINTRESOURCEW(IDD_ATTEMPT), hWnd, (DLGPROC)RemoveSettingsDlgProc);
				break;
			case IDC_ABOUT:
				DialogBoxW(g_hInst, MAKEINTRESOURCEW(IDD_ABOUT), hWnd, (DLGPROC)AboutDlgProc);
				break;
			case IDCANCEL:
				DestroyWindow(hWnd);
				break;
			}
			break;

		default:
			return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return (0);
}

//////////////////////////////////////////////////////////////////////

void ListboxAddf(HWND hwndListbox, const wchar_t *format, ...) {
	wchar_t buf[256];
	va_list val;

	va_start(val, format);
	vswprintf_s(buf, format, val);
	va_end(val);
	buf[std::size(buf) - 1] = 0;

	SendMessageW(hwndListbox, LB_ADDSTRING, 0, (LPARAM)buf);
}

BOOL InstallFile(const wchar_t* szSource, const wchar_t* szDestFormat, ...) {
	wchar_t szDest[MAX_PATH];
	wchar_t szDestPath[MAX_PATH];
	wchar_t szDestFile[MAX_PATH];
	wchar_t* lpszDestFile;
	wchar_t szCurInst[MAX_PATH];
	wchar_t szTempFile[MAX_PATH];
	DWORD dwFlags = VIFF_DONTDELETEOLD;
	DWORD dwRet;
	UINT uTmpLen;
	wchar_t szFailure[256];
	va_list val;

	va_start(val, szDestFormat);
	vswprintf_s(szDest, szDestFormat, val);
	va_end(val);

	if (!GetFullPathNameW(szDest, std::size(szDestPath), szDestPath, &lpszDestFile)) {
		return FALSE;
	}

	wcscpy(szDestFile, lpszDestFile);
	*lpszDestFile=0;

	do {
		szTempFile[0]=0;
		szCurInst[0]=0;
		uTmpLen = std::size(szTempFile);
		dwRet = VerInstallFileW(dwFlags, szSource, szDestFile, g_szProgPath, szDestPath, szCurInst, szTempFile, &uTmpLen);

		if (dwRet & VIF_TEMPFILE) {
			DeleteFileW(szTempFile);
			dwRet &= ~VIF_TEMPFILE;
		}

		szFailure[0]=0;

		if (dwRet & (VIF_MISMATCH | VIF_DIFFTYPE)) {
			swprintf_s(szFailure,
				L"The old %s doesn't look like a VirtualDub file.\n"
				"If it belongs to another application, installing the new file may cause "
				"the other app to stop functioning.\n"
				"Install the new file only if you are sure or have a backup."
				, szDestFile);
		}
		else if (dwRet & VIF_SRCOLD) {
			swprintf_s(szFailure,
				L"%s is older than the %s being installed over.\n"
				"You should install the older %s if you do not use other versions "
				"of VirtualDub, since the newer file may be incompatible."
				, szSource, szDestFile, szSource);
		}
		else if (dwRet & VIF_WRITEPROT) {
			swprintf_s(szFailure,
				L"The %s being installed over has been marked read-only.\n"
				"Override read-only attribute and install anyway?"
				, szDestFile);
		}
		else if (dwRet & VIF_FILEINUSE) {
			swprintf_s(szFailure,
				L"%s is in use.  It cannot be installed over right now.\n"
				"If you have any copies of VirtualDub or any programs that "
				"may be using VirtualDub's AVIFile handler, please close them "
				"and then click OK to retry the operation."
				, szDestFile);
		}
		else if (dwRet & VIF_OUTOFSPACE) {
			swprintf_s(szFailure,
				L"Doh! We're out of space trying to write:\n\t%s\n\nCan you clear some up?"
				, szDest);
		}
		else if (dwRet & VIF_ACCESSVIOLATION) {
			swprintf_s(szFailure,
				L"Access violation.  Check with your administrator to see if you have "
				"the appropriate permissions to write to \"%s\"."
				, szDest);
		}
		else if (dwRet & VIF_SHARINGVIOLATION) {
			swprintf_s(szFailure,
				L"Share violation; something else probably has %s open.  Try closing applications that "
				"have the file open, and check permissions on network drives."
				, szDestFile);
		}
		else if (dwRet & VIF_CANNOTCREATE) {
			swprintf_s(szFailure, L"Couldn't create temporary file %s.\nTry again?", szTempFile);
		}
		else if (dwRet & VIF_CANNOTDELETE) {
			swprintf_s(szFailure, L"Couldn't delete temporary file %s.\nTry installing again?", szTempFile);
		}
		else if (dwRet & VIF_CANNOTDELETECUR) {
			swprintf_s(szFailure, L"Couldn't delete existing file \"%s\".\nTry installing again?", szDest);
		}
		else if (dwRet & VIF_CANNOTRENAME) {
			swprintf_s(szFailure,
				L"Deleted old file %s, but couldn't move %s into its place.\n"
				"You should retry this operation.",
				szDestFile, szSource);
		}
		else if (dwRet & VIF_CANNOTREADSRC) {
			swprintf_s(szFailure,
				L"Couldn't read source file \"%s\".  Should I try again?"
				, szSource);
		}
		else if (dwRet & VIF_CANNOTREADDST) {
			swprintf_s(szFailure,
				L"Couldn't read destination file \"%s\".  I can try installing over it anyway, though."
				, szDest);
		}
		else if (dwRet & VIF_OUTOFMEMORY) {
			swprintf_s(szFailure, L"Ran out of memory!  Try freeing some up.");
		}
		else if (dwRet) {
			swprintf_s(szFailure,
				L"Unidentified error copying:\n\t%s\n\t\tto\n\t%s\n\nTry forcing install?"
				, szSource, szDest);
		}

		if (szFailure[0]) {
			if (IDNO == MessageBoxW(NULL, szFailure, L"Install error", MB_YESNO | MB_APPLMODAL)) {
				return FALSE;
			}
		}

		dwFlags |= VIFF_FORCEINSTALL;
	} while(dwRet);

	return TRUE;
}

BOOL InstallRegStr(HKEY hkBase, const char *szKeyName, const char *szName, const char *szValue) {
	char buf[256];

	if (!SetRegString(hkBase, szKeyName, szName, szValue)) {
		sprintf(buf,"Couldn't set registry key %s\\%s",szKeyName,szName?szName:"(default)");
		MessageBoxA(NULL, buf, "Install error", MB_OK);
		return FALSE;
	}

	return TRUE;
}

BOOL InstallRegStr64(HKEY hkBase, const char *szKeyName, const char *szName, const char *szValue) {
	char buf[256];

	if (!SetRegString64(hkBase, szKeyName, szName, szValue)) {
		sprintf(buf,"Couldn't set x64 registry key %s\\%s",szKeyName,szName?szName:"(default)");
		MessageBoxA(NULL, buf, "Install error", MB_OK);
		return FALSE;
	}

	return TRUE;
}

BOOL InstallDeleteKey(HKEY key, const char *name) {
	LSTATUS r = RegDeleteKeyA(key,name);
	if (r==ERROR_FILE_NOT_FOUND) return TRUE;
	return r==ERROR_SUCCESS;
}

BOOL InstallDeleteKey64(HKEY key, const char *name) {
	LSTATUS r = pfnRegDeleteKeyExA(key,name,KEY_WOW64_64KEY,0);
	if (r==ERROR_FILE_NOT_FOUND) return TRUE;
	return r==ERROR_SUCCESS;
}

BOOL InstallDeleteFile(const wchar_t *szFileFormat, ...) {
	wchar_t szFile[MAX_PATH+50];
	va_list val;

	va_start(val, szFileFormat);
	vswprintf_s(szFile, szFileFormat, val);
	va_end(val);

	if (!DeleteFileW(szFile)) {
		if (GetLastError() != ERROR_FILE_NOT_FOUND) {
			return FALSE;
		}
	}

	return TRUE;
}

///////////////////////////////////////

BOOL APIENTRY InstallAVIFileDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL fSuccess;

	switch(msg) {
		case WM_INITDIALOG:
			{
				HWND hwndListbox = GetDlgItem(hDlg, IDC_ACTIONLIST);

				SetWindowTextW(hDlg, L"Install AVIFile frameclient");

				ListboxAddf(hwndListbox, L"Copy " VDREMOTE_DLL " to %s\\" VDREMOTE_DLL, g_szSystemPath);
				ListboxAddf(hwndListbox, L"Copy " VDSRVLNK_DLL " to %s\\" VDSRVLNK_DLL, g_szSystemPath);

				ListboxAddf(hwndListbox, L"Add VDRemote class and AVIFile entries to Registry");
			}
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
			case IDOK:

				fSuccess = true;

				fSuccess &= InstallFile(_CRT_WIDE(VDREMOTE_DLL), L"%s\\" VDREMOTE_DLL, g_szSystemPath);
				fSuccess &= InstallFile(VDSRVLNK_DLL, L"%s\\" VDSRVLNK_DLL, g_szSystemPath);

				fSuccess &= InstallRegStr(HKEY_CLASSES_ROOT,"CLSID\\{894288e0-0948-11d2-8109-004845000eb5}",NULL,"VirtualDub link handler");
				fSuccess &= InstallRegStr(HKEY_CLASSES_ROOT,"CLSID\\{894288e0-0948-11d2-8109-004845000eb5}\\InprocServer32",NULL, VDREMOTE_DLL);
				fSuccess &= InstallRegStr(HKEY_CLASSES_ROOT,"CLSID\\{894288e0-0948-11d2-8109-004845000eb5}\\InprocServer32","ThreadingModel","Apartment");
				fSuccess &= InstallRegStr(HKEY_CLASSES_ROOT,"CLSID\\{894288e0-0948-11d2-8109-004845000eb5}\\InprocServer32\\AVIFile",NULL,"1");
				fSuccess &= InstallRegStr(HKEY_CLASSES_ROOT,"AVIFile\\Extensions\\VDR",NULL,"{894288e0-0948-11d2-8109-004845000eb5}");
				fSuccess &= InstallRegStr(HKEY_CLASSES_ROOT,"AVIFile\\RIFFHandlers\\VDRM",NULL,"{894288e0-0948-11d2-8109-004845000eb5}");

				if (fSuccess) {
					MessageBoxA(hDlg, "AVIFile frameclient install successful.", "VirtualDub Setup", MB_OK);
				} else {
					MessageBoxA(hDlg, "AVIFile frameclient install failed.", "VirtualDub Setup", MB_OK);
				}

				EndDialog(hDlg, TRUE);
				return TRUE;
			case IDCANCEL:
				EndDialog(hDlg, FALSE);
				return TRUE;
			}
	}
	return FALSE;
}

BOOL APIENTRY UninstallAVIFileDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL fSuccess;
	BOOL fRegSuccess;

	switch(msg) {
		case WM_INITDIALOG:
			{
				HWND hwndListbox = GetDlgItem(hDlg, IDC_ACTIONLIST);

				SetWindowTextW(hDlg, L"Uninstall AVIFile frameclient");

				ListboxAddf(hwndListbox, L"Delete %s\\" VDREMOTE_DLL, g_szSystemPath);
				ListboxAddf(hwndListbox, L"Delete %s\\" VDSRVLNK_DLL, g_szSystemPath);
				ListboxAddf(hwndListbox, L"Remove VDRemote class and AVIFile entries from Registry");
			}
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
			case IDOK:
				fSuccess = true;

				fSuccess &= InstallDeleteFile(L"%s\\" VDREMOTE_DLL, g_szSystemPath);
				fSuccess &= InstallDeleteFile(L"%s\\" VDSRVLNK_DLL, g_szSystemPath);

				fRegSuccess = true;

				fRegSuccess &= InstallDeleteKey(HKEY_CLASSES_ROOT,"Clsid\\{894288e0-0948-11d2-8109-004845000eb5}\\InprocServer32\\AVIFile");
				fRegSuccess &= InstallDeleteKey(HKEY_CLASSES_ROOT,"Clsid\\{894288e0-0948-11d2-8109-004845000eb5}\\InprocServer32");
				fRegSuccess &= InstallDeleteKey(HKEY_CLASSES_ROOT,"Clsid\\{894288e0-0948-11d2-8109-004845000eb5}");
				fRegSuccess &= InstallDeleteKey(HKEY_CLASSES_ROOT,"AVIFile\\Extensions\\VDR");
				fRegSuccess &= InstallDeleteKey(HKEY_CLASSES_ROOT,"AVIFile\\RIFFHandlers\\VDRM");

				if (!fRegSuccess)
					MessageBoxA(hDlg, "Registry entries were in use.  Deinstall not successful.\n"
									"\n"
									"A partial installation now exists on your system.  Reinstall the AVIFile "
									"handler to restore frameclient functionality, or close applications that may "
									"be occupying the Registry entries and retry the deinstall."
									,"VirtualDub Setup",MB_OK);

				else if (!fSuccess)
					MessageBoxA(hDlg, "DLL files were in use.  Deinstall not successful.\n"
									"\n"
									"A partial installation now exists on your system.  Reinstall the AVIFile "
									"handler to restore frameclient functionality, or close applications that may "
									"be occupying the shared DLLs and retry the deinstall."
									,"VirtualDub Setup",MB_OK);
				else
					MessageBoxA(hDlg, "AVIFile frameclient deinstall successful.", "VirtualDub Setup", MB_OK);

				EndDialog(hDlg, TRUE);
				return TRUE;
			case IDCANCEL:
				EndDialog(hDlg, FALSE);
				return TRUE;
			}
	}
	return FALSE;
}

///////////////////////////////////////////////

void RemoveVirtualDubKeys(HWND hwndListbox)
{
	FILETIME ftModified;
	int i;
	LONG err;
	wchar_t szKeyName[MAX_PATH];
	wchar_t szErrorText[128];
	DWORD dwKeyNameLen;

	SendMessageW(hwndListbox, LB_RESETCONTENT, 0, 0);

	i=0;
	for(;;) {
		dwKeyNameLen = std::size(szKeyName);
		err = RegEnumKeyExW(HKEY_USERS, i++, szKeyName, &dwKeyNameLen, 0, NULL, 0, &ftModified);

		if (err == ERROR_NO_MORE_ITEMS) {
			break;
		}
		else if (err == ERROR_SUCCESS) {
			HKEY hkeyUser;
			wchar_t* bp = szKeyName + wcslen(szKeyName);
			
			err = RegOpenKeyExW(HKEY_USERS, szKeyName, 0, KEY_ALL_ACCESS, &hkeyUser);
			if (err != ERROR_SUCCESS) {
				FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, szErrorText, std::size(szErrorText), NULL);
				ListboxAddf(hwndListbox, L"HKEY_USERS\\%s: %s", szKeyName, szErrorText);
				continue;
			}

			wcscpy(bp, L"\\Software\\VirtualDub2");

			err = SHDeleteKeyW(HKEY_USERS, szKeyName);
			if (err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND) {
				FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, szErrorText, std::size(szErrorText), NULL);
			} else {
				wcscpy(szErrorText, L"Deleted");
			}
			ListboxAddf(hwndListbox, L"HKEY_USERS\\%s: %s", szKeyName, szErrorText);

			RegCloseKey(hkeyUser);
		}
	}
}

BOOL APIENTRY RemoveSettingsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_INITDIALOG:
			{
				HWND hwndListbox = GetDlgItem(hDlg, IDC_ACTIONLIST);

				SetWindowTextW(hDlg, L"Remove VirtualDub preference data");

				ListboxAddf(hwndListbox, L"Remove HKEY_USERS\\*\\Software\\VirtualDub2\\*");
			}
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
			case IDOK:
				SetWindowTextW(GetDlgItem(hDlg, IDC_ACTION), L"Results:");

				RemoveVirtualDubKeys(GetDlgItem(hDlg, IDC_ACTIONLIST));

				SetWindowTextW(GetDlgItem(hDlg, IDOK), L"Retry");
				SetWindowTextW(GetDlgItem(hDlg, IDCANCEL), L"Done");
				return TRUE;
			case IDCANCEL:
				EndDialog(hDlg, FALSE);
				return TRUE;
			}
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////
//
//	About...
//
///////////////////////////////////////////////////////////////////////////

BOOL APIENTRY AboutDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_INITDIALOG:
			SetDlgItemTextW(hDlg, IDC_FINALS_SUCK,
#ifdef _DEBUG
				L"Debug build"
#else
				L"Release build"
#endif
				L" (" __DATE__ " " __TIME__ ")");

			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
			case IDOK:
			case IDCANCEL:
				EndDialog(hDlg, FALSE);
				return TRUE;
			}
	}
	return FALSE;
}
