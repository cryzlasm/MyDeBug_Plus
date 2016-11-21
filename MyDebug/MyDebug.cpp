// MyDebug.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "MyDebug.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

using namespace std;


int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
    int nRetCode = 0;
    
    // initialize MFC and print and error on failure
    if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
    {
        // TODO: change error code to suit your needs
        cerr << _T("Fatal Error: MFC initialization failed") << endl;
        nRetCode = 1;
    }
    else
    {
        if(argc > 1)
        {
            CDeBug DeBug;
            DeBug.Start(argv);
        }
        else
        {
            tcout << TEXT("请输入被调试程序的程序名！") << endl;
        }
        
    }
    
    return nRetCode;
}


