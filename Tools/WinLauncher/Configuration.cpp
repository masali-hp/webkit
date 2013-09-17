#include <windows.h>
#include <WebKit.h>
#include <WebKit/WebKitCOMAPI.h>
#include <stdio.h>

#include "WinLauncher.h"

#include "HPWebKitMemoryDebug.h"
#include "HPCommonSystemOS\FileSystems\FileSystems.h"

extern WinLauncherWebHost* gWebHost;
extern BSTR storagePath;
extern BSTR inspectorURL;
extern BSTR inspectorServer;

// Windows XP has a method named GetPrivateProfileString that makes it easy to read settings from .ini files.
// Windows CE does not have this method, so I have to implement my own version to support both platforms.
void GetPrivateProfileStringQ(const char * section, const char * param, const char * default_value, char * out_buff, size_t buff_size, const char * file_path)
{
    FILE *pIni = fopen(file_path, "r");
    bool found = false;
    if (pIni)
    {
        bool inSection = false;
        size_t param_len = strlen(param);
        size_t section_len = strlen(section);
        while (!found && fgets(out_buff, buff_size, pIni))
        {
            if (!inSection)
            {
                if (*out_buff == '['){
                    if ( (memcmp(out_buff + 1, section, section_len) == 0) &&
                          *(out_buff + 1 + section_len) == ']' )
                    {
                        inSection = true;
                    }
                }
            }
            else {
                if (memcmp(out_buff, param, param_len) == 0)
                {
                    char * b = out_buff + param_len;
                    while (*b++ != '=' && *b);
                    while (*b == ' ' && *b) { b++; }
                    param_len = strlen(b);
                    memmove(out_buff, b, param_len + 1);
                    // removing trailing whitespace
                    b = out_buff + param_len - 1;
                    while (*b == ' ' ||
                           *b == '\r' ||
                           *b == '\n' ||
                           *b == '\t')
                    {
                        *b-- = '\0';
                    }
                    found = true;
                }
            }
        }

        fclose(pIni);
    }
    if (!found)
        strcpy(out_buff, default_value);
}

static BSTR ConvertToBSTR(const char * buff)
{
    int len = strlen(buff);
    wchar_t * wbuff = new wchar_t[len+1];
    size_t wl = mbstowcs (wbuff, buff, len);
    BSTR result = SysAllocStringLen(wbuff, wl);
    delete wbuff;
    return result;
}

static void ConvertToWCHAR(const char * buff, wchar_t * wbuff)
{
    while (*buff)
        *wbuff++ = *buff++;
    *wbuff = 0;
}

void LoadIniSettings(const char * inipath)
{
    const int buff_size = 256;
    char buffer[buff_size];
    wchar_t wbuff[buff_size * 2 + 1];

    IWebNetworkConfiguration * networkConfig;
    HRESULT hr = WebKitCreateInstance(CLSID_WebNetworkConfiguration, 0, IID_IWebNetworkConfiguration, reinterpret_cast<void**>(&networkConfig));

    GetPrivateProfileStringQ("Proxy", "Host", "", buffer, buff_size, inipath);
    if (*buffer) {
        if (networkConfig) {
            BSTR proxy = ConvertToBSTR(buffer);
            GetPrivateProfileStringQ("Proxy", "Port", "8088", buffer, buff_size, inipath);
            int port = atoi(buffer);
            //GetPrivateProfileStringQ("Proxy", "Bypass", "", buffer, buff_size, inipath);
            networkConfig->setProxyInfo(proxy, port, NULL, NULL);
            SysFreeString(proxy);
        }
    }

    /*
    GetPrivateProfileStringQ("Credentials", "User", "", user, buff_size, inipath);
    GetPrivateProfileStringQ("Credentials", "Password", "", pass, buff_size, inipath);
    if (*user)
    {
        gWebHost->SetUserCredentials(wuser, wpass);
    }

    GetPrivateProfileStringQ("Timeouts", "Connect", "15", buffer, buff_size, inipath);
    if (*buffer)
    {
        WebKitSetConnectionTimeout(atoi(buffer));
    }

    GetPrivateProfileStringQ("Timeouts", "Operation", "60", buffer, buff_size, inipath);
    if (*buffer)
    {
        WebKitSetOperationTimeout(atoi(buffer));
    }
    */

    GetPrivateProfileStringQ("Memory", "MemoryPoolSize", "", buffer, buff_size, inipath);
    if (*buffer)
    {
        int size = atoi(buffer);
        SetMemoryPoolSize(size);
    }

    /*
    GetPrivateProfileStringQ("Memory", "MemoryOutBufferSize", "", buffer, buff_size, inipath);
    if (*buffer)
    {
        unsigned int size = atoi(buffer);
        SetMemoryOutBufferSize(size);
    }

    GetPrivateProfileStringQ("Memory", "MemoryLeakLogFile", "", buffer, buff_size, inipath);
    if (*buffer)
    {
        ConvertToWCHAR(buffer, szLeakLogFile);
    }
    else
    {
        szLeakLogFile[0] = 0;
    }

    GetPrivateProfileStringQ("Memory", "MemoryFreeLogFile", "", buffer, buff_size, inipath);
    if (*buffer)
    {
        ConvertToWCHAR(buffer, szFreeLogFile);
    }
    else
    {
        szFreeLogFile[0] = 0;
    }
    */

    GetPrivateProfileStringQ("Memory", "RecordAllocationStacks", "false", buffer, buff_size, inipath);
    EnableStackCollection(!strcmp(buffer, "true"));

    GetPrivateProfileStringQ("Memory", "RecordFreeStacks", "false", buffer, buff_size, inipath);
    EnableMemoryFreeLogging(!strcmp(buffer, "true"));

    /*GetPrivateProfileStringA("Memory", "CacheCapacity", "", buffer, buff_size, inipath);
    if (*buffer)
    {
        settings->cacheTotalCapacity = atoi(buffer);
    }
    */

    GetPrivateProfileStringQ("Inspector", "InspectorURL", "", buffer, buff_size, inipath);
    if (*buffer)
        inspectorURL = ConvertToBSTR(buffer);

    GetPrivateProfileStringQ("Inspector", "InspectorServer", "", buffer, buff_size, inipath);
    if (*buffer)
        inspectorServer = ConvertToBSTR(buffer);

    /*GetPrivateProfileStringQ("Other", "CookieFile", "", buffer, buff_size, inipath);
    if (*buffer)
    {
        ConvertToWCHAR(buffer, wbuff);
        WebKitSetCookiePath(wbuff);
    }*/

    GetPrivateProfileStringQ("Other", "CurlDebug", "", buffer, buff_size, inipath);
    if (*buffer)
    {
        bool curlDebug = (strcmp(buffer, "true") == 0);
        if (curlDebug)
            networkConfig->setDebugDelegate(gWebHost);
    }

    GetPrivateProfileStringQ("Other", "LogDir", "", buffer, buff_size, inipath);
    if (*buffer)
    {
        ConvertToWCHAR(buffer, wbuff);
        HP::Common::System::OS::FileSystems::HPSetDirectory(HP::Common::System::OS::FileSystems::LogDir, wbuff);
    }

    GetPrivateProfileStringQ("Other", "StoragePath", "", buffer, buff_size, inipath);
    if (*buffer)
    {
        ConvertToWCHAR(buffer, wbuff);
        storagePath = SysAllocString(wbuff);
    }

    //WebKitStartNewCookieSession();

    networkConfig->Release();
}
