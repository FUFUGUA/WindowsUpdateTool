#include "ImGui/ImGui.h"
#include "ImGui/imgui_impl_dx9.h"
#include "ImGui/imgui_impl_win32.h"

#include "menu.h"
#include "globals.h"

#include <iostream>
#include <windows.h>
#include <TlHelp32.h>
#include <tchar.h>

std::wstring name0 = L"wuauclt.exe";
std::wstring name1 = L"UsoClient.exe";
std::wstring name2 = L"wuagent.exe";
std::wstring name3 = L"qmgr.exe";
std::wstring name4 = L"svchost.exe";
const char* infoText = "";
bool svc = false;

class initWindow {
public:
    const char* window_title = "Windows update process killer -On boot, etc...";
    ImVec2 window_size{ 400, 400 };
    
    DWORD window_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize;
} iw;

void TProcess(const std::wstring& processName) {
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapShot == INVALID_HANDLE_VALUE) {
        return;
    }

    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(hSnapShot, &entry)) {
        CloseHandle(hSnapShot);
        return;
    }

    do {
        if (_wcsicmp(entry.szExeFile, processName.c_str()) == 0) {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
            if (hProcess != NULL) {
                if (!TerminateProcess(hProcess, 0)) {
                }
                else {
                }
                CloseHandle(hProcess);
            }
            else {
            }
        }
    } while (Process32NextW(hSnapShot, &entry));

    CloseHandle(hSnapShot);
}
bool DisableWindowsUpdateService() {
    SC_HANDLE scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (scmHandle == NULL) {
        std::cerr << "Failed to open Service Control Manager." << std::endl;
        return false;
    }

    SC_HANDLE serviceHandle = OpenService(scmHandle, L"wuauserv", SERVICE_STOP | SERVICE_CHANGE_CONFIG);
    if (serviceHandle == NULL) {
        CloseServiceHandle(scmHandle);
        return false;
    }

    // Stop the service
    if (!ControlService(serviceHandle, SERVICE_CONTROL_STOP, new SERVICE_STATUS)) {
        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(scmHandle);
        return false;
    }

    // Change the startup type to Disabled
    if (!ChangeServiceConfig(serviceHandle, SERVICE_NO_CHANGE, SERVICE_DISABLED, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL)) {
        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(scmHandle);
        return false;
    }

    infoText = "Disabled the service succesfully";

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);
    return true;
}
bool AddToStartup(const TCHAR* keyName, const TCHAR* appPath) {
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_SET_VALUE, &hKey);

    if (result != ERROR_SUCCESS) {
        return false;
    }

    result = RegSetValueEx(hKey, keyName, 0, REG_SZ, (const BYTE*)appPath, (lstrlen(appPath) + 1) * sizeof(TCHAR));

    RegCloseKey(hKey);

    return result == ERROR_SUCCESS;
}


bool RemoveFromStartup(const TCHAR* keyName) {
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_SET_VALUE, &hKey);

    if (result != ERROR_SUCCESS) {
        return false;
    }

    result = RegDeleteValue(hKey, keyName);

    RegCloseKey(hKey);

    return result == ERROR_SUCCESS;
}


bool GetExecutablePath(TCHAR* path, DWORD size) {
    DWORD result = GetModuleFileName(NULL, path, size);
    return (result != 0 && result != size);
}

bool DisableWindowsUpdatePolicy() {
    // Modify the registry key/value corresponding to the Windows Update policy
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate", 0, KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS) {
        return false;
    }

    // Set the value to disable Windows Update
    DWORD value = 1; // Set to 1 to disable, 0 to enable
    result = RegSetValueEx(hKey, L"AUOptions", 0, REG_DWORD, reinterpret_cast<BYTE*>(&value), sizeof(DWORD));
    if (result != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }


    RegCloseKey(hKey);
    return true;
}


void menu::render()
{

    const TCHAR* appName = _T("UpdateTerminator");
    TCHAR appPath[MAX_PATH];
    if (globals.active)
    {
        ImGui::SetNextWindowSize(iw.window_size);
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGui::Begin(iw.window_title, &globals.active, iw.window_flags);
        bool boot = true;
        if (boot) {

            TProcess(name0);
            TProcess(name1);
            TProcess(name2);
            TProcess(name3);
            if (svc) {
                TProcess(name4);
            }
            boot = false;
        }
        {
            ImGui::Checkbox("Kill svchost too", &svc);
                    
            
            if (ImGui::Button("Kill Windows update processes")) {
     

                TProcess(name0);
                TProcess(name1);
                TProcess(name2);
                TProcess(name3);
                if (svc) {
                    TProcess(name4);
                }
            }
            if (ImGui::Button("Launch the program on boot")) {
                if (GetExecutablePath(appPath, MAX_PATH)) {
                    if (AddToStartup(appName, appPath)) {
                        infoText = "Succesfully added to startup.";
                   }
                    else {
                        infoText = "Something went wrong on adding the program as a startup one.";
                    }
                }
            }
            if (ImGui::Button("Remove the launch on boot")) {

                //The GetExecutablePath call on the if-statement is probably just useless, but still added it. - softCare
                if (GetExecutablePath(appPath, MAX_PATH)) {
                    if (RemoveFromStartup(appName)) {
                        infoText = "Succesfully removed from startup.";
                    }
                    else {
                        infoText = "Something went wrong on removing";
                    }
                }
            }
            if (ImGui::Button("Disable Windows Update service")) {
                if (DisableWindowsUpdateService()) {
                    infoText = "Disabled succesfully";
                }
                else {
                    infoText = "Something went wrong on disabling the service.";
                }
            }
            if (ImGui::Button("Disable Windows Update through policy")) {
                if (DisableWindowsUpdatePolicy()) {
                    infoText = "Succesfully disabled through policy";
                }
                else {
                    infoText = "Something went wrong on disabling it through policy"; 
                }

            }
            ImGui::TextWrapped(infoText);
        }
        ImGui::End();
    }
    else
    {
        exit(0);
    }
}
