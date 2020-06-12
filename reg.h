#ifndef REG_H
#define REG_H

#include <windows.h>
#include <winreg.h>
#include <stdio.h>
#include <tchar.h>
#include <convert.h>
#include <string>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
#define HKLM HKEY_LOCAL_MACHINE
#define HKCU HKEY_CURRENT_USER

struct KEY_VALUE
{
    TCHAR key[MAX_VALUE_NAME];
    BYTE value[MAX_VALUE_NAME];
    int length = 0;
};
struct GROUP_KEY
{
    TCHAR subKey[MAX_VALUE_NAME];
    int subKeyLength = 0;
    int length = 0;
};

KEY_VALUE *QueryKey(HKEY hKey);
GROUP_KEY *QueryGroupKey(HKEY hKey);
KEY_VALUE *GetKeyValue(HKEY hKey, LPCSTR lpSubKey);
KEY_VALUE *GetGroupKeyValue(HKEY hKey, LPCSTR lpSubKey, KEY_VALUE* list, int* length);

KEY_VALUE *QueryKey(HKEY hKey)
{
    TCHAR achKey[MAX_KEY_LENGTH];        // buffer for subkey name
    DWORD cbName;                        // size of name string
    TCHAR achClass[MAX_PATH] = TEXT(""); // buffer for class name
    DWORD cchClassName = MAX_PATH;       // size of class string
    DWORD cSubKeys = 0;                  // number of subkeys
    DWORD cbMaxSubKey;                   // longest subkey size
    DWORD cchMaxClass;                   // longest class string
    DWORD cValues;                       // number of values for key
    DWORD cchMaxValue;                   // longest value name
    DWORD cbMaxValueData;                // longest value data
    DWORD cbSecurityDescriptor;          // size of security descriptor
    FILETIME ftLastWriteTime;            // last write time

    DWORD i, retCode;

    TCHAR achValue[MAX_VALUE_NAME];
    BYTE achData[MAX_VALUE_NAME];
    DWORD cchValue = MAX_VALUE_NAME;
    DWORD cchData = MAX_VALUE_NAME;
    DWORD Type;

    // Get the class name and the value count.
    retCode = RegQueryInfoKey(
        hKey,                  // key handle
        achClass,              // buffer for class name
        &cchClassName,         // size of class string
        NULL,                  // reserved
        &cSubKeys,             // number of subkeys
        &cbMaxSubKey,          // longest subkey size
        &cchMaxClass,          // longest class string
        &cValues,              // number of values for this key
        &cchMaxValue,          // longest value name
        &cbMaxValueData,       // longest value data
        &cbSecurityDescriptor, // security descriptor
        &ftLastWriteTime);     // last write time

    // Enumerate the subkeys, until RegEnumKeyEx fails.

    // Enumerate the key values.
    KEY_VALUE *list = new KEY_VALUE [cValues + 1];
    list[0].length = 0;
    if (cValues)
    {
        for (i = 0, retCode = ERROR_SUCCESS; i < cValues; i++)
        {
            cchData = MAX_VALUE_NAME;
            cchValue = MAX_VALUE_NAME;
            list[i].key[0] = '\0';
            list[i].value[0] = '\0';
            retCode = RegEnumValue(hKey, i, list[i].key, &cchValue, NULL, &Type, list[i].value, &cchData);
            if (retCode == ERROR_SUCCESS)
            {
                list[0].length += 1;
            }
        }
    }
    return list;
}

GROUP_KEY *QueryGroupKey(HKEY hKey)
{
    TCHAR achKey[MAX_KEY_LENGTH];        // buffer for subkey name
    DWORD cbName;                        // size of name string
    TCHAR achClass[MAX_PATH] = TEXT(""); // buffer for class name
    DWORD cchClassName = MAX_PATH;       // size of class string
    DWORD cSubKeys = 0;                  // number of subkeys
    DWORD cbMaxSubKey;                   // longest subkey size
    DWORD cchMaxClass;                   // longest class string
    DWORD cValues;                       // number of values for key
    DWORD cchMaxValue;                   // longest value name
    DWORD cbMaxValueData;                // longest value data
    DWORD cbSecurityDescriptor;          // size of security descriptor
    FILETIME ftLastWriteTime;            // last write time

    DWORD i, retCode;

    TCHAR achValue[MAX_VALUE_NAME];
    BYTE achData[MAX_VALUE_NAME];
    DWORD cchValue = MAX_VALUE_NAME;
    DWORD cchData = MAX_VALUE_NAME;
    DWORD Type;

    // Get the class name and the value count.
    retCode = RegQueryInfoKey(
        hKey,                  // key handle
        achClass,              // buffer for class name
        &cchClassName,         // size of class string
        NULL,                  // reserved
        &cSubKeys,             // number of subkeys
        &cbMaxSubKey,          // longest subkey size
        &cchMaxClass,          // longest class string
        &cValues,              // number of values for this key
        &cchMaxValue,          // longest value name
        &cbMaxValueData,       // longest value data
        &cbSecurityDescriptor, // security descriptor
        &ftLastWriteTime);     // last write time

    // Enumerate the subkeys, until RegEnumKeyEx fails.

    // Enumerate the key values.
    GROUP_KEY *list = new GROUP_KEY [cSubKeys + 1];
    if (cSubKeys)
    {
        for(i = 0; i < cSubKeys; i++)
        {
            cbName = MAX_KEY_LENGTH;
            retCode = RegEnumKeyEx(hKey, i,
                                   list[i].subKey,
                                   &cbName,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &ftLastWriteTime);
            if (retCode == ERROR_SUCCESS)
            {
                list[i].subKeyLength = cbName;
                list[0].length += 1;
            }
        }
    }
    return list;
}

KEY_VALUE *GetKeyValue(HKEY hKey, LPCSTR lpSubKey)
{
    HKEY hTestKey;
    KEY_VALUE *list = new KEY_VALUE;
    if (RegOpenKeyExA(hKey, lpSubKey, 0, KEY_READ, &hTestKey) == ERROR_SUCCESS)
    {
        list = QueryKey(hTestKey);
    }
    RegCloseKey(hTestKey);
    return list;
}

GROUP_KEY *GetGroupKeyValue(HKEY hKey, LPCSTR lpSubKey)
{
    // get group key
    HKEY hTestKey;
    GROUP_KEY *list = new GROUP_KEY;
    if (RegOpenKeyExA(hKey, lpSubKey, 0, KEY_READ, &hTestKey) == ERROR_SUCCESS)
    {
        list = QueryGroupKey(hTestKey);
    }
    RegCloseKey(hTestKey);
    return list;
}


#endif // REG_H
