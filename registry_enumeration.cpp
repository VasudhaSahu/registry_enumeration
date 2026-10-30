// RegEnum.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include<windows.h>
#include<tchar.h>
#include<process.h>
#include<conio.h>
#include<strsafe.h>
#include<string.h>
#include<fstream>
#include<list>
#include<vector>
using namespace std;

#pragma comment(lib, "advapi32.lib")

#define ARRAY_SIZE(X)  (sizeof(X)/sizeof((X)[0])

#define EXPORT_PATH ".\\Output\\test"
#define EXT ".ini"

#define HIVE 5

#define  MAX_KEY_NAME 255
#define MAX_VALUE_NAME 260//16383
#define MAX_VALUE 1024*1024//1MB(standard) Available Memory(latest)

typedef struct RegKey
{
	string szAppName;
	string szKeyName;
	string szValue;
}RegKey;

BOOL ExportRegistrySubKeys(HKEY hKey, LPCTSTR pKeyRootName, LPCTSTR pKey, list<RegKey>* lstReg);
void DisplayValues(HKEY hCurKey,string pKeyName, DWORD dNoOfValue,DWORD dLongestKeyLen,DWORD dLongestValueLen,list<RegKey>*lstReg);
void WriteFunc(list<RegKey>*lstReg,LPCTSTR lpFileName);
unsigned int _stdcall ThreadProc(void* vParam);

list<RegKey> ListReg[5];

HKEY arrHKey[5] = { HKEY_CLASSES_ROOT,HKEY_CURRENT_USER,HKEY_LOCAL_MACHINE,HKEY_USERS,HKEY_CURRENT_CONFIG };

int main()
{
	HANDLE hThreadArray[5];
	int iParam[] = { 0,1,2,3,4 };

	for (int i = 0; i < 5; i++)
	{
		hThreadArray[i] = (HANDLE)_beginthreadex(NULL, 0, &ThreadProc, &iParam[i], 0, NULL);
		if (0 == hThreadArray[i])
		{
			cout << "\n Thread creation Failed";
			return -1;
		}
	}

	DWORD dWaitVal = WaitForMultipleObjects(HIVE, hThreadArray, TRUE, INFINITE);
	for (int j = 0; j < HIVE; j++)
	{
		CloseHandle(hThreadArray[j]);
	}

	_getch();

	return 0;

}
unsigned int __stdcall ThreadProc(void* vparam)
{
	int* iThreadId = static_cast<int*>(vparam);
	char szPath[260];
	sprintf_s(szPath, "%s%d%s", EXPORT_PATH, *iThreadId, EXT);

	ExportRegistrySubKeys(arrHKey[*iThreadId], NULL, NULL, &ListReg[*iThreadId]);
	WriteFunc(&ListReg[*iThreadId], szPath);

	return 1;
}

void WriteFunc(list<RegKey>* lstReg, LPCTSTR lpFilename)
{
	BOOL bRet;
	int iRet=1;
	fstream fobj;
	fobj.open(lpFilename,ios::out | ios::app);
	list<RegKey>::iterator itr;
	printf("\nNumber of elements in to be written in %s: %d", lpFilename, lstReg->size());
	for (itr = lstReg->begin(); itr != lstReg->end(); itr++)
	{
		iRet = _stricmp(itr->szKeyName.c_str(), "");
		if (iRet == 0)
		{
			fobj << "\n\n[" << itr->szAppName.c_str() << "]\n";
		}
		else
		{
			fobj << itr->szKeyName.c_str() << "=" << itr->szValue.c_str() << "\n";

		}
	}
	fobj.close();
	return;
}
BOOL ExportRegistrySubKeys(HKEY hKey, LPCTSTR pKeyRootName, LPCTSTR pKeyName, list<RegKey>* lstReg)
{

	LONG lRet = 0;
	HKEY hCurKey;
	string szSectionName;
	DWORD dwNoOfSubKey;
	DWORD dwMaxSubKeyLen;
	DWORD dwMaxValueNameLen;
	DWORD dwMaxValueLenBytes;
	DWORD dwNoOfValues;

	lRet = RegOpenKeyEx(hKey, pKeyName, 0,
		KEY_WOW64_32KEY | KEY_READ | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
		&hCurKey);
	
	if (ERROR_SUCCESS != lRet)
	{
		return FALSE;
	}

	if (NULL == pKeyRootName)
	{
		switch ((ULONG_PTR)hKey)
		{
		case (ULONG_PTR)HKEY_CLASSES_ROOT:
			pKeyRootName = _T("HKEY_CLASSES_ROOT");
			break;
		case (ULONG_PTR)HKEY_CURRENT_USER:
			pKeyRootName = _T("HKEY_CURRENT_USER");
			break;
		case (ULONG_PTR)HKEY_LOCAL_MACHINE:
			pKeyRootName = _T("HKEY_LOCAL_MACHINE");
			break;
		case (ULONG_PTR)HKEY_USERS:
			pKeyRootName = _T("HKEY_USERS");
			break;
		case (ULONG_PTR)HKEY_PERFORMANCE_DATA:
			pKeyRootName = _T("HKEY_PERFORMANCE_DATA");
			break;
		case (ULONG_PTR)HKEY_CURRENT_CONFIG:
			pKeyRootName = _T("HKEY_CURRENT_CONFIG");
			break;
		case (ULONG_PTR)HKEY_DYN_DATA:
			pKeyRootName = _T("HKEY_DYN_DATA");
			break;
		default:
			break;
		}
		szSectionName = string(pKeyRootName);
	}
	else
	{
		szSectionName = (pKeyRootName);
		szSectionName.append( "\\");
		szSectionName.append( pKeyName);
	}
	RegKey stReg;
	stReg.szAppName = szSectionName;
	lstReg->push_back(stReg);

	lRet = RegQueryInfoKey(hCurKey, NULL, NULL, NULL, &dwNoOfSubKey, &dwMaxSubKeyLen, NULL, &dwNoOfValues, &dwMaxValueNameLen, &dwMaxValueLenBytes, NULL, NULL);
	
	if (ERROR_SUCCESS == lRet && dwNoOfValues > 0)
	{
		dwMaxValueNameLen++; //Since this value returned byRegQueryInfo doesn't include NULL character
		DisplayValues(hCurKey, szSectionName, dwNoOfValues, dwMaxValueNameLen, dwMaxValueLenBytes, lstReg);
	}

	TCHAR buffKeyName[MAX_KEY_NAME];

	//Look for other subkeys in this key
	for (DWORD dwKeyIndex = 0;dwKeyIndex< dwNoOfSubKey; dwKeyIndex++)
	{
		DWORD dwSize = MAX_KEY_NAME;
		
		lRet = RegEnumKeyEx(hCurKey,dwKeyIndex, buffKeyName, &dwSize, NULL, NULL, NULL, NULL);

		if (lRet == ERROR_NO_MORE_ITEMS)
		{
			lRet = ERROR_SUCCESS;
			break;
		}
		else 
		if(lRet != ERROR_SUCCESS)
		{
			//Error (Stop looking to prevent deadlocking)
			//_tprintf(_T("ERROR: %d, Subkey index: %d, key: '%s'\n"), dwR, dwKeyIndex, pKeyName);

			break;
		}

		string strSubKey;

		if (NULL != pKeyName)
		{
			strSubKey = pKeyName;
			strSubKey.append("\\");
		}
			strSubKey.append(buffKeyName);// dwSize+1 as RegEnumKey return len excluding Null character.

			lRet = ExportRegistrySubKeys(hKey, pKeyRootName, strSubKey.c_str(), lstReg);
			

	}

	RegCloseKey(hCurKey);

	return TRUE;
}

void DisplayValues(HKEY hCurKey, string pKeyName, DWORD dwNoOfValues, DWORD dwMaxValueNameLen, DWORD dwMaxValueLenBytes, list<RegKey>* lstReg)
{
	LONG lRet;

	if (dwNoOfValues <= 0)
		return;
	
	// RegQueryInfoKey gives us the max name/data sizes for the key. Reserve one more slot
	// for the terminating null byte and a small safety margin for conversions.
	const DWORD valueNameBufferLen = dwMaxValueNameLen + 1;
	const DWORD valueDataBufferLen = dwMaxValueLenBytes + sizeof(TCHAR);

	BYTE* byValue = new BYTE[valueDataBufferLen];
	TCHAR* szValueNameRetrieved = new TCHAR[valueNameBufferLen];

	if (!byValue || !szValueNameRetrieved)
	{
		cout << "\n New failed to allocate memory for values";
		delete[] byValue;
		delete[] szValueNameRetrieved;
		return;
	}

	//Enum values
	for (int nIndex = 0; nIndex < dwNoOfValues; nIndex++)
	{
		DWORD dwValType = 0;
		DWORD dwValNameLen = valueNameBufferLen;
		DWORD dwValDataLength = valueDataBufferLen;

		ZeroMemory(byValue, valueDataBufferLen);
		ZeroMemory(szValueNameRetrieved, valueNameBufferLen * sizeof(TCHAR));

		lRet = RegEnumValue(hCurKey, nIndex, szValueNameRetrieved, &dwValNameLen, NULL, &dwValType, byValue, &dwValDataLength);
		if (lRet == ERROR_NO_MORE_ITEMS)
		{
			lRet = ERROR_SUCCESS;
			break;
		}
		else if (lRet != ERROR_SUCCESS)
		{
			//Error (Stop looking to prevent deadlocking)
			//_tprintf(_T("ERROR: %d, Subkey index: %d, key: '%s'\n"), dwR, dwKeyIndex, pKeyName);

			break;
		}
		DWORD dwValueSize = dwValDataLength;
		dwValueSize /= sizeof(TCHAR);

		TCHAR* szValueName = new TCHAR[(dwValNameLen+1) / sizeof(TCHAR)];// +1 for termination null character
		TCHAR* szValuedData = new TCHAR[dwValueSize];
		if (szValueNameRetrieved[0])
		{
			StringCchCopy(szValueName, dwValNameLen+1, szValueNameRetrieved);
		}
		else
		{
			StringCchCopy(szValueName, dwValNameLen+1, "Default");
		}

		switch (dwValType)
		{
		case REG_SZ:
			{
				if (dwValDataLength > 1 && byValue != NULL)
				{
					StringCchPrintf(szValuedData, (dwValDataLength/sizeof(TCHAR)), "%s", byValue);
				}
				else
				{
					StringCchCopy(szValuedData, (dwValDataLength / sizeof(TCHAR)), "\0");
				}
			}
		break;
		case REG_EXPAND_SZ:
		{
			if (dwValDataLength > 1 && byValue != NULL)
			{
				StringCchPrintf(szValuedData, (dwValDataLength / sizeof(TCHAR)), "%s", byValue);
			}
			else
			{
				StringCchCopy(szValuedData, (dwValDataLength / sizeof(TCHAR)), "\0");
			}
			break;
		}
		case REG_DWORD:
			if (dwValDataLength == sizeof(DWORD))
			{
				byValue[dwValDataLength] = '\0';
				DWORD dwValue = *(DWORD*)byValue;
				StringCchPrintf(szValuedData, (dwValDataLength / sizeof(TCHAR)), "dword:%08x", dwValue);
			}
			break;
		case REG_BINARY:
			if (dwValDataLength > 0)
			{
				if (0 == (*(int*)byValue))
				{
					byValue[dwValDataLength] = '\0';
					StringCchPrintf(szValuedData, (dwValDataLength / sizeof(TCHAR)), "binary:0");
				}
				else
				{
					byValue[dwValDataLength] = '\0';
					StringCchPrintf(szValuedData, (dwValDataLength / sizeof(TCHAR)), "(%d)", byValue);
				}
			}
			break;
		case REG_MULTI_SZ:
		{
			// REG_MULTI_SZ stores a sequence of null-terminated strings, terminated by an extra null.
			// Walk the buffer one string at a time and join individual entries with a readable separator.
			std::string multiString;
			const BYTE* curr = byValue;
			const BYTE* end = byValue + dwValDataLength;

			while (curr < end)
			{
				const char* currStringPtr = reinterpret_cast<const char*>(curr);
				const size_t currStringLength = strnlen(currStringPtr, static_cast<size_t>(end - curr));

				// Stop on the terminator that marks the end of the multi-string buffer.
				if (currStringLength == 0)
				{
					break;
				}

				if (!multiString.empty())
				{
					multiString += " | ";
				}
				multiString.append(currStringPtr, currStringLength);

				// Advance to the next string in the buffer, skipping the terminating null byte.
				curr += currStringLength + 1;
			}

			if (multiString.empty())
			{
				StringCchCopyA(szValuedData, (dwValDataLength / sizeof(TCHAR)) + 1, "");
			}
			else
			{
				StringCchCopyA(szValuedData, (dwValDataLength / sizeof(TCHAR)) + 1, multiString.c_str());
			}
		}
		break;
		default:
			if (dwValDataLength > 0)
			{
				StringCchCopy(szValuedData, (dwValDataLength / sizeof(TCHAR)), "Unspecified");
			}
			else
			{
				StringCchCopy(szValuedData, (dwValDataLength / sizeof(TCHAR)), "\0");
			}
		}

		RegKey stReg;
		stReg.szAppName = pKeyName;
		stReg.szKeyName.assign(szValueName);
		stReg.szValue.assign(szValuedData);
		lstReg->push_back(stReg);

		if (NULL != szValuedData)
			delete[] szValuedData;
		if (NULL != szValueName)
			delete[] szValueName;
		
	} 

	if (byValue)
		delete[] byValue;
	if (NULL != szValueNameRetrieved)
		delete[] szValueNameRetrieved;

	return;
}