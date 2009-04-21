/*	EID Authentication
    Copyright (C) 2009 Vincent Le Toux

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License version 2.1 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <windows.h>
#include <tchar.h>
#include "EIDCardLibrary.h"
#include "Tracing.h"
#include "eidlib.h"
#include "eidlibException.h"

#ifndef  _M_X64
#pragma comment(lib,"../EIDCardLibrary/beid35libCpp")
#endif

using namespace eIDMW;

BOOL GetBEIDCertificateData(__in LPCTSTR szReaderName,__out LPTSTR *pszContainerName,
							__out PDWORD pdwKeySpec, __out PBYTE *ppbData, __out PDWORD pdwCount,
							__in_opt DWORD dwKeySpec = 0)
{
	EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Retrieving certificate");
#ifndef  _M_X64
	BEID_ReaderSet *m_ReaderSet=NULL;
	BEID_ReaderContext *reader=NULL;
	try
	{
		m_ReaderSet = &BEID_ReaderSet::instance();
	#ifdef UNICODE
		PCHAR szReaderName2 = (PCHAR) malloc(wcslen(szReaderName) +1);
			
		WideCharToMultiByte(CP_ACP,0,szReaderName, -1, szReaderName2, wcslen(szReaderName)+1,NULL, NULL);
		reader = &m_ReaderSet->getReaderByName(szReaderName2);
		free(szReaderName2);
	#else
		reader = &m_ReaderSet->getReaderByName(szReaderName);
	#endif
		BEID_EIDCard& card = reader->getEIDCard();
		BEID_ByteArray cardInfo = card.getRawData_CardInfo();
		
		BEID_Certificates& certificates = card.getCertificates();
		BEID_Certificate *authenticationCertificate;
		if (dwKeySpec == AT_SIGNATURE)
		{
			authenticationCertificate = &(certificates.getSignature());
			*pdwKeySpec = AT_SIGNATURE;
		}
		else
		{
			authenticationCertificate = &(certificates.getAuthentication());
			*pdwKeySpec = AT_SIGNATURE;
		}
		
		BEID_ByteArray certificateData = authenticationCertificate->getCertData();
		*pdwCount = certificateData.Size();
		*ppbData = (PBYTE) malloc(*pdwCount);
		memcpy(*ppbData,certificateData.GetBytes(),*pdwCount);
		const unsigned char * pSerialNum = cardInfo.GetBytes();
		if (dwKeySpec == AT_SIGNATURE)
		{
			DWORD dwSize = sizeof(TEXT("Signature()"))+2*16*sizeof(TCHAR);
			*pszContainerName = (LPTSTR) malloc(dwSize);
			_stprintf_s(*pszContainerName,dwSize/sizeof(TCHAR),TEXT("Signature(%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X)"),
				pSerialNum[0],pSerialNum[1],pSerialNum[2],pSerialNum[3],
				pSerialNum[4],pSerialNum[5],pSerialNum[6],pSerialNum[7],
				pSerialNum[8],pSerialNum[9],pSerialNum[10],pSerialNum[11],
				pSerialNum[12],pSerialNum[13],pSerialNum[14],pSerialNum[15]);
		}
		else
		{
			DWORD dwSize = sizeof(TEXT("Authentication()"))+2*16*sizeof(TCHAR);
			*pszContainerName = (LPTSTR) malloc(dwSize);
			_stprintf_s(*pszContainerName,dwSize/sizeof(TCHAR),TEXT("Authentication(%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X)"),
				pSerialNum[0],pSerialNum[1],pSerialNum[2],pSerialNum[3],
				pSerialNum[4],pSerialNum[5],pSerialNum[6],pSerialNum[7],
				pSerialNum[8],pSerialNum[9],pSerialNum[10],pSerialNum[11],
				pSerialNum[12],pSerialNum[13],pSerialNum[14],pSerialNum[15]);

		}
		}
	catch(...)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Exception");
	}
	BEID_ReleaseSDK();
	return TRUE;
#else
	UNREFERENCED_PARAMETER(dwKeySpec);
	UNREFERENCED_PARAMETER(pdwCount);
	UNREFERENCED_PARAMETER(ppbData);
	UNREFERENCED_PARAMETER(pdwKeySpec);
	UNREFERENCED_PARAMETER(pszContainerName);
	UNREFERENCED_PARAMETER(szReaderName);
	return FALSE;
#endif
}


PCCERT_CONTEXT GetBEIDCertificateFromCspInfo(__in PEID_SMARTCARD_CSP_INFO pCspInfo)
{
#ifndef  _M_X64
	PCCERT_CONTEXT pCertContext = NULL;
	LPTSTR szContainerName = pCspInfo->bBuffer + pCspInfo->nContainerNameOffset;
	LPTSTR szProviderName = pCspInfo->bBuffer + pCspInfo->nCSPNameOffset;
	LPTSTR szReaderName = pCspInfo->bBuffer + pCspInfo->nReaderNameOffset;

	BEID_ReaderSet *m_ReaderSet=NULL;
	BEID_ReaderContext *reader=NULL;
	try
	{
		m_ReaderSet = &BEID_ReaderSet::instance();
	#ifdef UNICODE
		PCHAR szReaderName2 = (PCHAR) malloc(wcslen(szReaderName) +1);
			
		WideCharToMultiByte(CP_ACP,0,szReaderName, -1, szReaderName2, wcslen(szReaderName)+1,NULL, NULL);
		reader = &m_ReaderSet->getReaderByName(szReaderName2);
		free(szReaderName2);
	#else
		reader = &m_ReaderSet->getReaderByName(szReaderName);
	#endif
		BEID_EIDCard& card = reader->getEIDCard();
		BEID_ByteArray cardInfo = card.getRawData_CardInfo();
		
		BEID_Certificates& certificates = card.getCertificates();
		BEID_Certificate *authenticationCertificate;
		/*if (pCspInfo->KeySpec == AT_SIGNATURE)
		{
			authenticationCertificate = &(certificates.getSignature());
		}
		else
		{*/
			authenticationCertificate = &(certificates.getAuthentication());
		//}
		
		BEID_ByteArray certificateData = authenticationCertificate->getCertData();
		pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING, certificateData.GetBytes(), certificateData.Size()); 
		if (pCertContext) {
			// save reference to CSP (else we can't access private key)
			CRYPT_KEY_PROV_INFO KeyProvInfo = {0};
			KeyProvInfo.pwszProvName = szProviderName;
			KeyProvInfo.pwszContainerName = szContainerName;
			KeyProvInfo.dwProvType = PROV_RSA_FULL;
			KeyProvInfo.dwKeySpec = pCspInfo->KeySpec;

			CertSetCertificateContextProperty(pCertContext,CERT_KEY_PROV_INFO_PROP_ID,0,&KeyProvInfo);
			EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"Certificate OK");

		}
		else
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Unable to CertCreateCertificateContext : %d",GetLastError());
		}
	}
	catch(...)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Exception");
	}
	BEID_ReleaseSDK();
	return pCertContext;
#else
	UNREFERENCED_PARAMETER(pCspInfo);
	return NULL;
#endif
}


BOOL SolveBEIDChallenge(__in PCCERT_CONTEXT pCertContext, __in LPCTSTR Pin)
{
#ifndef  _M_X64
	PSTR szPin = NULL;
	BOOL fAuthenticated = FALSE;
	UNREFERENCED_PARAMETER(pCertContext);
	try
	{
#ifdef UNICODE
		szPin = (PCHAR) malloc(wcslen(Pin) +1);
		WideCharToMultiByte(CP_ACP,0,Pin, -1, szPin, wcslen(Pin)+1,NULL, NULL);
#else
		szPin = Pin;
#endif
		unsigned long ulRemaining=0xFFFF;

		BEID_ReaderContext &reader = ReaderSet.getReader();
		BEID_EIDCard &card = reader.getEIDCard();

		if(card.getPins().getPinByNumber(0).verifyPin(szPin,ulRemaining))
		{
			fAuthenticated = TRUE;
			EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"verify pin succeeded");
		}
		else
		{
			if(ulRemaining==0xFFFF)
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"verify pin canceled");
			}
			else
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"verify pin failed (%d tries left)",ulRemaining );
			}
		}
	}
    catch(BEID_ExCardBadType &ex)
	{
        EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"This is not an eid card (0x%08x)",ex.GetError());
	}
    catch(BEID_ExNoCardPresent &ex)
	{
        EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"No card present (0x%08x)",ex.GetError());
	}
    catch(BEID_ExNoReader &ex)
	{
        EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"No reader found (0x%08x)",ex.GetError());
	}
    catch(BEID_Exception &ex)
	{
        EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"BEID_Exception exception (0x%08x)",ex.GetError());
	}
    catch(...)
	{
        EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"Other exception");
	}
#ifdef UNICODE
	if(szPin)
		free(szPin);
#endif
	BEID_ReleaseSDK();
	return fAuthenticated;
#else
	UNREFERENCED_PARAMETER(Pin);
	UNREFERENCED_PARAMETER(pCertContext);
	return FALSE;
#endif
}
/*
#define BEID_MAX_CERT_LEN                            2048
#define BEID_MAX_CERT_LABEL_LEN                 256
#define BEID_MAX_CERT_NUMBER                     10
#define BEID_PIN_TYPE_PKCS15 0
#define BEID_USAGE_AUTH 1
#define BEID_INTERFACE_VERSION                     1  // Changes each time the interface is modified 
#define BEID_INTERFACE_COMPAT_VERSION        1  // Stays until incompatible changes in existing functions 
#define BELPIC_MAX_USER_PIN_LEN     12


typedef struct 
{
	long general;                           
	long system;                           
	long pcsc;                             
	BYTE cardSW[2];                 
	BYTE rfu[6];
} BEID_Status;

typedef struct 
{
	BYTE certif[BEID_MAX_CERT_LEN];   
	long certifLength;                
	char certifLabel[BEID_MAX_CERT_LABEL_LEN+1];
	long certifStatus;                
	BYTE rfu[6];
} BEID_Certif;

typedef struct 
{
	long usedPolicy;                              
	BEID_Certif certificates[BEID_MAX_CERT_NUMBER];
	long certificatesLength;                 
	long signatureCheck;                   
	BYTE rfu[6];
} BEID_Certif_Check;


typedef struct 
{
	BYTE SerialNumber[16];                    
	BYTE ComponentCode;                 
	BYTE OSNumber;                       
	BYTE OSVersion;                      
	BYTE SoftmaskNumber;                      
	BYTE SoftmaskVersion;               
	BYTE AppletVersion;                             
	unsigned short GlobalOSVersion;                             
	BYTE AppletInterfaceVersion;                          
	BYTE PKCS1Support;                              
	BYTE KeyExchangeVersion;                              
	BYTE ApplicationLifeCycle;
	BYTE GraphPerso;                          
	BYTE ElecPerso;
	BYTE ElecPersoInterface;                              
	BYTE Reserved;                                                          
	BYTE rfu[6];
} BEID_VersionInfo;


typedef  struct 
{ 
	BYTE *data; 
	unsigned long length; 
	BYTE rfu[6];
} BEID_Bytes; 

typedef struct {
	long pinType; // PIN Type (see 2.10.1)
	BYTE id; // PIN reference or ID
	long usageCode; // PIN Usage (see 2.10.2)
	char *shortUsage; // May be NULL for usage known by the middleware
	char *longUsage; // May be NULL for usage known by the middleware
	BYTE rfu[6];
} BEID_Pin;

typedef BEID_Status (*BEID_InitEx) (char *ReaderName, long OCSP, long CRL, long *CardHandle,long interfaceVersion, long interfaceCompVersion ); 
typedef BEID_Status (*BEID_GetCertificates) (BEID_Certif_Check *CertifCheck);
typedef BEID_Status (*BEID_GetVersionInfo) (BEID_VersionInfo *pVersionInfo, BOOL Signature, BEID_Bytes *SignedStatus);
typedef BEID_Status (*BEID_VerifyPIN) (BEID_Pin *pin, char *Pin, long *TriesLeft);
typedef BEID_Status (*BEID_Exit) ();

BOOL GetBEIDCertificateData(__in LPCTSTR szReaderName,__out LPTSTR *pszContainerName,
							__out PDWORD pdwKeySpec, __out PBYTE *ppbData, __out PDWORD pdwCount,
							__in_opt DWORD dwKeySpec = 0)
{
	
	BOOL fReturn = FALSE;
	BEID_InitEx MyBEID_InitEx = NULL;
	BEID_GetCertificates MyBEID_GetCertificates = NULL;
	BEID_GetVersionInfo MyBEID_GetVersionInfo = NULL;
	BEID_Exit MyBEID_Exit = NULL;
	HMODULE hEidLib = NULL;
	BEID_Status Status;
	BEID_Certif_Check MyBEID_Certif_Check;
	LONG CardHandle = NULL;
	BEID_Certif *authenticationCertificate = NULL;
	BEID_VersionInfo tVersionInfo = {0};
	BEID_Bytes tSignature = {0};
	BYTE bufferSig[256] = {0};
	tSignature.length = 256;
	tSignature.data = bufferSig;

	__try
	{
		*ppbData = NULL;
		*pdwCount = 0;
		*pszContainerName = NULL;
		hEidLib = LoadLibrary(TEXT("eidlib.dll"));
		if (!hEidLib)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"LoadLibrary eidlib error = %d",GetLastError());
			__leave;
		}
		MyBEID_InitEx = (BEID_InitEx) GetProcAddress(hEidLib, "BEID_InitEx");
		if (!MyBEID_InitEx)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetProcAddress BEID_InitEx error = %d",GetLastError());
			__leave;
		}
		MyBEID_GetVersionInfo = (BEID_GetVersionInfo) GetProcAddress(hEidLib, "BEID_GetVersionInfo");
		if (!MyBEID_GetVersionInfo)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetProcAddress BEID_GetVersionInfo error = %d",GetLastError());
			__leave;
		}
		MyBEID_GetCertificates = (BEID_GetCertificates) GetProcAddress(hEidLib, "BEID_GetCertificates");
		if (!MyBEID_GetCertificates)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetProcAddress BEID_GetCertificates error = %d",GetLastError());
			__leave;
		}
		MyBEID_Exit = (BEID_Exit) GetProcAddress(hEidLib, "BEID_Exit");
		if (!MyBEID_Exit)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetProcAddress BEID_Exit error = %d",GetLastError());
			__leave;
		}
#ifdef UNICODE
		{
			CHAR szReaderName2[1000];
			WideCharToMultiByte(CP_ACP,0,szReaderName, -1, szReaderName2, wcslen(szReaderName)+1,NULL, NULL);
			Status = MyBEID_InitEx(szReaderName2, 0, 0, &CardHandle, BEID_INTERFACE_VERSION, BEID_INTERFACE_COMPAT_VERSION);
		}
#else
		Status = MyBEID_Init(szReaderName, 0, 0, &CardHandle, BEID_INTERFACE_VERSION, BEID_INTERFACE_COMPAT_VERSION);
#endif
		if (Status.general != 0)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"MyBEID_Init error = %d",Status.general);
			__leave;
		}
		Status = MyBEID_GetVersionInfo(&tVersionInfo, FALSE, &tSignature);
		if (Status.general != 0)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"MyBEID_GetVersionInfo error = %d",Status.general);
			__leave;
		}
		memset(&MyBEID_Certif_Check, 0, sizeof(BEID_Certif_Check));
		MyBEID_Certif_Check.certificatesLength = BEID_MAX_CERT_NUMBER;
		Status = MyBEID_GetCertificates(&MyBEID_Certif_Check);
		if (Status.general != 0)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"MyBEID_GetCertificates error = %d",Status.general);
			__leave;
		}
		*pdwKeySpec = AT_SIGNATURE;
		
		for (int i = 0; i < MyBEID_Certif_Check.certificatesLength; i++)
		{
			if (dwKeySpec == AT_SIGNATURE)
			{
				if (strcmp(MyBEID_Certif_Check.certificates[i].certifLabel , "Signature") == 0)
				{
					authenticationCertificate = &MyBEID_Certif_Check.certificates[i];
					break;
				}
			}
			else
			{
				if (strcmp(MyBEID_Certif_Check.certificates[i].certifLabel , "Authentication") == 0)
				{
					authenticationCertificate = &MyBEID_Certif_Check.certificates[i];
					break;
				}
			}
		}
		if (!authenticationCertificate)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Certificate not found");
			__leave;
		}
	
		*pdwCount = authenticationCertificate->certifLength;
		*ppbData = (PBYTE) malloc(*pdwCount);
		if (!*ppbData)
		{
			__leave;
		}

		memcpy(*ppbData,authenticationCertificate->certif,*pdwCount);
		PBYTE pSerialNum = tVersionInfo.SerialNumber;
		if (dwKeySpec == AT_SIGNATURE)
		{
			DWORD dwSize = sizeof(TEXT("Signature()"))+2*16*sizeof(TCHAR);
			*pszContainerName = (LPTSTR) malloc(dwSize);
			if (!*pszContainerName)
			{
				__leave;
			}
			_stprintf_s(*pszContainerName,dwSize/sizeof(TCHAR),TEXT("Signature(%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X)"),
				pSerialNum[0],pSerialNum[1],pSerialNum[2],pSerialNum[3],
				pSerialNum[4],pSerialNum[5],pSerialNum[6],pSerialNum[7],
				pSerialNum[8],pSerialNum[9],pSerialNum[10],pSerialNum[11],
				pSerialNum[12],pSerialNum[13],pSerialNum[14],pSerialNum[15]);
		}
		else
		{
			DWORD dwSize = sizeof(TEXT("Authentication()"))+2*16*sizeof(TCHAR);
			*pszContainerName = (LPTSTR) malloc(dwSize);
			if (!*pszContainerName)
			{
				__leave;
			}
			_stprintf_s(*pszContainerName,dwSize/sizeof(TCHAR),TEXT("Authentication(%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X)"),
				pSerialNum[0],pSerialNum[1],pSerialNum[2],pSerialNum[3],
				pSerialNum[4],pSerialNum[5],pSerialNum[6],pSerialNum[7],
				pSerialNum[8],pSerialNum[9],pSerialNum[10],pSerialNum[11],
				pSerialNum[12],pSerialNum[13],pSerialNum[14],pSerialNum[15]);

		}
		fReturn = TRUE;
	}
	__finally
	{
		if (!fReturn)
		{
			if (*pszContainerName)
				free(*pszContainerName);
			if (*ppbData)
				free(*ppbData);
		}
		if (MyBEID_Exit)
			MyBEID_Exit();
		if (hEidLib)
			FreeLibrary(hEidLib);
	}

	return fReturn;
}


PCCERT_CONTEXT GetBEIDCertificateFromCspInfo(__in PEID_SMARTCARD_CSP_INFO pCspInfo)
{
	PCCERT_CONTEXT pCertContext = NULL;
	LPTSTR szContainerName = pCspInfo->bBuffer + pCspInfo->nContainerNameOffset;
	LPTSTR szProviderName = pCspInfo->bBuffer + pCspInfo->nCSPNameOffset;
	LPTSTR szReaderName = pCspInfo->bBuffer + pCspInfo->nReaderNameOffset;

	BEID_InitEx MyBEID_InitEx = NULL;
	BEID_GetCertificates MyBEID_GetCertificates = NULL;
	BEID_Exit MyBEID_Exit = NULL;
	HMODULE hEidLib = NULL;
	BEID_Status Status;
	BEID_Certif_Check MyBEID_Certif_Check;
	LONG CardHandle = NULL;
	BEID_Certif *authenticationCertificate = NULL;

	__try
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"1");
		hEidLib = LoadLibrary(TEXT("eidlib.dll"));
		if (!hEidLib)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"LoadLibrary eidlib error = %d",GetLastError());
			__leave;
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"2");
		MyBEID_InitEx = (BEID_InitEx) GetProcAddress(hEidLib, "BEID_InitEx");
		if (!MyBEID_InitEx)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetProcAddress BEID_InitEx error = %d",GetLastError());
			__leave;
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"3");
		MyBEID_GetCertificates = (BEID_GetCertificates) GetProcAddress(hEidLib, "BEID_GetCertificates");
		if (!MyBEID_GetCertificates)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetProcAddress BEID_GetCertificates error = %d",GetLastError());
			__leave;
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"4");
		MyBEID_Exit = (BEID_Exit) GetProcAddress(hEidLib, "BEID_Exit");
		if (!MyBEID_Exit)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetProcAddress BEID_Exit error = %d",GetLastError());
			__leave;
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"5");
#ifdef UNICODE
		{
			CHAR szReaderName2[1000];
			WideCharToMultiByte(CP_ACP,0,szReaderName, -1, szReaderName2, wcslen(szReaderName)+1,NULL, NULL);
			Status = MyBEID_InitEx(szReaderName2, 0, 0, &CardHandle, BEID_INTERFACE_VERSION, BEID_INTERFACE_COMPAT_VERSION);
		}
#else
		Status = MyBEID_InitEx(szReaderName, 0, 0, &CardHandle, BEID_INTERFACE_VERSION, BEID_INTERFACE_COMPAT_VERSION);
#endif
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"6");
		if (Status.general != 0)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"MyBEID_Init error = %d",Status.general);
			__leave;
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"7");
		memset(&MyBEID_Certif_Check, 0, sizeof(BEID_Certif_Check));
		Status = MyBEID_GetCertificates(&MyBEID_Certif_Check);
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"8");
		if (Status.general != 0)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"MyBEID_GetCertificates error = %d",Status.general);
			__leave;
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"9");
		for (int i = 0; i < MyBEID_Certif_Check.certificatesLength; i++)
		{
			if (strcmp(MyBEID_Certif_Check.certificates[i].certifLabel , "Authentication") == 0)
			{
				authenticationCertificate = &MyBEID_Certif_Check.certificates[i];
				break;
			}
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"10");
		if (!authenticationCertificate)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Certificate not found");
			__leave;
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"11");
		pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING, authenticationCertificate->certif, authenticationCertificate->certifLength); 
		if (pCertContext) {
			// save reference to CSP (else we can't access private key)
			CRYPT_KEY_PROV_INFO KeyProvInfo;
			memset(&KeyProvInfo, 0, sizeof(CRYPT_KEY_PROV_INFO));
			KeyProvInfo.pwszProvName = szProviderName;
			KeyProvInfo.pwszContainerName = szContainerName;
			KeyProvInfo.dwProvType = PROV_RSA_FULL;
			KeyProvInfo.dwKeySpec = pCspInfo->KeySpec;

			CertSetCertificateContextProperty(pCertContext,CERT_KEY_PROV_INFO_PROP_ID,0,&KeyProvInfo);
			EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"Certificate OK");
		}
		else
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Unable to CertCreateCertificateContext : %d",GetLastError());
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"12");
	}
	__finally
	{
		if (MyBEID_Exit)
			MyBEID_Exit();
		if (hEidLib)
			FreeLibrary(hEidLib);
	}
	return pCertContext;
}


BOOL SolveBEIDChallenge(__in PCCERT_CONTEXT pCertContext, __in LPCTSTR Pin)
{
	UNREFERENCED_PARAMETER(pCertContext);
	BOOL fAuthenticated = FALSE;
	BEID_InitEx MyBEID_InitEx = NULL;
	BEID_VerifyPIN MyBEID_VerifyPIN = NULL;
	BEID_Exit MyBEID_Exit = NULL;
	HMODULE hEidLib = NULL;
	BEID_Status Status;
	LONG CardHandle = NULL;
	BEID_Pin MyBEID_Pin;
	LONG ulRemaining = 0;
	CHAR szPin[BELPIC_MAX_USER_PIN_LEN];
	__try
	{
#ifdef UNICODE
		WideCharToMultiByte(CP_ACP,0,Pin, -1, szPin, BELPIC_MAX_USER_PIN_LEN,NULL, NULL);
#else
		strcpy(szPin,Pin);
#endif
		hEidLib = LoadLibrary(TEXT("eidlib.dll"));
		if (!hEidLib)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"LoadLibrary eidlib error = %d",GetLastError());
			__leave;
		}
		MyBEID_InitEx = (BEID_InitEx) GetProcAddress(hEidLib, "BEID_InitEx");
		if (!MyBEID_InitEx)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetProcAddress BEID_InitEx error = %d",GetLastError());
			__leave;
		}
		MyBEID_VerifyPIN = (BEID_VerifyPIN) GetProcAddress(hEidLib, "BEID_VerifyPIN");
		if (!MyBEID_VerifyPIN)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetProcAddress BEID_VerifyPIN error = %d",GetLastError());
			__leave;
		}
		MyBEID_Exit = (BEID_Exit) GetProcAddress(hEidLib, "BEID_Exit");
		if (!MyBEID_Exit)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetProcAddress BEID_Exit error = %d",GetLastError());
			__leave;
		}
		Status = MyBEID_InitEx(NULL, 0, 0, &CardHandle, BEID_INTERFACE_VERSION, BEID_INTERFACE_COMPAT_VERSION);
		if (Status.general != 0)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"MyBEID_Init error = %d",Status.general);
			__leave;
		}
		memset(&MyBEID_Pin, 0, sizeof(BEID_Pin));
		MyBEID_Pin.id = 0x01;
		MyBEID_Pin.pinType = BEID_PIN_TYPE_PKCS15;
		MyBEID_Pin.usageCode = BEID_USAGE_AUTH;
		Status = MyBEID_VerifyPIN(&MyBEID_Pin, szPin, &ulRemaining);
		if (Status.general != 0)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"MyBEID_VerifyPIN error = %d Remaining = %d",Status.general,ulRemaining);
			__leave;
		}
		fAuthenticated = TRUE;
	}
	__finally
	{
		if (MyBEID_Exit)
			MyBEID_Exit();
		if (hEidLib)
			FreeLibrary(hEidLib);
	}
	return fAuthenticated;
}*/