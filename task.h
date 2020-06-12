#ifndef TASK_H
#define TASK_H

/********************************************************************
 This sample enumerates through the tasks on the local computer and
 displays their name and state.
********************************************************************/

#define _WIN32_DCOM

#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <comdef.h>
//  Include the task header file.
#include <taskschd.h>
#include <combaseapi.h>

using namespace std;

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

struct TASK
{
    char* taskName;
    char* folderName;
    char* imagePath;
    int length = 0;
};

BOOL WalkFolders(ITaskFolder* rootFolder, HRESULT hr, TASK *list);
BOOL GetTasks(ITaskFolder* rootFolder, HRESULT hr, TASK *list);
void GetAllTasks(TASK *list);

void GetAllTasks(TASK *list)
{
    //  Initialize COM.
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    //  Set general COM security levels.
    hr = CoInitializeSecurity(
                            NULL,
                            -1,
                            NULL,
                            NULL,
                            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                            RPC_C_IMP_LEVEL_IMPERSONATE,
                            NULL,
                            0,
                            NULL);

    if (FAILED(hr)) // CoInitializeSecurity failed
    {
        CoUninitialize();
        return;
    }

    ITaskService* pService = NULL;

    hr = CoCreateInstance(CLSID_TaskScheduler,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITaskService,
        (void**)&pService);

    if (FAILED(hr)) // Failed to CoCreate an instance of the TaskService class
    {
        CoUninitialize();
        return;
    }

    //  Connect to the task service.
    hr = pService->Connect(_variant_t(), _variant_t(),
        _variant_t(), _variant_t());

    if (FAILED(hr)) // ITaskService::Connect failed
    {
        pService->Release();
        CoUninitialize();
        return;
    }

    //  Get the pointer to the root task folder.
    ITaskFolder* pRootFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);

    if (FAILED(hr)) // Cannot get Root Folder pointer
    {
        return;
    }

    WalkFolders(pRootFolder, hr, list);

    pRootFolder->Release();

    pService->Release();
    CoUninitialize();
}

// recursive search sub-folders
BOOL WalkFolders(ITaskFolder* rootFolder, HRESULT hr, TASK *list)
{
    ITaskFolderCollection* pFolders = NULL;
    hr = rootFolder->GetFolders(0, &pFolders);
    GetTasks(rootFolder, hr, list);
    if (FAILED(hr))
    {
        return FALSE;
    }

    LONG numFolders = 0;
    hr = pFolders->get_Count(&numFolders);

    if (numFolders != 0) {
        for (LONG i = 0; i < numFolders; i++)
        {
            ITaskFolder* pRootFolder = NULL;
            hr = pFolders->get_Item(_variant_t(i + 1), &pRootFolder);
            if (SUCCEEDED(hr))
            {
                BSTR name = NULL;
                hr = pRootFolder->get_Name(&name);
                if (FAILED(hr)) // fail to get folder name
                {
                    return FALSE;
                }
                SysFreeString(name);
//                list[0].length++;
                WalkFolders(pRootFolder, hr, list);
            }
        }
        pFolders->Release();
    }
    return TRUE;
}

// get the registered tasks in the folder
BOOL GetTasks(ITaskFolder* rootFolder, HRESULT hr, TASK *list) {
    IRegisteredTaskCollection* pTaskCollection = NULL;
    hr = rootFolder->GetTasks(NULL, &pTaskCollection);

    if (FAILED(hr))
    {
        return FALSE;
    }

    LONG numTasks = 0;
    hr = pTaskCollection->get_Count(&numTasks);

    if (numTasks == 0)
    {
        pTaskCollection->Release();
        return FALSE;
    }

    TASK_STATE taskState;
    int len = list[0].length;
    for (LONG i = 0; i < numTasks; i++)
    {
        IRegisteredTask* pRegisteredTask = NULL;
        hr = pTaskCollection->get_Item(_variant_t(i + 1), &pRegisteredTask);

        if (SUCCEEDED(hr))
        {
            BSTR taskName = NULL;
            hr = pRegisteredTask->get_Name(&taskName);
            if (SUCCEEDED(hr))
            {
                list[i + len + 1].taskName = _com_util::ConvertBSTRToString(taskName);
                list[0].length += 1;
                SysFreeString(taskName);

                hr = pRegisteredTask->get_State(&taskState);

                ITaskDefinition* taskDefination = NULL;
                hr = pRegisteredTask->get_Definition(&taskDefination);
                if (FAILED(hr))
                {
                    return FALSE;
                }

                IActionCollection* taskActions = NULL;
                hr = taskDefination->get_Actions(&taskActions);
                if (FAILED(hr))
                {
                    return FALSE;
                }
                taskDefination->Release();

                IAction* action = NULL;
                hr = taskActions->get_Item(1, &action);
                if (FAILED(hr))
                {
                    return FALSE;
                }
                taskActions->Release();

                IExecAction* execAction = NULL;
                hr = action->QueryInterface(IID_IExecAction, (void**)&execAction);
                if (FAILED(hr))
                {
                    return FALSE;
                }
                action->Release();

                BSTR imagePath = NULL;
                hr = execAction->get_Path(&imagePath);
                if (SUCCEEDED(hr))
                {
                    list[i + len + 1].imagePath = _com_util::ConvertBSTRToString(imagePath);
                }
                execAction->Release();
            }
            pRegisteredTask->Release();
        }
    }
    pTaskCollection->Release();
    return TRUE;
}

#endif // TASK_H
