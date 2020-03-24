/*
Copyright (c) 2011-2013 Roger Light <roger@atchoo.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of mosquitto nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#if defined(WIN32) || defined(__CYGWIN__)

#include <windows.h>

#include <memory_mosq.h>

#ifdef DXL
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#include "shlobj.h"
#endif

extern int run;
SERVICE_STATUS_HANDLE service_handle = 0;
static SERVICE_STATUS service_status;
int main(int argc, char *argv[]);

/* Service control callback */
void __stdcall service_handler(DWORD fdwControl)
{
    switch(fdwControl){
        case SERVICE_CONTROL_CONTINUE:
            /* Continue from Paused state. */
            break;
        case SERVICE_CONTROL_PAUSE:
            /* Pause service. */
            break;
        case SERVICE_CONTROL_SHUTDOWN:
            /* System is shutting down. */
        case SERVICE_CONTROL_STOP:
            /* Service should stop. */
            service_status.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus(service_handle, &service_status);
            run = 0;
            break;
    }
}

/* Function called when started as a service. */
void __stdcall service_main(DWORD dwArgc, LPTSTR *lpszArgv)
{
    char **argv;
    int argc = 1;
    char conf_path[MAX_PATH + 20];
#ifndef DXL
    int rc;
#endif

#ifndef DXL
    service_handle = RegisterServiceCtrlHandler("mosquitto", service_handler);
#else
    service_handle = RegisterServiceCtrlHandler("dxlbroker", service_handler);
#endif
    if(service_handle){
#ifdef DXL
        if(!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, conf_path))){
            service_status.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(service_handle, &service_status);
            return;
        }            
        strcat(conf_path, "\\McAfee\\dxlbroker\\dxlbroker.conf");
#else
        rc = GetEnvironmentVariable("MOSQUITTO_DIR", conf_path, MAX_PATH);
        if(!rc || rc == MAX_PATH){
            service_status.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(service_handle, &service_status);
            return;
        }
        strcat(conf_path, "/mosquitto.conf");
#endif

        argv = (char**)_mosquitto_malloc(sizeof(char *)*3);
#ifndef DXL
        argv[0] = "mosquitto";
        argv[1] = "-c";
        argv[2] = conf_path;
        argc = 3;
#else
        argv[0] = "dxlbroker";
        argv[1] = "--config";
        argv[2] = conf_path;
        argc = 3;
#endif

        service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        service_status.dwCurrentState = SERVICE_RUNNING;
        service_status.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP;
        service_status.dwWin32ExitCode = NO_ERROR;
        service_status.dwCheckPoint = 0;
        SetServiceStatus(service_handle, &service_status);

        int retVal = main(argc, argv); 
        _mosquitto_free(argv);

        // DXL begin
        if(retVal != 0){
            // Force abnormal exit to cause restart
            exit(retVal);
            return;
        }
        // DXL end

        service_status.dwCurrentState = SERVICE_STOPPED;
        
        SetServiceStatus(service_handle, &service_status);
    }
}

void service_install(void)
{
    SC_HANDLE sc_manager, svc_handle;
    char exe_path[MAX_PATH + 7];
    SERVICE_DESCRIPTION svc_desc;

    exe_path[0] = '\"';
    GetModuleFileName(NULL, exe_path+sizeof(exe_path[0]), MAX_PATH);
    strcat(exe_path, "\" run");

    sc_manager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if(sc_manager){
#ifndef DXL
        svc_handle = CreateService(sc_manager, "mosquitto", "Mosquitto Broker", 
#else
        svc_handle = CreateService(sc_manager, "dxlbroker", "McAfee DXL Broker", 
#endif
            SERVICE_START | SERVICE_STOP | SERVICE_CHANGE_CONFIG,
            SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
            exe_path, NULL, NULL, NULL, NULL, NULL);

        if(svc_handle){
#ifndef DXL
            svc_desc.lpDescription = "MQTT v3.1 broker";
#else
            svc_desc.lpDescription = "McAfee DXL Broker";
#endif
            ChangeServiceConfig2(svc_handle, SERVICE_CONFIG_DESCRIPTION, &svc_desc);


#ifdef DXL
            // Service failure restart actions
            SERVICE_FAILURE_ACTIONS servFailActions;
            SC_ACTION failActions[3];

            failActions[0].Type = SC_ACTION_RESTART;
            failActions[0].Delay = 60 * 1000; // 10 seconds
            failActions[1].Type = SC_ACTION_RESTART;
            failActions[1].Delay = 60 * 1000; // 10 seconds
            failActions[2].Type = SC_ACTION_RESTART;
            failActions[2].Delay = 60 * 1000; // 10 seconds
            

            servFailActions.dwResetPeriod = 60 * 60 * 24; // Reset Failures Counter, in Seconds = 1day
            servFailActions.lpCommand = NULL; //Command to perform due to service failure, not used
            servFailActions.lpRebootMsg = NULL; //Message during rebooting computer due to service failure, not used
            servFailActions.cActions = 3; 
            servFailActions.lpsaActions = failActions;
            ChangeServiceConfig2(svc_handle, SERVICE_CONFIG_FAILURE_ACTIONS, &servFailActions);
#endif

            CloseServiceHandle(svc_handle);
        }
        CloseServiceHandle(sc_manager);
    }
}

void service_uninstall(void)
{
    SC_HANDLE sc_manager, svc_handle;
    SERVICE_STATUS status;

    sc_manager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
    if(sc_manager){
#ifndef DXL
        svc_handle = OpenService(sc_manager, "mosquitto", SERVICE_QUERY_STATUS | DELETE);
#else
        svc_handle = OpenService(sc_manager, "dxlbroker", SERVICE_QUERY_STATUS | DELETE);
#endif
        if(svc_handle){
            if(QueryServiceStatus(svc_handle, &status)){
                if(status.dwCurrentState == SERVICE_STOPPED){
                    DeleteService(svc_handle);
                }
            }
            CloseServiceHandle(svc_handle);
        }
        CloseServiceHandle(sc_manager);
    }
}

void service_run(void)
{
    SERVICE_TABLE_ENTRY ste[] = {
#ifndef DXL
        { "mosquitto", service_main },
#else
        { "dxlbroker", service_main },
#endif
        { NULL, NULL }
    };

    StartServiceCtrlDispatcher(ste);
}

#endif
