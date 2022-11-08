#ifndef __SControl__
#define __SControl__
#include <string>
#include <signal.h>
#include <Python.h>
#include <thread>

#include "TObject.h"
#include <TFile.h>
class SControl : public TObject {
    public:
    SControl() { 
    }
    static void Start() {
        int pid = fork();
        if (pid == 0) {
            chdir("/home/lab/Desktop/DAQ/build/");
            Py_Initialize();
            char *argv[9];
            int argc = 9;
            argv[0] = "acquire_sipm_data";
            argv[1] = "--config";
            argv[2] = "/home/lab/Desktop/DAQ/sandbox/config.ini";
            argv[3] = "-o";
            argv[4] = "run99999";
            argv[5] = "--mode";
            argv[6] = "qdc";
            argv[7] = "--time";
            argv[8] = "3600.0";
            wchar_t** _argv = static_cast<wchar_t**>(PyMem_Malloc(sizeof(wchar_t*)*argc) );
            for (int i=0; i<argc; i++) {
                wchar_t* arg = Py_DecodeLocale(argv[i], NULL);
                _argv[i] = arg;
            }
            PySys_SetArgv(argc, _argv);
            FILE *file = fopen("acquire_sipm_data", "r");
            PyRun_SimpleFile(file, "acquire_sipm_data");
            Py_Finalize(); 
        }
    }
    static void Stop() { 
        kill(0, SIGTERM);
    }
    private:
    ClassDef(SControl, 1)
};
#endif
