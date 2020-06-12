#ifndef SIG_H
#define SIG_H

#include <windows.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <stdio.h>
#include <tchar.h>
#include <softpub.h>
#include <string>
#include <QString>
#include "convert.h"

#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

typedef struct {
    LPWSTR lpszProgramName;
    LPWSTR lpszPublisherLink;
    LPWSTR lpszMoreInfoLink;
} SPROG_PUBLISHERINFO, *PSPROG_PUBLISHERINFO;

LPWSTR AllocateAndCopyWideString(LPCWSTR inputString);
int GetSignaturePublisher(TCHAR *path, QString *buf);
BOOL GetProgAndPublisherInfo(PCMSG_SIGNER_INFO pSignerInfo,
                             PSPROG_PUBLISHERINFO Info);
BOOL PrintCertificateInfo(PCCERT_CONTEXT pCertContext, QString *buf);
BOOL VerifyEmbeddedSignature(LPCWSTR pwszSourceFile, QString *buf);

BOOL PrintCertificateInfo(PCCERT_CONTEXT pCertContext, QString *buf)
{
    BOOL fReturn = FALSE;
    LPTSTR szName = NULL;
    DWORD dwData;
    // Get Subject name size.
    if (!(dwData = CertGetNameString(pCertContext,
                                     CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                     0,
                                     NULL,
                                     NULL,
                                     0)))
    {
        fReturn = FALSE;
        return fReturn;
    }

    // Allocate memory for subject name.
    szName = (LPTSTR)LocalAlloc(LPTR, dwData * sizeof(TCHAR));
    if (!szName)
    {
        fReturn = FALSE;
        return fReturn;
    }

    // Get subject name.
    if (!(CertGetNameString(pCertContext,
                            CERT_NAME_SIMPLE_DISPLAY_TYPE,
                            0,
                            NULL,
                            szName,
                            dwData)))
    {
        fReturn = FALSE;
        return fReturn;
    }

    // Print Subject Name.
    *buf = QString::fromWCharArray(szName);
    fReturn = TRUE;

    if (szName != NULL) LocalFree(szName);

    return fReturn;
}

LPWSTR AllocateAndCopyWideString(LPCWSTR inputString)
{
    LPWSTR outputString = NULL;

    outputString = (LPWSTR)LocalAlloc(LPTR,
        (wcslen(inputString) + 1) * sizeof(WCHAR));
    if (outputString != NULL)
    {
        lstrcpyW(outputString, inputString);
    }
    return outputString;
}

BOOL GetProgAndPublisherInfo(PCMSG_SIGNER_INFO pSignerInfo,
                             PSPROG_PUBLISHERINFO Info)
{
    BOOL fReturn = FALSE;
    PSPC_SP_OPUS_INFO OpusInfo = NULL;
    DWORD dwData;
    BOOL fResult;


    {
        // Loop through authenticated attributes and find
        // SPC_SP_OPUS_INFO_OBJID OID.
        for (DWORD n = 0; n < pSignerInfo->AuthAttrs.cAttr; n++)
        {
            if (lstrcmpA(SPC_SP_OPUS_INFO_OBJID,
                        pSignerInfo->AuthAttrs.rgAttr[n].pszObjId) == 0)
            {
                // Get Size of SPC_SP_OPUS_INFO structure.
                fResult = CryptDecodeObject(ENCODING,
                            SPC_SP_OPUS_INFO_OBJID,
                            pSignerInfo->AuthAttrs.rgAttr[n].rgValue[0].pbData,
                            pSignerInfo->AuthAttrs.rgAttr[n].rgValue[0].cbData,
                            0,
                            NULL,
                            &dwData);
                if (!fResult)
                {
                    _tprintf(_T("CryptDecodeObject failed with %x\n"),
                        GetLastError());
                    ;
                }

                // Allocate memory for SPC_SP_OPUS_INFO structure.
                OpusInfo = (PSPC_SP_OPUS_INFO)LocalAlloc(LPTR, dwData);
                if (!OpusInfo)
                {
                    _tprintf(_T("Unable to allocate memory for Publisher Info.\n"));
                    ;
                }

                // Decode and get SPC_SP_OPUS_INFO structure.
                fResult = CryptDecodeObject(ENCODING,
                            SPC_SP_OPUS_INFO_OBJID,
                            pSignerInfo->AuthAttrs.rgAttr[n].rgValue[0].pbData,
                            pSignerInfo->AuthAttrs.rgAttr[n].rgValue[0].cbData,
                            0,
                            OpusInfo,
                            &dwData);
                if (!fResult)
                {
                    _tprintf(_T("CryptDecodeObject failed with %x\n"),
                        GetLastError());
                    ;
                }

                // Fill in Program Name if present.
                if (OpusInfo->pwszProgramName)
                {
                    Info->lpszProgramName =
                        AllocateAndCopyWideString(OpusInfo->pwszProgramName);
                }
                else
                    Info->lpszProgramName = NULL;

                // Fill in Publisher Information if present.
                if (OpusInfo->pPublisherInfo)
                {

                    switch (OpusInfo->pPublisherInfo->dwLinkChoice)
                    {
                        case SPC_URL_LINK_CHOICE:
                            Info->lpszPublisherLink =
                                AllocateAndCopyWideString(OpusInfo->pPublisherInfo->pwszUrl);
                            break;

                        case SPC_FILE_LINK_CHOICE:
                            Info->lpszPublisherLink =
                                AllocateAndCopyWideString(OpusInfo->pPublisherInfo->pwszFile);
                            break;

                        default:
                            Info->lpszPublisherLink = NULL;
                            break;
                    }
                }
                else
                {
                    Info->lpszPublisherLink = NULL;
                }

                // Fill in More Info if present.
                if (OpusInfo->pMoreInfo)
                {
                    switch (OpusInfo->pMoreInfo->dwLinkChoice)
                    {
                        case SPC_URL_LINK_CHOICE:
                            Info->lpszMoreInfoLink =
                                AllocateAndCopyWideString(OpusInfo->pMoreInfo->pwszUrl);
                            break;

                        case SPC_FILE_LINK_CHOICE:
                            Info->lpszMoreInfoLink =
                                AllocateAndCopyWideString(OpusInfo->pMoreInfo->pwszFile);
                            break;

                        default:
                            Info->lpszMoreInfoLink = NULL;
                            break;
                    }
                }
                else
                {
                    Info->lpszMoreInfoLink = NULL;
                }

                fReturn = TRUE;

                break; // Break from for loop.
            } // lstrcmp SPC_SP_OPUS_INFO_OBJID
        } // for
    }

    {
        if (OpusInfo != NULL) LocalFree(OpusInfo);
    }

    return fReturn;
}

BOOL VerifyEmbeddedSignature(LPCWSTR pwszSourceFile, QString *buf)
{
    LONG lStatus;
    DWORD dwLastError;

    // Initialize the WINTRUST_FILE_INFO structure.

    WINTRUST_FILE_INFO FileData;
    memset(&FileData, 0, sizeof(FileData));
    FileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
    FileData.pcwszFilePath = pwszSourceFile;
    FileData.hFile = NULL;
    FileData.pgKnownSubject = NULL;

    GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA WinTrustData;

    // Initialize the WinVerifyTrust input data structure.

    // Default all fields to 0.
    memset(&WinTrustData, 0, sizeof(WinTrustData));

    WinTrustData.cbStruct = sizeof(WinTrustData);

    // Use default code signing EKU.
    WinTrustData.pPolicyCallbackData = NULL;

    // No data to pass to SIP.
    WinTrustData.pSIPClientData = NULL;

    // Disable WVT UI.
    WinTrustData.dwUIChoice = WTD_UI_NONE;

    // No revocation checking.
    WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;

    // Verify an embedded signature on a file.
    WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;

    // Verify action.
    WinTrustData.dwStateAction = WTD_STATEACTION_VERIFY;

    // Verification sets this value.
    WinTrustData.hWVTStateData = NULL;

    // Not used.
    WinTrustData.pwszURLReference = NULL;

    // This is not applicable if there is no UI because it changes
    // the UI to accommodate running applications instead of
    // installing applications.
    WinTrustData.dwUIContext = 0;

    // Set pFile.
    WinTrustData.pFile = &FileData;

    // WinVerifyTrust verifies signatures as specified by the GUID
    // and Wintrust_Data.
    lStatus = WinVerifyTrust(
        NULL,
        &WVTPolicyGUID,
        &WinTrustData);

    switch (lStatus)
    {
        case ERROR_SUCCESS:
            /*
            Signed file:
                - Hash that represents the subject is trusted.

                - Trusted publisher without any verification errors.

                - UI was disabled in dwUIChoice. No publisher or
                    time stamp chain errors.

                - UI was enabled in dwUIChoice and the user clicked
                    "Yes" when asked to install and run the signed
                    subject.
            */
//            wprintf_s(L"The file \"%s\" is signed and the signature "
//                L"was verified.\n",
//                pwszSourceFile);
//            wprintf_s(L"The file \"%d\" is signed and the signature "
//            L"was verified.\n",
//            WVTPolicyGUID);
            *buf = "(Verified)";
            break;

        case TRUST_E_NOSIGNATURE:
            // The file was not signed or had a signature
            // that was not valid.

            // Get the reason for no signature.
            dwLastError = GetLastError();
            if (TRUST_E_NOSIGNATURE == dwLastError ||
                    TRUST_E_SUBJECT_FORM_UNKNOWN == dwLastError ||
                    TRUST_E_PROVIDER_UNKNOWN == dwLastError)
            {
                // The file was not signed.
//                wprintf_s(L"The file \"%s\" is not signed.\n",
//                    pwszSourceFile);
                *buf = "(Not Verified)";
            }
            else
            {
                // The signature was not valid or there was an error
                // opening the file.
                wprintf_s(L"An unknown error occurred trying to "
                    L"verify the signature of the \"%s\" file.\n",
                    pwszSourceFile);
            }

            break;

        case TRUST_E_EXPLICIT_DISTRUST:
            // The hash that represents the subject or the publisher
            // is not allowed by the admin or user.
//            wprintf_s(L"The signature is present, but specifically "
//                L"disallowed.\n");
            *buf = "(Disallowed)";
            break;

        case TRUST_E_SUBJECT_NOT_TRUSTED:
            // The user clicked "No" when asked to install and run.
            wprintf_s(L"The signature is present, but not "
                L"trusted.\n");
            break;

        case CRYPT_E_SECURITY_SETTINGS:
            /*
            The hash that represents the subject or the publisher
            was not explicitly trusted by the admin and the
            admin policy has disabled user trust. No signature,
            publisher or time stamp errors.
            */
            wprintf_s(L"CRYPT_E_SECURITY_SETTINGS - The hash "
                L"representing the subject or the publisher wasn't "
                L"explicitly trusted by the admin and admin policy "
                L"has disabled user trust. No signature, publisher "
                L"or timestamp errors.\n");
            break;

        default:
            // The UI was disabled in dwUIChoice or the admin policy
            // has disabled user trust. lStatus contains the
            // publisher or time stamp chain error.
//            wprintf_s(L"Error is: 0x%x.\n",
//                lStatus);
            break;
    }

    // Any hWVTStateData must be released by a call with close.
    WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;

    lStatus = WinVerifyTrust(
        NULL,
        &WVTPolicyGUID,
        &WinTrustData);

    return true;
}

int GetSignaturePublisher(TCHAR *path, QString *buf)
{
    WCHAR szFileName[MAX_PATH];
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    BOOL fResult;
    DWORD dwEncoding, dwContentType, dwFormatType;
    PCMSG_SIGNER_INFO pSignerInfo = NULL;
    PCMSG_SIGNER_INFO pCounterSignerInfo = NULL;
    DWORD dwSignerInfo;
    CERT_INFO CertInfo;
    SPROG_PUBLISHERINFO ProgPubInfo;

    ZeroMemory(&ProgPubInfo, sizeof(ProgPubInfo));

#ifdef UNICODE
        lstrcpynW(szFileName, path, MAX_PATH);
#endif
        // Get message handle and store handle from the signed file.
        fResult = CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                                   szFileName,
                                   CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                                   CERT_QUERY_FORMAT_FLAG_BINARY,
                                   0,
                                   &dwEncoding,
                                   &dwContentType,
                                   &dwFormatType,
                                   &hStore,
                                   &hMsg,
                                   NULL);
        if (!fResult)
        {
            return 1;
        }

        // Get signer information size.
        fResult = CryptMsgGetParam(hMsg,
                                   CMSG_SIGNER_INFO_PARAM,
                                   0,
                                   NULL,
                                   &dwSignerInfo);
        if (!fResult)
        {
            return 1;
        }

        // Allocate memory for signer information.
        pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);
        if (!pSignerInfo)
        {
            return 1;
        }

        // Get Signer Information.
        fResult = CryptMsgGetParam(hMsg,
                                   CMSG_SIGNER_INFO_PARAM,
                                   0,
                                   (PVOID)pSignerInfo,
                                   &dwSignerInfo);
        if (!fResult)
        {
            return 1;
        }

        // Get program name and publisher information from
        // signer info structure.
        GetProgAndPublisherInfo(pSignerInfo, &ProgPubInfo);

        // Search for the signer certificate in the temporary
        // certificate store.
        CertInfo.Issuer = pSignerInfo->Issuer;
        CertInfo.SerialNumber = pSignerInfo->SerialNumber;

        pCertContext = CertFindCertificateInStore(hStore,
                                                  ENCODING,
                                                  0,
                                                  CERT_FIND_SUBJECT_CERT,
                                                  (PVOID)&CertInfo,
                                                  NULL);
        if (!pCertContext)
        {
            return 1;
        }

        // Print Signer certificate information.
        PrintCertificateInfo(pCertContext, buf);

        // Clean up.
    if (ProgPubInfo.lpszProgramName != NULL)
        LocalFree(ProgPubInfo.lpszProgramName);
    if (ProgPubInfo.lpszPublisherLink != NULL)
        LocalFree(ProgPubInfo.lpszPublisherLink);
    if (ProgPubInfo.lpszMoreInfoLink != NULL)
        LocalFree(ProgPubInfo.lpszMoreInfoLink);

    if (pSignerInfo != NULL) LocalFree(pSignerInfo);
    if (pCounterSignerInfo != NULL) LocalFree(pCounterSignerInfo);
    if (pCertContext != NULL) CertFreeCertificateContext(pCertContext);
    if (hStore != NULL) CertCloseStore(hStore, 0);
    if (hMsg != NULL) CryptMsgClose(hMsg);
    return 0;
}

#endif // SIG_H
