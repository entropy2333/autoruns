#ifndef FILE_H
#define FILE_H

#include <tchar.h>
#include <windows.h>
#include <convert.h>
#include <QString>
#include <Shlwapi.h>

BOOL GetFileVersionString(LPCTSTR pFileName, LPCTSTR pName /* = NULL */, LPTSTR ptBuf, UINT lenBuf);
void GetFileDescription(QString filePath, QString *fileDescription);
void GetDllDescription(QString path, QString *fileDescription);

BOOL GetFileVersionString(LPCTSTR pFileName, LPCTSTR pName /* = NULL */, LPTSTR ptBuf, UINT lenBuf)
{
    DWORD   dwDummyHandle = 0; // will always be set to zero
    DWORD   dwLen = 0;
    BYTE*    pVersionInfo = NULL;
    BOOL    bRetVal;

    VS_FIXEDFILEINFO    FileVersion;

    HMODULE        hVerDll;
    hVerDll = LoadLibrary(_T("VERSION.dll"));
    if (hVerDll == NULL)
        return FALSE;

#ifdef _UNICODE
    typedef DWORD(WINAPI *Fun_GetFileVersionInfoSize)(LPCTSTR, DWORD *);
    typedef BOOL(WINAPI *Fun_GetFileVersionInfo)(LPCTSTR, DWORD, DWORD, LPVOID);
    typedef BOOL(WINAPI *Fun_VerQueryValue)(LPCVOID, LPCTSTR, LPVOID, PUINT);
#else
    typedef DWORD(WINAPI *Fun_GetFileVersionInfoSize)(LPCSTR, DWORD *);
    typedef BOOL(WINAPI *Fun_GetFileVersionInfo)(LPCSTR, DWORD, DWORD, LPVOID);
    typedef BOOL(WINAPI *Fun_VerQueryValue)(LPCVOID, LPCSTR, LPVOID, PUINT);
#endif

    Fun_GetFileVersionInfoSize        pGetFileVersionInfoSize;
    Fun_GetFileVersionInfo            pGetFileVersionInfo;
    Fun_VerQueryValue                pVerQueryValue;

#ifdef _UNICODE
    pGetFileVersionInfoSize = (Fun_GetFileVersionInfoSize)::GetProcAddress(hVerDll, "GetFileVersionInfoSizeW");
    pGetFileVersionInfo = (Fun_GetFileVersionInfo)::GetProcAddress(hVerDll, "GetFileVersionInfoW");
    pVerQueryValue = (Fun_VerQueryValue)::GetProcAddress(hVerDll, "VerQueryValueW");
#else
    pGetFileVersionInfoSize = (Fun_GetFileVersionInfoSize)::GetProcAddress(hVerDll, "GetFileVersionInfoSizeA");
    pGetFileVersionInfo = (Fun_GetFileVersionInfo)::GetProcAddress(hVerDll, "GetFileVersionInfoA");
    pVerQueryValue = (Fun_VerQueryValue)::GetProcAddress(hVerDll, "VerQueryValueA");
#endif

    struct TRANSLATION {
        WORD langID;            // language ID
        WORD charset;            // character set (code page)
    } Translation;

    Translation.langID = 0x0409;    //
    Translation.charset = 1252;        // default = ANSI code page

    dwLen = pGetFileVersionInfoSize(pFileName, &dwDummyHandle);
    if (dwLen == 0)
    {
        bRetVal = FALSE;
        goto End;
    }

    pVersionInfo = new BYTE[dwLen]; // allocate version info
    bRetVal = pGetFileVersionInfo(pFileName, 0, dwLen, pVersionInfo);
    if (bRetVal == FALSE)
    {
        goto End;
    }

    VOID    * pVI;
    UINT    uLen;

    bRetVal = pVerQueryValue(pVersionInfo, _T("\\"), &pVI, &uLen);
    if (bRetVal == FALSE)
    {
        goto End;
    }

    memcpy(&FileVersion, pVI, sizeof(VS_FIXEDFILEINFO));

    bRetVal = pVerQueryValue(pVersionInfo, _T("\\VarFileInfo\\Translation"),
        &pVI, &uLen);
    if (bRetVal && uLen >= 4)
    {
        memcpy(&Translation, pVI, sizeof(TRANSLATION));
    }
    else
    {
        bRetVal = FALSE;
        goto End;
    }

    //  BREAKIF(FileVersion.dwSignature != VS_FFI_SIGNATURE);
    if (FileVersion.dwSignature != VS_FFI_SIGNATURE)
    {
        bRetVal = FALSE;
        goto End;
    }

    VOID        *pVal;
    UINT        iLenVal;

    if (pName == NULL)
    {
        _stprintf_s(ptBuf, lenBuf, _T("%d.%d.%d.%d"),
            HIWORD(FileVersion.dwFileVersionMS), LOWORD(FileVersion.dwFileVersionMS),
            HIWORD(FileVersion.dwFileVersionLS), LOWORD(FileVersion.dwFileVersionLS));
    }
    else
    {
        TCHAR    szQuery[1024];
        _stprintf_s(szQuery, 1024, _T("\\StringFileInfo\\%04X%04X\\%s"),
            Translation.langID, Translation.charset, pName);

        bRetVal = pVerQueryValue(pVersionInfo, szQuery, &pVal, &iLenVal);
        if (bRetVal)
        {
            _stprintf_s(ptBuf, lenBuf, _T("%s"), (TCHAR*)pVal);
        }
        else
        {
            _stprintf_s(ptBuf, lenBuf, _T("%s"), _T(""));
        }
    }

End:
    FreeLibrary(hVerDll);
    hVerDll = NULL;
    delete[] pVersionInfo;
    pVersionInfo = NULL;

    return bRetVal;
}

void GetFileDescription(QString filePath, QString *fileDescription)
{
    TCHAR* ptszStr = new TCHAR[1024];

    BOOL bRet = GetFileVersionString(filePath.toStdWString().c_str(),
        _T("FileDescription"), ptszStr, 1024);

    if (bRet)
        *fileDescription = QString::fromWCharArray(ptszStr);
    else
        *fileDescription = "";

    delete[] ptszStr;
}

void GetDllDescription(QString path, QString *fileDescription)
{
    TCHAR* ptszStr = new TCHAR[1024];
    if (SHLoadIndirectString(path.toStdWString().c_str(), ptszStr, 1024, NULL) == S_OK)
        *fileDescription = QString::fromWCharArray(ptszStr);
    else
        *fileDescription = "";
    delete[] ptszStr;
}

#endif // FILE_H
