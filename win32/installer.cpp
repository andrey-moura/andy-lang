#include <windows.h>
#include <winhttp.h>
#include <commctrl.h>

#include "resource.h"

#include <filesystem>
#include <fstream>

HWND hProgress;

#include <windows.h>
#include <shellapi.h>

bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0, &adminGroup)) {
        if (!CheckTokenMembership(NULL, adminGroup, &isAdmin)) {
            isAdmin = FALSE;
        }
        FreeSid(adminGroup);
    }
    return isAdmin;
}

void RelaunchAsAdmin(HINSTANCE hInstance) {
    char exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };
    sei.lpVerb = "runas";  // Solicita privilégios elevados
    sei.lpFile = exePath;  // Caminho do executável
    sei.nShow = SW_NORMAL;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    if (ShellExecuteEx(&sei)) {
        ExitProcess(0); // Fecha o processo atual após iniciar a nova instância
    }
}

INT_PTR CALLBACK LoadingDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_INITDIALOG:
            hProgress = GetDlgItem(hDlg, IDC_PROGRESS);
            SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); 
            SendMessage(hProgress, PBM_SETSTEP, 1, 0);
            // Center
            RECT rc, rcDlg, rcOwner;
            GetWindowRect(GetDesktopWindow(), &rcOwner);
            GetWindowRect(hDlg, &rcDlg);
            CopyRect(&rc, &rcOwner);
            OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
            OffsetRect(&rc, -rc.left, -rc.top);
            OffsetRect(&rc, (rc.right - rc.left - rcDlg.right) / 2, (rc.bottom - rc.top - rcDlg.bottom) / 2);
            SetWindowPos(hDlg, HWND_TOP, rc.left, rc.top, 0, 0, SWP_NOSIZE);

            return TRUE;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            return TRUE;
    }
    return FALSE;
}

bool SaveAndyExecutable(std::filesystem::path path) {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    HRSRC hRes = FindResource(hInstance, MAKEINTRESOURCE(IDR_BINARY_DATA), RT_RCDATA);
    if (!hRes) return false;

    HGLOBAL hData = LoadResource(hInstance, hRes);
    if (!hData) return false;

    DWORD size = SizeofResource(hInstance, hRes);
    void* pData = LockResource(hData);

    SendMessage(hProgress, PBM_SETPOS, 50, 0);

    std::ofstream stream(path, std::ios::binary);
    stream.write((char*)pData, size);
    stream.close();

    SendMessage(hProgress, PBM_SETPOS, 90, 0);

    return true;
}

bool AddPathToSystem(const std::string& newPath) {
    HKEY hKey;
    const char* key = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key, 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        char currentPath[32767];
        DWORD size = sizeof(currentPath);
        
        if (RegQueryValueExA(hKey, "Path", nullptr, nullptr, (LPBYTE)currentPath, &size) == ERROR_SUCCESS) {
            std::string updatedPath = std::string(currentPath) + ";" + newPath;
            if (RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ, (const BYTE*)updatedPath.c_str(), updatedPath.size() + 1) == ERROR_SUCCESS) {
                return true;
            } else {
                return false;
            }
        }
        RegCloseKey(hKey);
    } else {
        return false;
    }

    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
}

// Função principal
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HWND hDlg = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LOADING_DIALOG), nullptr, LoadingDlgProc);
    ShowWindow(hDlg, SW_SHOW);

    //for (int i = 0; i <= 100; i += 10) {
        //SendMessage(hProgress, PBM_SETPOS, i, 0);
        //Sleep(500);  // Simula um processo demorado
    //}

    std::filesystem::path install_dir = std::filesystem::path(std::string(1, std::filesystem::current_path().string()[0]) + ":\\") / "Program Files" / "Andy";
    std::filesystem::path bin = install_dir / "bin";

    if (!std::filesystem::exists(bin)) {
        std::filesystem::create_directories(bin);
    }

    std::filesystem::path dest = bin / "andy.exe";

    SendMessage(hProgress, PBM_SETPOS, 5, 0);

    if(!SaveAndyExecutable(dest)) {
        MessageBox(hDlg, "Failed to save Andy executable", "Error", MB_ICONERROR);
        return 0;
    }

    if(!AddPathToSystem(bin.string().c_str())) {
        MessageBox(hDlg, "Failed to add Andy to system path", "Error", MB_ICONERROR);
        return 0;
    }

    SendMessage(hProgress, PBM_SETPOS, 100, 0);

    EndDialog(hDlg, 0);

    int response = MessageBoxA(NULL, "Andy has been installed successfully. Would you like to restart your computer now?", "Success", MB_YESNO | MB_ICONQUESTION);
    
    if (response == IDYES) {
        system("shutdown /r /t 0");
    }

    return 0;
}