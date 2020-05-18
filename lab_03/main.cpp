#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <string>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

using namespace std;

typedef struct KEY_DATA {
    TCHAR    achKey[MAX_KEY_LENGTH];            // buffer for subkey name
    DWORD    cbName;                            // size of name string 
    TCHAR    achClass[MAX_PATH] = TEXT("");     // buffer for class name 
    DWORD    cchClassName = MAX_PATH;           // size of class string 
    DWORD    cSubKeys = 0;                      // number of subkeys 
    DWORD    cbMaxSubKey;                       // longest subkey size 
    DWORD    cchMaxClass;                       // longest class string 
    DWORD    cValues;                           // number of values for key 
    DWORD    cchMaxValue;                       // longest value name 
    DWORD    cbMaxValueData;                    // longest value data 
    DWORD    cbSecurityDescriptor;              // size of security descriptor 
    FILETIME ftLastWriteTime;                   // last write time 
} KEY_DATA, *PKEY_DATA;

bool GetKey(HKEY** hKey, LPSTR keyPath, REGSAM samDesired);
bool GetKeyData(HKEY key, KEY_DATA * keyData);
void ListSubkeys(HKEY hKey);
void ListParameters(HKEY hKey);
bool FindString(HKEY hKey, string search_s, LPSTR keyPath);
bool SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
void SaveToFile(HKEY hKey);

int main()
{
    HKEY hKey;
    PHKEY phKey = &hKey;
    CHAR keyPath[MAX_PATH];

    enum program_mode
    {
        SUBKEYS,
        KEYPARAMS,
        SUBSEARCH,
        TOFILE,
        EXIT,
    };

    int mode;
    bool isExit = false;

    do
    {
        cout << "Main menu. Choose operation:" << endl
            << "0 - print a list of key subkeys" << endl
            << "1 - print a list of keys parameters with values and types" << endl
            << "2 - search for a string in key names, param names and its values" << endl
            << "3 - save a key to a file" << endl
            << "4 - exit" << endl;
        cin >> mode;

        switch (mode)
        {
            case SUBKEYS:
            {
                if (GetKey(&phKey, keyPath, KEY_READ)) {
                    ListSubkeys(hKey);
                }
                else
                {
                    cout << "Key does not exist." << endl;
                }

                break;
            }
            case KEYPARAMS:
            {
                if (GetKey(&phKey, keyPath, KEY_READ)) {
                    ListParameters(hKey);
                }
                else
                {
                    cout << "Key does not exist." << endl;
                }

                break;
            }
            case SUBSEARCH:
            {
                if (GetKey(&phKey, keyPath, KEY_READ))
                {
                    string req_str = "";
                    cout << "Input string: ";
                    getline(cin, req_str);

                    FindString(hKey, req_str, keyPath);
                }
                else
                {
                    cout << "Key does not exist." << endl;
                }

                break;
            }
            case TOFILE:
            {
                HANDLE processToken;

                //get access token 
                if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &processToken))
                {
                    cout << "Cant get access rights " << endl;
                }
                else {
                    // set privilege
                    if (SetPrivilege(processToken, SE_BACKUP_NAME, TRUE))
                    {
                        if (GetKey(&phKey, keyPath, KEY_READ)) {
                            SaveToFile(hKey);
                        }
                        else
                        {
                            cout << "Key does not exist." << endl;
                        }
                    }
                }

                break;
            }
            case EXIT:
            {
                isExit = true;
                break;
            }
            default:
            {
                cout << "Invalid operation." << endl;
                break;
            }
        }

    }
    while (!isExit);
}

bool GetKey(HKEY** hKey, LPSTR keyPath, REGSAM samDesired)
{
    HKEY rootKey;

    enum root_keys
    {
        CLASSES_ROOT,
        CURRENT_USER,
        LOCAL_MACHINE,
        USERS,
        CURRENT_CONFIG,
        PERFORMANCE_DATA
    };

    int mode;

    memset(keyPath, '\0', sizeof(keyPath));

    cout << "Choose a root key:" << endl
        << "0 - HKEY_CLASSES_ROOT" << endl
        << "1 - HKEY_CURRENT_USER" << endl
        << "2 - HKEY_LOCAL_MACHINE" << endl
        << "3 - HKEY_USERS" << endl
        << "4 - HKEY_CURRENT_CONFIG" << endl
        << "5 - HKEY_PERFORMANCE_DATA" << endl
        << "or any other key to return." << endl;
    cin >> mode;

    switch (mode)
    {
        case CLASSES_ROOT:
        {
            rootKey = HKEY_CLASSES_ROOT;
            strcpy(keyPath, "HKEY_CLASSES_ROOT\\");
            break;
        }
        case CURRENT_USER:
        {
            rootKey = HKEY_CURRENT_USER;
            strcpy(keyPath, "HKEY_CURRENT_USER\\");
            break;
        }
        case LOCAL_MACHINE:
        {
            rootKey = HKEY_LOCAL_MACHINE;
            strcpy(keyPath, "HKEY_LOCAL_MACHINE\\");
            break;
        }
        case USERS:
        {
            rootKey = HKEY_USERS;
            strcpy(keyPath, "HKEY_USERS\\");
            break;
        }
        case CURRENT_CONFIG:
        {
            rootKey = HKEY_CURRENT_CONFIG;
            strcpy(keyPath, "HKEY_CURRENT_CONFIG\\");
            break;
        }
        case PERFORMANCE_DATA:
        {
            rootKey = HKEY_PERFORMANCE_DATA;
            strcpy(keyPath, "HKEY_PERFORMANCE_DATA\\");
            break;
        }
        default:
        {
            return false;
        }
    }

    string destKey = "";

    cout << "Enter required subkey path (to list root key subkeys press ENTER): ";
    getline(cin, destKey);
    getline(cin, destKey);

    LPSTR destination_key = const_cast<char *>(destKey.c_str());
    if (RegOpenKeyExA(rootKey, destination_key, 0, samDesired, *hKey) == ERROR_SUCCESS)
    {
        strcat(keyPath, destination_key);
        return true;
    }

    return false;
}

bool GetKeyData(HKEY key, KEY_DATA * keyData)
{
    DWORD retCode = RegQueryInfoKey(
        key,
        (*keyData).achClass,
        &(*keyData).cchClassName,
        NULL,
        &(*keyData).cSubKeys,
        &(*keyData).cbMaxSubKey,
        &(*keyData).cchMaxClass,
        &(*keyData).cValues,
        &(*keyData).cchMaxValue,
        &(*keyData).cbMaxValueData,
        &(*keyData).cbSecurityDescriptor,
        &(*keyData).ftLastWriteTime
    );

    return retCode == ERROR_SUCCESS;
}

void ListSubkeys(HKEY hKey)
{
    KEY_DATA keyData = {};
    GetKeyData(hKey, &keyData);

    if (keyData.cSubKeys)
    {
        cout << "Number of subkeys: " << keyData.cSubKeys << endl;

        for (size_t i = 0; i < keyData.cSubKeys; i++)
        {
            keyData.cbName = MAX_KEY_LENGTH;

            DWORD retCode = RegEnumKeyEx(
                hKey,
                i,
                keyData.achKey,
                &keyData.cbName,
                NULL,
                NULL,
                NULL,
                NULL
            );

            if (retCode == ERROR_SUCCESS)
            {
                _tprintf(TEXT("(%d) %s\n"), i + 1, keyData.achKey);
            }
        }
    }
    else
    {
        cout << "No subkeys found." << endl;
    }

    RegCloseKey(hKey);
}

void ListParameters(HKEY hKey)
{
    DWORD retCode = ERROR_SUCCESS;
    DWORD i = 0;
    CHAR achValue[MAX_VALUE_NAME];
    DWORD cchValue = MAX_VALUE_NAME;
    DWORD dwType = 0;
    LPBYTE lpData = NULL;
    DWORD dwData = 0;

    KEY_DATA keyData = {};
    GetKeyData(hKey, &keyData);

    if (keyData.cValues)
    {
        printf("\nNumber of values: %d\n", keyData.cValues);

        for (i = 0, retCode = ERROR_SUCCESS; i < keyData.cValues; i++)
        {
            cchValue = MAX_VALUE_NAME;
            achValue[0] = '\0';

            //get data size for allocation
            retCode = RegEnumValueA(hKey, i, achValue, &cchValue, NULL, NULL, NULL, &dwData);
            lpData = (LPBYTE)malloc((dwData + 1) * sizeof(BYTE));
            cchValue = keyData.cchMaxValue + 1;

            retCode = RegEnumValueA(
                hKey,
                i,
                achValue,
                &cchValue,
                NULL,
                &dwType,
                lpData,
                &dwData
            );

            if (retCode == ERROR_SUCCESS)
            {
                cout << "(" << i + 1 << ") " << achValue << endl;

                switch (dwType)
                {
                    case REG_BINARY:
                    {
                        printf("Value type: REG_BINARY\nValue data: binary\n");
                        break;
                    }
                    case REG_DWORD:
                    {
                        printf("Value type: REG_DWORD\nValue data: %#x|%u\n", *(DWORD*)(lpData), *(DWORD*)(lpData));
                        break;
                    }
                    case REG_EXPAND_SZ:
                    {
                        printf("Value type: REG_EXPAND_SZ\nValue data: %s\n", lpData);
                        break;
                    }
                    case REG_LINK:
                    {
                        LPCWCHAR data = reinterpret_cast<wchar_t*>(lpData);
                        wprintf(L"Value type: REG_LINK\nValue data: %ws\n", data);
                        break;
                    }
                    case REG_SZ:
                    {
                        printf("Value type: REG_SZ\nValue data: %s\n", lpData);
                        break;
                    }
                    case REG_NONE:
                    {
                        printf("Value type: REG_NONE\nValue data: %x\n", *(DWORD*)(lpData));
                        break;
                    }
                    default:
                    {
                        printf("Value type: undefined\nValue data: %x\n", *(DWORD*)(lpData));
                        break;
                    }
                }
            }

            free(lpData);
        }
    }
    else
    {
        cout << "No parameters found." << endl;
    }

    RegCloseKey(hKey);
}

bool FindString(HKEY hKey, string searchStr, LPSTR keyPath)
{
    LPSTR request_string = const_cast<char*>(searchStr.c_str());
    KEY_DATA keyData = {};
    DWORD retCode = ERROR_SUCCESS;
    LPSTR subkeyPath;

    if (!GetKeyData(hKey, &keyData))
    {
        return false;
    }

    if (keyData.cSubKeys)
    {
        for (size_t i = 0; i < keyData.cSubKeys; i++)
        {
            keyData.cbName = MAX_KEY_LENGTH;

            retCode = RegEnumKeyEx(
                hKey,
                i,
                keyData.achKey,
                &keyData.cbName,
                NULL,
                NULL,
                NULL,
                NULL
            );

            if (retCode == ERROR_SUCCESS)
            {
                wstring wideStr = wstring(searchStr.begin(), searchStr.end());
                if (_tcscmp(keyData.achKey, wideStr.c_str()) == 0)
                {
                    cout << "Found in subkey name: " << request_string << endl;
                }

                subkeyPath = (LPSTR)malloc(MAX_VALUE_NAME * sizeof(TCHAR));

                wstring s = keyData.achKey;
                string b = string(s.begin(), s.end());

                strcpy(subkeyPath, const_cast<char*>(b.c_str()));
                strcat(subkeyPath, "\\");
                strcat(subkeyPath, request_string);

                HKEY subKey = {};

                if (RegOpenKeyEx(hKey, keyData.achKey, 0, KEY_READ, &subKey) == ERROR_SUCCESS)
                {
                    FindString(subKey, request_string, subkeyPath);
                }
            }
        }
    }

    if (keyData.cValues)
    {
        CHAR achValue[MAX_VALUE_NAME];
        DWORD cchValue = MAX_VALUE_NAME;
        DWORD dwType = 0;
        LPBYTE lpData = NULL;
        DWORD dwData = 0;

        for (int i = 0; i < keyData.cValues; i++)
        {
            cchValue = MAX_VALUE_NAME;
            achValue[0] = '\0';

            //get data size for allocation
            retCode = RegEnumValueA(hKey, i, achValue, &cchValue, NULL, NULL, NULL, &dwData);
            lpData = (LPBYTE)malloc((dwData + 1) * sizeof(BYTE));
            cchValue = keyData.cchMaxValue + 1;

            retCode = RegEnumValueA(
                hKey,
                i,
                achValue,
                &cchValue,
                NULL,
                &dwType,
                lpData,
                &dwData
            );

            if (retCode == ERROR_SUCCESS)
            {
                if (_strcmpi(achValue, request_string) == 0)
                {
                    cout << "Found in value name: " << keyPath << "; " << achValue << endl;
                }
                if (((dwType & REG_EXPAND_SZ) == REG_EXPAND_SZ) || ((dwType & REG_SZ) == REG_SZ))
                {
                    if (_strcmpi((LPSTR)lpData, request_string) == 0)
                    {
                        cout << "Found in value " << keyPath << " data; " << achValue << ";" << endl
                            << "Data: " << lpData << endl;
                    }
                }
            }
        }
    }

    RegCloseKey(hKey);
}

// MSDN function
bool SetPrivilege(
    HANDLE hToken,          // access token handle
    LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
)
{
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!LookupPrivilegeValue(
        NULL,            // lookup privilege on local system
        lpszPrivilege,   // privilege to lookup 
        &luid
    ))                   // receives LUID of privilege
    {
        cout << "LookupPrivilegeValue error: " << GetLastError() << endl;
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;

    if (bEnablePrivilege)
    {
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    }
    else
    {
        tp.Privileges[0].Attributes = 0;
    }

    // Enable the privilege or disable all privileges.

    if (!AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES)NULL,
        (PDWORD)NULL
    ))
    {
        cout << "AdjustTokenPrivileges error: " << GetLastError() << endl;
        return false;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        cout << "The token does not have the specified privilege." << endl;
        return false;
    }

    return true;
}

void SaveToFile(HKEY hKey) {

    string file_path = "";
    DWORD retCode = ERROR_SUCCESS;

    cout << "Input file path: ";
    getline(cin, file_path);

    retCode = RegSaveKeyA(hKey, file_path.c_str(), NULL);

    switch (retCode)
    {
        case ERROR_SUCCESS:
        {
            cout << "Saved into file: " << file_path << endl;
            break;
        }
        case ERROR_ALREADY_EXISTS:
        {
            cout << "File already exists!" << endl;
            break;
        }
        default:
        {
            cout << "Cannot save key into file." << endl;
            break;
        }
    }

    RegCloseKey(hKey);
}
