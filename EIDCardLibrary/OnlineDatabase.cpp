#include <Windows.h>
#include <tchar.h>
#include <Winhttp.h>

#include "CertificateUtilities.h"
#include "Tracing.h"

#pragma comment(lib,"Version.lib")
#pragma comment(lib,"Winhttp.lib")
#pragma comment(lib,"Wininet.lib")

extern "C"
{
	// wininet and winhttp conflicts
	BOOLAPI
	InternetCanonicalizeUrlA(
		__in LPCSTR lpszUrl,
		__out_ecount(*lpdwBufferLength) LPSTR lpszBuffer,
		__inout LPDWORD lpdwBufferLength,
		__in DWORD dwFlags
		);
	BOOLAPI
	InternetCanonicalizeUrlW(
		__in LPCWSTR lpszUrl,
		__out_ecount(*lpdwBufferLength) LPWSTR lpszBuffer,
		__inout LPDWORD lpdwBufferLength,
		__in DWORD dwFlags
		);
	#ifdef UNICODE
	#define InternetCanonicalizeUrl  InternetCanonicalizeUrlW
	#else
	#define InternetCanonicalizeUrl  InternetCanonicalizeUrlA
	#endif // !UNICODE
}

BOOL PostDataToTheSupportSite(PTSTR szPostData)
{
	HINTERNET hSession = NULL;
	HINTERNET hConnect = NULL;
	HINTERNET hRequest = NULL;
	DWORD dwError = 0;
	BOOL fReturn = FALSE;
	__try
	{ 
		hSession = WinHttpOpen(TEXT("EIDAuthenticate"), 
				WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 
				WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (!hSession)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Failed WinHttpOpen 0x%08X",dwError);
			__leave;
		}
		hConnect = WinHttpConnect(hSession, TEXT("www.mysmartlogon.com"),INTERNET_DEFAULT_PORT, 0);
		if (!hConnect)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Failed WinHttpConnect 0x%08X",dwError);
			__leave;
		}
		// WINHTTP_FLAG_SECURE
		hRequest = WinHttpOpenRequest(hConnect,TEXT("POST"),TEXT("/support/submitReport.aspx"),NULL,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,0);
		if (!hRequest)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Failed WinHttpOpenRequest 0x%08X",dwError);
			__leave;
		}
		LPCTSTR additionalHeaders = TEXT("Content-Type: application/x-www-form-urlencoded\r\n");
		if (!WinHttpSendRequest(hRequest, additionalHeaders, (DWORD) -1, (LPVOID)szPostData, (DWORD) (_tcslen(szPostData)*sizeof(TCHAR)), (DWORD) _tcslen(szPostData)*sizeof(TCHAR), 0))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Failed WinHttpSendRequest 0x%08X",dwError);
			__leave;
		}
		fReturn = TRUE;
	}
	__finally
	{
		if (hSession)
			WinHttpCloseHandle(hSession);
	}
	SetLastError(dwError);
	return fReturn;
}

BOOL CommunicateTestNotOK(DWORD dwErrorCode, PTSTR szEmail, PTSTR szTracingFile)
{
	BOOL fReturn = FALSE;
	TCHAR szReaderName[256] = TEXT("");
	TCHAR szCardName[256] = TEXT("");
	TCHAR szProviderName[256] = TEXT("");
	TCHAR szATR[256] = TEXT("");
	TCHAR szATRMask[256] = TEXT("");
	TCHAR szCspDll[256] = TEXT("");
	TCHAR szOsInfo[256] = TEXT("");
	TCHAR szHardwareInfo[256] = TEXT("");
	TCHAR szFileVersion[256] = TEXT("");
	TCHAR szCompany[256] = TEXT("");
	DWORD dwProviderNameLen = ARRAYSIZE(szProviderName);
	DWORD dwSize;
	TCHAR szPostData[100000]= TEXT("");

	if (!AskForCard(szReaderName,256,szCardName,256))
	{
		return FALSE;
	}
	SchGetProviderNameFromCardName(szCardName, szProviderName, &dwProviderNameLen);
	HKEY hRegKeyCalais, hRegKeyCSP, hRegKey;
	// smart card info (atr & mask)
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Cryptography\\Calais\\SmartCards"), 0, KEY_READ, &hRegKeyCalais))
	{
		BYTE bATR[100];
		DWORD dwSize = sizeof(bATR);
		if (!RegOpenKeyEx(hRegKeyCalais, szCardName, 0, KEY_READ, &hRegKey))
		{
			RegQueryValueEx(hRegKey,TEXT("ATR"), NULL, NULL,(PBYTE)&bATR,&dwSize);
			for(DWORD i=0; i< dwSize; i++)
			{
				_stprintf_s(szATR + 2*i, ARRAYSIZE(szATR) - 2*i,TEXT("%02X"),bATR[i]);
			}
			dwSize = sizeof(bATR);
			RegQueryValueEx(hRegKey,TEXT("ATRMask"), NULL, NULL,(PBYTE)&bATR,&dwSize);
			for(DWORD i=0; i< dwSize; i++)
			{
				_stprintf_s(szATRMask + 2*i, ARRAYSIZE(szATRMask) - 2*i,TEXT("%02X"),bATR[i]);
			}
			if (_tcscmp(TEXT("Microsoft Base Smart Card Crypto Provider"), szProviderName) == 0)
			{
				dwSize = sizeof(szCspDll);
				RegQueryValueEx(hRegKey,TEXT("80000001"), NULL, NULL,(PBYTE)&szCspDll,&dwSize);
			}
			RegCloseKey(hRegKey);
		}
		RegCloseKey(hRegKeyCalais);
	}
	if (szCspDll[0] == 0)
	{
		// csp info
		if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider"), 0, KEY_READ, &hRegKeyCSP))
		{
			dwSize = sizeof(szCspDll);
			if (!RegOpenKeyEx(hRegKeyCalais, szProviderName, 0, KEY_READ, &hRegKey))
			{
				RegQueryValueEx(hRegKey, TEXT("Image Path"), NULL,NULL,(PBYTE)&szCspDll,&dwSize);
				RegCloseKey(hRegKey);
			}
			RegCloseKey(hRegKeyCalais);
		}
	}
	if (szCspDll[0] != 0)
	{
		DWORD dwHandle;
		dwSize = GetFileVersionInfoSize(szCspDll, &dwHandle);
		if (dwSize)
		{
			UINT uSize;
			PVOID versionInfo = malloc(dwSize);
			PWSTR pszFileVersion = NULL;
			PWSTR pszCompany = NULL;
			if (GetFileVersionInfo(szCspDll, dwHandle, dwSize, versionInfo))
			{
				BOOL retVal; 
				LPVOID version=NULL;
				DWORD vLen,langD;
				TCHAR szfileVersionPath[256];
				retVal = VerQueryValue(versionInfo,TEXT("\\VarFileInfo\\Translation"),&version,(UINT *)&vLen);
				if (retVal && vLen==4) 
				{
					memcpy(&langD,version,4);            
					_stprintf_s(szfileVersionPath, ARRAYSIZE(szfileVersionPath),
								TEXT("\\StringFileInfo\\%02X%02X%02X%02X\\FileVersion"),
							(langD & 0xff00)>>8,langD & 0xff,(langD & 0xff000000)>>24, 
							(langD & 0xff0000)>>16);            
				}
				else 
					_stprintf_s(szfileVersionPath, ARRAYSIZE(szfileVersionPath),
								TEXT("\\StringFileInfo\\%04X04B0\\FileVersion"),
							GetUserDefaultLangID());
				retVal = VerQueryValue(versionInfo,szfileVersionPath,(PVOID*)&pszFileVersion,(UINT *)&uSize);

				if (pszFileVersion != NULL) 
					_stprintf_s(szFileVersion, ARRAYSIZE(szFileVersion),TEXT("%ls"),pszFileVersion);

				if (retVal && vLen==4) 
				{
					memcpy(&langD,version,4);            
					_stprintf_s(szfileVersionPath, ARRAYSIZE(szfileVersionPath),
								TEXT("\\StringFileInfo\\%02X%02X%02X%02X\\CompanyName"),
							(langD & 0xff00)>>8,langD & 0xff,(langD & 0xff000000)>>24, 
							(langD & 0xff0000)>>16);            
				}
				else 
					_stprintf_s(szfileVersionPath, ARRAYSIZE(szfileVersionPath),
								TEXT("\\StringFileInfo\\%04X04B0\\CompanyName"),
							GetUserDefaultLangID());
				retVal = VerQueryValue(versionInfo,szfileVersionPath,(PVOID*)&pszCompany,(UINT *)&uSize);

				if (pszFileVersion != NULL) 
					_stprintf_s(szCompany, ARRAYSIZE(szCompany),TEXT("%ls"),pszCompany);
			}
			free(versionInfo);
		}
	}

	// os version
	OSVERSIONINFOEX version;
	version.dwOSVersionInfoSize = sizeof(version);
	GetVersionEx((LPOSVERSIONINFO )&version);
	_stprintf_s(szOsInfo, ARRAYSIZE(szOsInfo),TEXT("%d.%d.%d;%d;%d.%d;%s"), 
								version.dwMajorVersion, version.dwMinorVersion, 
								version.dwBuildNumber, version.dwPlatformId,
								version.wSuiteMask, version.wProductType, 
								version.szCSDVersion);
	
	// hardware info
	SYSTEM_INFO SystemInfo;
	GetNativeSystemInfo(&SystemInfo);
	_stprintf_s(szHardwareInfo, ARRAYSIZE(szHardwareInfo), TEXT("%u;%u;%u"), 
      SystemInfo.dwNumberOfProcessors, SystemInfo.dwProcessorType, SystemInfo.wProcessorRevision);

	dwSize = ARRAYSIZE(szPostData) - (DWORD) _tcslen(szPostData) -1;
	_tcscat_s(szPostData, dwSize, TEXT("hardwareInfo="));
	dwSize = ARRAYSIZE(szPostData) - (DWORD) _tcslen(szPostData) -1;
	InternetCanonicalizeUrl(szHardwareInfo,szPostData + _tcslen(szPostData), &dwSize,0);
	dwSize = ARRAYSIZE(szPostData) - (DWORD) _tcslen(szPostData) -1;
	_tcscat_s(szPostData, dwSize, TEXT("&osInfo="));
	dwSize = ARRAYSIZE(szPostData) - (DWORD) _tcslen(szPostData) -1;
	InternetCanonicalizeUrl(szOsInfo,szPostData + _tcslen(szPostData), &dwSize,0);
	_tcscat_s(szPostData, ARRAYSIZE(szPostData), TEXT("\r\n"));
	PostDataToTheSupportSite(szPostData);
	return fReturn;
}

BOOL CommunicateTestOK()
{
	return CommunicateTestNotOK(0, NULL, NULL);
}
