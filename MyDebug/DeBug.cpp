// DeBug.cpp: implementation of the CDeBug class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MyDebug.h"
#include "DeBug.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void __stdcall CDeBug::OutErrMsg(LPCTSTR strErrMsg)
{
    OutputDebugString(strErrMsg);
}

CDeBug::CDeBug()
{
    memset(&m_DbgEvt, 0, sizeof(DEBUG_EVENT));
    memset(&m_DstContext, 0, sizeof(CONTEXT));
    m_hDstProcess = INVALID_HANDLE_VALUE;
    m_hDstThread = INVALID_HANDLE_VALUE;
    
    m_pfnOpenThread = NULL;
    
    m_dwErrCount = 0;
    
    m_bIsMyStepOver = FALSE;
    m_bIsMyStepInto = FALSE;
    
    m_bIsScript = FALSE;
    m_bIsInput = TRUE;
    
    m_bIsNormalStep = FALSE;
    m_lpTmpStepAddr = NULL;

    m_bIsMyHardReSet = FALSE;

    m_dwWhichHardReg = 0;

    m_bIsMemStep = FALSE;

    m_lpTmpMemExec = NULL;
}

CDeBug::~CDeBug()
{
    POSITION pos = m_ModuleLst.GetHeadPosition();
    POSITION posTmp = NULL;
    while(pos)
    {
        posTmp = pos;
        PMODLST pMod = m_ModuleLst.GetNext(pos);
        if(pMod != NULL)
        {
            delete pMod;
            m_ModuleLst.RemoveAt(posTmp);
        }
    }
    
    pos = m_NorMalBpLst.GetHeadPosition();
    while(pos)
    {
        posTmp = pos;
        PMYBREAK_POINT pBp = m_NorMalBpLst.GetNext(pos);
        if(pBp != NULL)
        {
            delete pBp;
            m_NorMalBpLst.RemoveAt(posTmp);
        }
    }

    pos = m_HardBpLst.GetHeadPosition();
    while(pos)
    {
        posTmp = pos;
        PMYBREAK_POINT pBp = m_HardBpLst.GetNext(pos);
        if(pBp != NULL)
        {
            delete pBp;
            m_HardBpLst.RemoveAt(posTmp);
        }
    }pos = m_HardBpLst.GetHeadPosition();

    pos = m_CmdOrderLst.GetHeadPosition();
    while(pos)
    {
        posTmp = pos;
        CString* pStr = m_CmdOrderLst.GetNext(pos);
        if(pStr != NULL)
        {
            delete pStr;
            m_CmdOrderLst.RemoveAt(posTmp);
        }
    }
}

BOOL CDeBug::GetFun()
{
    
    m_pfnOpenThread = (PFN_OpenThread)GetProcAddress(
        LoadLibrary(TEXT("Kernel32.dll")), 
        "OpenThread");
    if(m_pfnOpenThread == NULL)
    {
        tcout << TEXT("��ȡ����ʧ�ܣ�����ϵ����Ա��") << endl;
        return FALSE;
    }
    
    return TRUE;
}

BOOL CDeBug::Start(TCHAR* argv[])			//����ʼ
{
    BOOL bRet = FALSE;
    CString strArgv = argv[1];
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    
    if(strArgv.Find(TEXT(".exe")) == -1)
    {
        //�������Թ�ϵ
        bRet = CreateProcess(NULL, 
            argv[1], 
            NULL,
            NULL,
            FALSE,
            DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE,
            NULL,
            NULL,
            &si,
            &pi);
        if(bRet == FALSE)
        {
            tcout << TEXT("��������ȷ�ı����Գ�������") << endl;
            return FALSE;
        }
    }
    else
    {
        //�������Թ�ϵ
        bRet = CreateProcess(NULL, 
            argv[1], 
            NULL,
            NULL,
            FALSE,
            DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE,
            NULL,
            NULL,
            &si,
            &pi);
    }
    
    system("color 0a");
    if(!GetFun())
        return FALSE;
    
    return EventLoop();
}

BOOL CDeBug::EventLoop()       //��Ϣѭ��
{
    DWORD bpState = DBG_EXCEPTION_NOT_HANDLED;
    BOOL bRet = FALSE;
    
    while(TRUE == WaitForDebugEvent(&m_DbgEvt, INFINITE))
    {
        if(m_dwErrCount > 10)
        {
            tcout << TEXT("����������࣬����ϵ����Ա��") << endl;
            break;
        }
        
        m_hDstProcess = OpenProcess(PROCESS_ALL_ACCESS, 
            FALSE, 
            m_DbgEvt.dwProcessId);
        if(m_hDstProcess == NULL)
        {
            tcout << TEXT("�򿪵��Խ���ʧ�ܣ�") << endl;
            m_dwErrCount++;
            continue;
        }
        
        m_hDstThread = m_pfnOpenThread(THREAD_ALL_ACCESS, 
            FALSE, 
            m_DbgEvt.dwThreadId);
        if(m_hDstThread == NULL)
        {
            tcout << TEXT("�򿪵��Խ���ʧ�ܣ�") << endl;
            m_dwErrCount++;
            continue;
        }
        
        
        m_DstContext.ContextFlags = CONTEXT_ALL;
        GetThreadContext(m_hDstThread, &m_DstContext);
        
        switch(m_DbgEvt.dwDebugEventCode)
        {
        case EXCEPTION_DEBUG_EVENT:
            //tcout << TEXT("EXCEPTION_DEBUG_EVENT") << endl;
            
            bRet = OnExceptionEvent();
            
            break;
        case CREATE_THREAD_DEBUG_EVENT:		
            tcout << TEXT("CREATE_THREAD_DEBUG_EVENT") << endl;
            
            //bRet = OnCreateThreadEvent();
            
            break;
        case CREATE_PROCESS_DEBUG_EVENT:
            //tcout << TEXT("CREATE_PROCESS_DEBUG_EVENT") << endl;
            
            bRet = OnCreateProcessEvent();
            
            break;
            
        case EXIT_THREAD_DEBUG_EVENT:		
            tcout << TEXT("EXIT_THREAD_DEBUG_EVENT") << endl;
            break;
            
        case EXIT_PROCESS_DEBUG_EVENT:	
            //tcout << TEXT("EXIT_PROCESS_DEBUG_EVENT") << endl;	
            tcout << TEXT("�����Գ������˳�!") << endl;	
            return TRUE;
            
        case LOAD_DLL_DEBUG_EVENT:	
            bRet = OnLoadDll();
            
            //tcout << TEXT("LOAD_DLL_DEBUG_EVENT") << endl;		
            break;
            
        case UNLOAD_DLL_DEBUG_EVENT:
            //tcout << TEXT("UNLOAD_DLL_DEBUG_EVENT") << endl;
            bRet = OnUnLoadDll();
            break;
            
        case OUTPUT_DEBUG_STRING_EVENT:	
            //tcout << TEXT("OUTPUT_DEBUG_STRING_EVENT") << endl;	
            tcout << TEXT("�����Գ����е�����Ϣ�������������δ���в���.") << endl;		
            break;
            
        default:
            break;
        }
        
        //����Ѿ������򷵻��Ѵ�������Ĭ�Ϸ���û����
        if(bRet)
            bpState = DBG_CONTINUE;
        
        //m_DstContext.Dr6 = 0;
        
        //�����߳�������
        if(!SetThreadContext(m_hDstThread, &m_DstContext))
        {
            tcout << TEXT("�����߳���Ϣʧ�ܣ�����ϵ����Ա") << endl;
        }
        
        //�رս��̾��
        if (m_hDstProcess != NULL)
        {
            CloseHandle(m_hDstProcess);
            m_hDstProcess = NULL;
        }
        
        //�ر��߳̾��
        if (m_hDstThread != NULL)
        {
            CloseHandle(m_hDstThread);
            m_hDstThread = NULL;
        }
        
        //���ô���״̬
        if(ContinueDebugEvent(m_DbgEvt.dwProcessId, 
            m_DbgEvt.dwThreadId, 
            bpState) == FALSE)
        {
            return FALSE;
        }
    }//End While
    
    return TRUE;
}

#define MAX_MEM 0x80000000
BOOL CDeBug::Interaction(LPVOID lpAddr, BOOL bIsShowDbgInfo)                //�˻�����
{   
    //����ַ�Ƿ�Խ��
    if((DWORD)lpAddr >= MAX_MEM)
    {
        tcout << TEXT("��Ч���򣬳���������գ�") << endl;
        return TRUE;
    }
    
    if(bIsShowDbgInfo)
    {
        //��ʾ��ǰ������Ϣ
        if(!ShowCurAllDbg(lpAddr))
        {
            m_dwErrCount++;
            OutErrMsg(TEXT("Interaction��δ֪������ʾ����"));
            return FALSE;
        }
    }
    
    //��ʼ����ȡ�û�����
    CMD_INFO CmdInfo;
    //ZeroMemory(&CmdInfo, sizeof(CMD_INFO));
    CmdInfo.dwState = CMD_INVALID;
    CmdInfo.bIsBreakInputLoop = FALSE;
    CmdInfo.dwPreAddr = NULL;
    
    while(TRUE)
    {
        //��ȡ�û�����
        BOOL bRet = GetUserInput(CmdInfo);
        if(bRet == FALSE)
        {
            m_dwErrCount++;
            OutErrMsg(TEXT("Interaction��δ֪�����������"));
            return FALSE;
        }
        else if(!m_bIsScript && !m_bIsInput)
        {
            m_bIsInput = TRUE;
            continue;
        }
        
        
        //�����û�����
        if(!HandleCmd(CmdInfo, lpAddr))
        {
            m_dwErrCount++;
            OutErrMsg(TEXT("Interaction��δ֪����ִ�д���"));
            return FALSE;
        }
        
        if(CmdInfo.bIsBreakInputLoop)
            break;
    }
    
    return TRUE;
}

#define  FILE_NAME TEXT("out.txt")
BOOL CDeBug::SaveScript(CMD_INFO& CmdInfo, LPVOID lpAddr)          //����ű�
{
//     TCHAR szFileName[MAX_PATH] = {0};
//     tcout << TEXT("�������ļ���") ;
//     _tscanf(TEXT("%255s"), szFileName);

    //�س�����
    char szBuf[3] = {0x0d, 0x0a, 0};
    
    try
    {
        CStdioFile File;
        if(!File.Open(FILE_NAME, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary | CFile::shareDenyRead))
        {
            tcout << TEXT("�����ļ�ʧ��") << endl;
            return FALSE;
        }
        
        //ɾ�����һ������
        m_CmdOrderLst.RemoveTail();
        
        //��������
        POSITION pos = m_CmdOrderLst.GetHeadPosition();
        while(pos)
        {
            CString & strOut = *m_CmdOrderLst.GetNext(pos);
            File.Write((LPCTSTR)strOut, strOut.GetLength());
            File.Write(szBuf, sizeof(szBuf) - 1);
            File.Flush();
        }
        File.Close();
        tcout << TEXT("�����ɹ�") << endl;
    }
    catch (CFileException* e)
    {
        TCHAR szBuf[MAXBYTE] = {0};

        try
        {
            e->GetErrorMessage(szBuf, MAXBYTE);
            tcout << TEXT("�ļ���������") << endl;
            tcout << szBuf << endl;
        }
        catch(...)
        {}
    }

    

    return TRUE;
}

BOOL CDeBug::LoadScript(CMD_INFO& CmdInfo, LPVOID lpAddr)          //����ű�
{
    try
    {
        CStdioFile File;
        if(!File.Open(FILE_NAME, CFile::modeRead))
        {
            tcout << TEXT("��ȡ�ļ�����") << endl;
            m_dwErrCount++;
            return FALSE;
        }

        //�����������
        if(m_CmdOrderLst.GetCount() > 0)
        {
            POSITION posTmp = NULL;
            POSITION pos = m_CmdOrderLst.GetHeadPosition();
            while(pos)
            {
                posTmp = pos;
                CString* pStr = m_CmdOrderLst.GetNext(pos);
                if(pStr != NULL)
                {
                    delete pStr;
                    m_CmdOrderLst.RemoveAt(posTmp);
                }
            }
        }

        //��ȡ�ļ�
        CString strRead = TEXT("");

        while(File.ReadString(strRead))
        {
            CString* pStr = new CString;
            if(pStr == NULL)
            {
                OutErrMsg(TEXT("����ڵ�ʧ�ܣ�"));
                m_dwErrCount++;
            }
            *pStr = strRead;
            m_CmdOrderLst.AddTail(pStr);
        }

        m_bIsScript = TRUE;
        m_bIsInput = FALSE;
        
        File.Close();

        tcout << TEXT("����ɹ�") << endl;
    }
    catch(...)
    {
        tcout << TEXT("�ļ���������") << endl;    
        m_dwErrCount++;
        return FALSE;
    }
    
    return TRUE;
}

#define MAX_INPUT   32
BOOL CDeBug::GetUserInput(CMD_INFO& CmdInfo)
{
    if(CmdInfo.dwState != CMD_DISPLAY_DATA && CmdInfo.dwState != CMD_DISPLAY_ASMCODE)
    {
        CmdInfo.dwPreAddr = NULL;
    }
    CmdInfo.strCMD = TEXT("");
    try
    {   
        if(!m_bIsScript)
        {
            //��ȡ����
            TCHAR szBuf[MAX_INPUT] = {0};
            tcout << TEXT('>');
            
            tcin.clear();
            tcin.sync();
            tcin.getline(szBuf, MAX_INPUT, TEXT('\n'));
            tcin.clear();
            tcin.sync();
            
            //��������
            CmdInfo.strCMD = szBuf;
            
            //����Сд
            CmdInfo.strCMD.MakeLower();
            
            
            CString strTmp = CmdInfo.strCMD;
            //ת��Ϊ������
            if(m_CMD.Resolve(CmdInfo))
            {
                //������������
                CString* pStrOrder = new CString;
                if(pStrOrder == NULL)
                {
                    OutErrMsg("GetUserInput����������ڵ�ʧ��");
                    m_dwErrCount++;
                }
                *pStrOrder = strTmp;
                m_CmdOrderLst.AddTail(pStrOrder);
            }
        }
        else
        {
            if(m_CmdOrderLst.GetCount() < 1)
            {
                m_bIsScript = FALSE;
                m_bIsInput = FALSE;
            }
            else
            {
                CString* pStr = m_CmdOrderLst.RemoveHead();
                CmdInfo.strCMD = *pStr;
                tcout << TEXT(">") << (LPCTSTR)CmdInfo.strCMD << endl;
                delete pStr;
                m_CMD.Resolve(CmdInfo);
            }
        }
    }
    catch(...)
    {
        return FALSE;
    }
    
    return TRUE;
}

BOOL CDeBug::OnBreakPointEvent()       //һ��ϵ�
{
    //static BOOL bIsFirstInto = TRUE;
    EXCEPTION_RECORD& pExceptionRecord = m_DbgEvt.u.Exception.ExceptionRecord; 
    POSITION pos = NULL;
    
    //     //��һ��������ϵͳ�ϵ㣬���ڶ�����ڵ�
    //     if(bIsFirstInto)
    //     {
    //         if(IsAddrInBpList(pExceptionRecord.ExceptionAddress, m_BreakPoint, pos))
    //         {
    //             PMYBREAK_POINT bp = m_BreakPoint.GetAt(pos);
    //             if(WriteRemoteCode(bp->lpAddr, bp->dwOldOrder, bp->dwCurOrder))
    //             {
    //                 m_BreakPoint.RemoveAt(pos);
    //                 delete bp;
    //             }
    //         }
    //         ShowCurAll(pExceptionRecord.ExceptionAddress);
    //         Interaction(pExceptionRecord.ExceptionAddress);
    //         bIsFirstInto = FALSE;
    //         return TRUE;
    //     }
    
    //�����ϵ�
    if(IsAddrInBpList(pExceptionRecord.ExceptionAddress, m_NorMalBpLst, pos))
    {
        PMYBREAK_POINT bp = m_NorMalBpLst.GetAt(pos);
        //��ԭ����
        if(!WriteRemoteCode(bp->lpAddr, bp->dwOldOrder, bp->dwCurOrder))
        {
            return FALSE;
        }
        
        //�޸�Ŀ���߳�EIP
        m_DstContext.Eip = m_DstContext.Eip - 1;
        
        //ϵͳһ���Զϵ㣬���ڶ�����ڵ�
        if(bp->bpState == BP_SYS || bp->bpState == BP_ONCE)
        {
            m_NorMalBpLst.RemoveAt(pos);
            delete bp;
        }
        //����ϵ�
        else if(bp->bpState == BP_NORMAL)
        {
            //���õ�����־λ
            m_DstContext.EFlags |= TF;
            
            bp->bIsSingleStep = TRUE;
            m_bIsNormalStep = TRUE;
            m_lpTmpStepAddr = bp->lpAddr;
            ShowCurAllDbg(pExceptionRecord.ExceptionAddress, CMD_SHOWFIVE);
            return TRUE;
        }
        
        return Interaction((LPVOID)m_DstContext.Eip);
    }
    
    return FALSE;
}

BOOL CDeBug::OnSingleStepEvent()       //�����쳣
{
    BOOL bRet = FALSE;
    EXCEPTION_RECORD& pExceptionRecord = m_DbgEvt.u.Exception.ExceptionRecord; 
    POSITION pos = NULL;
    PMYDR6 Dr6 = (PMYDR6)&(m_DstContext.Dr6);
    PMYDR7 Dr7 = (PMYDR7)&(m_DstContext.Dr7);

    //һ��ϵ�
    if(m_bIsNormalStep)
    {
        if(IsAddrInBpList(m_lpTmpStepAddr, m_NorMalBpLst, pos))
        {
            PMYBREAK_POINT bp = m_NorMalBpLst.GetAt(pos);
            if(bp->bIsSingleStep == TRUE)
            {
                //����ϵ�
                if(!WriteRemoteCode(bp->lpAddr, bp->dwCurOrder, bp->dwOldOrder))
                {
                    return FALSE;
                }
                
                bp->bIsSingleStep = FALSE;
                m_bIsNormalStep = FALSE;

                return Interaction(bp->lpAddr, FALSE);
            }
        }
    }

    //�ڴ�ϵ�
    if(m_bIsMemStep)
    {
        if(IsAddrInBpList(m_lpTmpStepAddr, m_MemBpLst, pos))
        {
            PMYBREAK_POINT bp = m_MemBpLst.GetAt(pos);
            if(bp->bIsSingleStep == TRUE)
            {
                tcout << TEXT("�ڴ�ϵ�����") << endl;
                //����ϵ�
                if(!VirtualProtectEx(m_hDstProcess, bp->lpAddr, bp->dwLen, PAGE_NOACCESS, &bp->dwOldProtect))
                {
                    return FALSE;
                }
                
                bp->bIsSingleStep = FALSE;
                m_bIsMemStep = FALSE;
                
                return Interaction(m_lpTmpMemExec, FALSE);
            }
        }
        
    }
    
    //F7����
    if(m_bIsMyStepInto)
    {
        m_bIsMyStepInto = FALSE;
        //���������һ��ͬʱ��������ʾ�ڶ�������
        if(!bRet)
        {
            bRet = Interaction(pExceptionRecord.ExceptionAddress);
        }
    }
    //Ӳ���ϵ�Ķϲ����
    if(m_bIsMyHardReSet)
    {
        UINT lpAddr = NULL;
        switch(m_dwWhichHardReg)
        {
        case 0:
            Dr7->L0 = HBP_SET;
            lpAddr = m_DstContext.Dr0;
            break;

        case 1:
            Dr7->L1 = HBP_SET;
            lpAddr = m_DstContext.Dr1;
            break;

        case 2:
            Dr7->L2 = HBP_SET;
            lpAddr = m_DstContext.Dr2;
            break;

        case 3:
            Dr7->L3 = HBP_SET;
            lpAddr = m_DstContext.Dr3;
            break;
        }
        m_bIsMyHardReSet = FALSE;

        //���������Ӳ��ͬʱ��������ʾ�ڶ�������
        if(!bRet)
        {
            bRet = Interaction((LPVOID)lpAddr, FALSE);
        }
        
    }

    //Ӳ��1������
    if(Dr6->B0 == HBP_HIT)
    {
        m_dwWhichHardReg = 0;
        Dr7->L0 = HBP_UNSET;
        if(Dr7->RW0 == HBP_INSTRUCTION_EXECUT)
        {
            ShowCurAllDbg(pExceptionRecord.ExceptionAddress, CMD_SHOWFIVE);
            
            _tprintf(TEXT("Ӳ��ִ�жϵ�����\r\n"));

            //�ϲ����,����ϵ�
            m_bIsMyHardReSet = TRUE;
            m_DstContext.EFlags |= TF;
            bRet = TRUE;
        }
        else
        {
            _tprintf(TEXT("Ӳ�������ϵ�����, , ��ǰָ��Ϊ�ϵ���һ��.\r\n"));
            bRet = Interaction(pExceptionRecord.ExceptionAddress);
        }
    }
    //Ӳ��2������
    else if(Dr6->B1 == HBP_HIT)
    {
        m_dwWhichHardReg = 1;
        Dr7->L1 = HBP_UNSET;
        if(Dr7->RW1 == HBP_INSTRUCTION_EXECUT)
        {
            ShowCurAllDbg(pExceptionRecord.ExceptionAddress, CMD_SHOWFIVE);
            
            _tprintf(TEXT("Ӳ��ִ�жϵ�����\r\n"));
            
            //�ϲ����,����ϵ�
            m_bIsMyHardReSet = TRUE;
            m_DstContext.EFlags |= TF;
            bRet = TRUE;
        }
        else
        {
            _tprintf(TEXT("Ӳ�������ϵ�����, ��ǰָ��Ϊ�ϵ���һ��.\r\n"));
            bRet = Interaction(pExceptionRecord.ExceptionAddress);
        }
    }
    //Ӳ��3������
    else if(Dr6->B2 == HBP_HIT)
    {
        m_dwWhichHardReg = 2;
        Dr7->L2 = HBP_UNSET;
        if(Dr7->RW2 == HBP_INSTRUCTION_EXECUT)
        {
            ShowCurAllDbg(pExceptionRecord.ExceptionAddress, CMD_SHOWFIVE);
            
            _tprintf(TEXT("Ӳ��ִ�жϵ�����\r\n"));
            
            //�ϲ����,����ϵ�
            m_bIsMyHardReSet = TRUE;
            m_DstContext.EFlags |= TF;
            bRet = TRUE;
        }
        else
        {
            _tprintf(TEXT("Ӳ�������ϵ�����, ��ǰָ��Ϊ�ϵ���һ��.\r\n"));
            bRet = Interaction(pExceptionRecord.ExceptionAddress);
        }
    }
    //Ӳ��4������
    else if(Dr6->B3 == HBP_HIT)
    {
        m_dwWhichHardReg = 3;
        Dr7->L3 = HBP_UNSET;
        if(Dr7->RW3 == HBP_INSTRUCTION_EXECUT)
        {
            ShowCurAllDbg(pExceptionRecord.ExceptionAddress, CMD_SHOWFIVE);
            
            _tprintf(TEXT("Ӳ��ִ�жϵ�����\r\n"));
            
            //�ϲ����,����ϵ�
            m_bIsMyHardReSet = TRUE;
            m_DstContext.EFlags |= TF;
            bRet = TRUE;
        }
        else
        {
            _tprintf(TEXT("Ӳ�������ϵ�����, ��ǰָ��Ϊ�ϵ���һ��.\r\n"));
            bRet = Interaction(pExceptionRecord.ExceptionAddress);
        }
    }

    
    return bRet;
}

BOOL CDeBug::OnAccessVolationEvent()   //�ڴ�����쳣
{
    //static BOOL bIsFirstInto = TRUE;
    EXCEPTION_RECORD& pExceptionRecord = m_DbgEvt.u.Exception.ExceptionRecord; 
    POSITION pos = NULL;

    //�����ϵ�
    if(IsAddrInBpList(pExceptionRecord.ExceptionAddress, m_MemBpLst, pos))
    {
        MYBREAK_POINT& bp = *m_MemBpLst.GetAt(pos);
        //��ԭ����
        DWORD dwOldProtect = 0;
        
        //����ϵ�
        if(bp.bpState == BP_MEM)
        {
            if(!VirtualProtectEx(m_hDstProcess, bp.lpAddr, bp.dwLen, bp.dwOldProtect, &dwOldProtect))
            {
                OutErrMsg(TEXT("��ԭ�ڴ�����ʧ�ܡ�"));
                return FALSE;
            }
            //���õ�����־λ
            m_DstContext.EFlags |= TF;
            
            bp.bIsSingleStep = TRUE;
            m_bIsMemStep = TRUE;
            m_lpTmpStepAddr = bp.lpAddr;
            m_lpTmpMemExec = pExceptionRecord.ExceptionAddress;
            ShowCurAllDbg(pExceptionRecord.ExceptionAddress, CMD_SHOWFIVE);
            return TRUE;
        }
        
        return Interaction((LPVOID)m_DstContext.Eip);
    }
    
    return FALSE;
}

BOOL CDeBug::ShowCurAllDbg(LPVOID lpAddr, CMDSTATE cmdState)  //��ʾ��ǰ���е�����Ϣ
{
    //ShowRemoteMem(lpAddr);
    ShowRemoteReg();
    DWORD dwCount = 1;
    DWORD dwNotUse = 0;
    
    if(cmdState == CMD_SHOWONCE)
        dwCount = 1;
    else if(cmdState == CMD_SHOWFIVE)
        dwCount = 5;
    else
        dwCount = 10;
    
    //��ʾ�����
    if(!ShowRemoteDisAsm(lpAddr, dwNotUse, dwCount))
        return FALSE;
    
    return TRUE;
}

BOOL CDeBug::CmdShowAsm(CMD_INFO& CmdInfo, LPVOID lpAddr)  //��ʾ�����
{
    BOOL bRet = TRUE;
    
    int nAddr = 0;

    //�鿴ָ���ڴ�
    if(CmdInfo.strCMD.GetLength() >1)
    {
        if(CmdInfo.dwPreAddr == NULL)
        {
            PTCHAR pTmp = NULL;
            nAddr = _tcstol((LPCTSTR)CmdInfo.strCMD, &pTmp, CONVERT_HEX);
        }
        else
        {
            nAddr = CmdInfo.dwPreAddr;
        }
        //int nAddr = atoi(CmdInfo.strCMD);
        bRet = ShowRemoteDisAsm((LPVOID)nAddr, CmdInfo.dwPreAddr);
    }
    //�鿴��ǰ�ڴ�
    else
    {
        if(CmdInfo.dwPreAddr == NULL)
            nAddr = (int)lpAddr;
        else
            nAddr = CmdInfo.dwPreAddr;

        bRet = ShowRemoteDisAsm((LPVOID)nAddr, CmdInfo.dwPreAddr);
    }
    
    return bRet;
}

BOOL CDeBug::CmdShowMem(CMD_INFO& CmdInfo, LPVOID lpAddr)
{
    BOOL bRet = TRUE;
    int nAddr = 0;
    if(CmdInfo.strCMD.GetLength() >1)
    {
        if(CmdInfo.dwPreAddr == NULL)
        {
            PTCHAR pTmp = NULL;
            nAddr = _tcstol(CmdInfo.strCMD, &pTmp, CONVERT_HEX);
        }
        else
        {
            nAddr = CmdInfo.dwPreAddr;
        }
        //int nAddr = atoi(CmdInfo.strCMD);
        bRet = ShowRemoteMem((LPVOID)nAddr, CmdInfo.dwPreAddr);
    }
    else
    {
        if(CmdInfo.dwPreAddr == NULL)
            nAddr = (int)lpAddr;
        else
            nAddr = CmdInfo.dwPreAddr;

        bRet = ShowRemoteMem((LPVOID)nAddr, CmdInfo.dwPreAddr);
    }
    
    return bRet;
}

#define RemoteOneReadSize 0x60  //һ�ζ�ȡԶ�����ݵĳ���
BOOL CDeBug::ShowRemoteMem(LPVOID lpAddr, DWORD& dwOutCurAddr)           //��ʾԶ���ڴ�
{
    DWORD dwAddr = (DWORD)lpAddr;
    DWORD dwRead = 0;
    UCHAR szBuf[RemoteOneReadSize] = {0};
    PUCHAR pszBuf = szBuf;
    
    //��ȡԶ���ڴ���Ϣ
    if(!ReadProcessMemory(m_hDstProcess, lpAddr, szBuf, RemoteOneReadSize, &dwRead))
    {
        OutErrMsg(TEXT("ShowRemoteDisAsm����ȡԶ���ڴ�ʧ�ܣ�"));
        return FALSE;
    }
    
    //����ڴ���Ϣ
    int nCount = dwRead / 0X10;
    for(int i = 0; i < nCount; i++)
    {
        //�����ַ
        _tprintf(TEXT("%08X   "), dwAddr);
        //tcout << ios::hex << dwAddr << TEXT("    ");
        
        //���ʮ������ֵ
        for(int j = 0; j < 0x10; j++)
        {
            _tprintf(TEXT("%02X "), pszBuf[j]);
            //tcout << ios::hex << pszBuf[j] << TEXT(' ');
        }
        
        tcout << TEXT("  ");
        
        //��������ַ���
        for(int n = 0; n < 0x10; n++)
        {
            if(pszBuf[n] < 32 || pszBuf[n] > 127)
                _puttchar(TEXT('.'));
            else
                _puttchar(pszBuf[n]);
        }
        
        //���س�����
        tcout << endl;
        
        dwAddr += 0x10;
        pszBuf += 0x10;
    }
    dwOutCurAddr = dwAddr;
    return TRUE;
}

BOOL CDeBug::ShowRemoteReg()           //��ʾԶ�̼Ĵ���
{
    // EAX=00000000   EBX=00000000   ECX=B2A10000   EDX=0008E3C8   ESI=FFFFFFFE
    // EDI=00000000   EIP=7703103C   ESP=0018FB08   EBP=0018FB34   DS =0000002B
    // ES =0000002B   SS =0000002B   FS =00000053   GS =0000002B   CS =00000023
    //��ȡEFlags
    EFLAGS& eFlags = *(PEFLAGS)&m_DstContext.EFlags;
    
    CONTEXT& text = m_DstContext;
    _tprintf(TEXT("EAX=%08X   EBX=%08X   ECX=%08X   EDX=%08X   ESI=%08X\r\n"),
        text.Eax,
        text.Ebx,
        text.Ecx,
        text.Edx,
        text.Esi);
    
    _tprintf(TEXT("EDI=%08X   EIP=%08X   ESP=%08X   EBP=%08X   DS =%08X\r\n"),
        text.Edi,
        text.Eip,
        text.Esp,
        text.Ebp,
        text.SegDs);
    
    _tprintf(TEXT("ES =%08X   SS =%08X   FS =%08X   GS =%08X   CS =%08X\r\n"),
        text.SegEs,
        text.SegSs,
        text.SegFs,
        text.SegGs,
        text.SegCs);
    
    _tprintf(TEXT("OF   DF   IF   TF   SF   ZF   AF   PF   CF\r\n"));
    
    _tprintf(TEXT("%02d   %02d   %02d   %02d   %02d   %02d   %02d   %02d   %02d\r\n\r\n"),
        eFlags.dwOF,
        eFlags.dwDF,
        eFlags.dwIF,
        eFlags.dwTF,
        eFlags.dwSF,
        eFlags.dwZF,
        eFlags.dwAF,
        eFlags.dwPF,
        eFlags.dwCF);
    
    return TRUE;
}


//�����ָ����ַһ������
BOOL CDeBug::GetOneAsm(LPVOID lpAddr, DWORD& dwOrderCount, CString& strOutAsm)
{
    UINT unCodeAddress = (DWORD)lpAddr;    //��ַ
    DWORD dwRead = 0;
    UCHAR szBuf[MAXBYTE] = {0};         //Զ���ڴ滺����
    //BYTE btCode[MAXBYTE] = {0};         //
    char szAsmBuf[MAXBYTE] = {0};       //����໺����
    char szOpcodeBuf[MAXBYTE] = {0};    //�����뻺����
    
    PBYTE pCode = szBuf;
    UINT unCodeSize = 0;
    
    
    //��ȡԶ����Ϣ
    if(!ReadProcessMemory(m_hDstProcess, lpAddr, szBuf, RemoteOneReadSize, &dwRead))
    {
        OutErrMsg(TEXT("ShowRemoteDisAsm����ȡԶ���ڴ�ʧ�ܣ�"));
        return FALSE;
    }

    POSITION pos = NULL;
    if(IsAddrInBpList(lpAddr, m_NorMalBpLst, pos))
    {
        PMYBREAK_POINT pBP = m_NorMalBpLst.GetAt(pos);
        *pCode = (UCHAR)pBP->dwOldOrder;
    }
    
    Decode2AsmOpcode(pCode, szAsmBuf, szOpcodeBuf,
        &unCodeSize, unCodeAddress);
    
    strOutAsm = szAsmBuf;
    dwOrderCount = unCodeSize;
    return TRUE;
}

#define OneAsmSize 10
BOOL CDeBug::ShowRemoteDisAsm(LPVOID lpAddr, DWORD& dwOutCurAddr, DWORD dwAsmCount)        //��ʾԶ�̷����
{
    DWORD dwRead = 0;
    UCHAR szBuf[MAXBYTE] = {0};
    
    BOOL bRet = FALSE;
    DWORD dwReadLen = 0;
    //BYTE btCode[MAXBYTE] = {0};
    char szAsmBuf[MAXBYTE] = {0};           //�����ָ�����
    char szOpcodeBuf[MAXBYTE] = {0};        //�����뻺����
    UINT unCodeSize = 0;
    UINT unCount = 0;
    UINT unCodeAddress = (DWORD)lpAddr;
    PBYTE pCode = szBuf;
    char szFmt[MAXBYTE *2] = {0};
    
    //��ȡԶ����Ϣ
    if(!ReadProcessMemory(m_hDstProcess, lpAddr, szBuf, RemoteOneReadSize, &dwRead))
    {
        OutErrMsg(TEXT("ShowRemoteDisAsm����ȡԶ���ڴ�ʧ�ܣ�"));
        return FALSE;
    }
    
    //ת��5�������
    DWORD dwRemaining = 0;
    while(unCount < dwAsmCount)
    {
        Decode2AsmOpcode(pCode, szAsmBuf, szOpcodeBuf,
            &unCodeSize, unCodeAddress);
        
        _tprintf(TEXT("%p:%-20s%s\r\n"), unCodeAddress, szOpcodeBuf, szAsmBuf);
        
        //         dwRemaining = 0x18 - _tcsclen(szOpcodeBuf);
        // 
        //         //���ո�
        //         while(dwRemaining--)
        //         {
        //             _puttchar(' ');
        //         }
        //         
        //         puts(szAsmBuf);
        
        
        pCode += unCodeSize;
        unCount++;
        unCodeAddress += unCodeSize;
    }
    dwOutCurAddr = unCodeAddress;
    
    return TRUE;
}

BOOL CDeBug::OnExceptionEvent()
{
    BOOL bRet = FALSE;
    EXCEPTION_RECORD& pExceptionRecord = m_DbgEvt.u.Exception.ExceptionRecord; 
    switch(pExceptionRecord.ExceptionCode)
    {
    case EXCEPTION_BREAKPOINT:          //�ϵ�
        bRet = OnBreakPointEvent();
        break;
        
    case EXCEPTION_SINGLE_STEP:         //����
        bRet = OnSingleStepEvent();
        break;
        
    case EXCEPTION_ACCESS_VIOLATION:    //C05
        bRet = OnAccessVolationEvent();
        break;
        
    }
    
    return bRet;
}

BOOL CDeBug::OnCreateProcessEvent()
{
    //������ڵ�ϵ㣬
    CREATE_PROCESS_DEBUG_INFO& pCreateEvent = m_DbgEvt.u.CreateProcessInfo;
    LPVOID lpEntryPoint = pCreateEvent.lpStartAddress;
    
    PMYBREAK_POINT ptagBp = new MYBREAK_POINT;
    ZeroMemory(ptagBp, sizeof(MYBREAK_POINT));
    
    ptagBp->bpState = BP_SYS;
    ptagBp->lpAddr = lpEntryPoint;
    ptagBp->dwCurOrder = NORMAL_CC;
    
    if(!WriteRemoteCode(lpEntryPoint, ptagBp->dwCurOrder, ptagBp->dwOldOrder))
    {
        tcout << TEXT("ϵͳ�ϵ�: ����BUG������ϵ����Ա��") << endl;
        
        //�ͷ���Դ
        if(ptagBp != NULL)
            delete ptagBp;
        
        return FALSE;
    }
    
    //��Ӷϵ�ڵ�
    m_NorMalBpLst.AddTail(ptagBp);
    
    return TRUE;
}

BOOL CDeBug::IsAddrInBpList(LPVOID lpAddr, 
                            CList<PMYBREAK_POINT, PMYBREAK_POINT&>& bpSrcLst, 
                            _OUT_ POSITION& dwOutPos,
                            BOOL bIsNextAddr)
{
    BOOL bRet = FALSE;
    POSITION pos = bpSrcLst.GetHeadPosition();
    POSITION posTmp = NULL;
    
    //��������
    while(pos)
    {
        posTmp = pos;
        MYBREAK_POINT& bp = *bpSrcLst.GetNext(pos);

        //һ��ϵ�
        if(bp.bpState == BP_NORMAL ||
           bp.bpState == BP_SYS ||
           bp.bpState == BP_ONCE)
        {
            if(bp.lpAddr == lpAddr)
            {
                bRet = TRUE;
                //���ҵ�
                dwOutPos = posTmp;
                break;
            }
        }
        //�ڴ�ϵ�
        else if(bp.bpState == BP_MEM)
        {
            //�ڴ�ϵ����з�Χ
            if(bp.lpAddr <= lpAddr || 
               (LPVOID)((DWORD)bp.lpAddr + bp.dwLen) >= lpAddr)
            {
                bRet = TRUE;
                //���ҵ�
                dwOutPos = posTmp;
                break;
            }
        }
        
    }
    
    return bRet;
}

BOOL CDeBug::ShowDllLst()
{
    POSITION pos = m_ModuleLst.GetHeadPosition();
    tcout << TEXT("==============================================================") << endl;
    //��������
    while(pos)
    {
        
        MODLST& ModLst = *m_ModuleLst.GetNext(pos);
        _tprintf(TEXT("��ַ:%08X\t·��:%s\r\n"),
            ModLst.dwBaseAddr,
            (LPCTSTR)ModLst.strPath);
        //         tcout << TEXT("��ַ:") << ios::hex << ModLst.dwBaseAddr << TEXT("\t")
        //               << TEXT("·��:") << ModLst.strPath << endl;
    }
    tcout << TEXT("==============================================================") << endl;
    return TRUE;
}

BOOL CDeBug::OnUnLoadDll()
{
    BOOL bRet = FALSE;
    POSITION pos = m_ModuleLst.GetHeadPosition();
    POSITION posTmp = NULL;
    
    //��������
    while(pos)
    {
        posTmp = pos;
        PMODLST pModLst = m_ModuleLst.GetNext(pos);
        if(m_DbgEvt.u.UnloadDll.lpBaseOfDll == (LPVOID)pModLst->dwBaseAddr)
        {
            if(pModLst != NULL)
            {
                delete pModLst;
                m_ModuleLst.RemoveAt(posTmp);
            }
            bRet = TRUE;
            break;
        }
        
    }
    return bRet;
}

BOOL CDeBug::OnLoadDll()
{
    TCHAR szBuf[MAX_PATH * 2] = {0};
    LPVOID lpString = NULL;
    
    //��������ڵ�
    LOAD_DLL_DEBUG_INFO& DllInfo = m_DbgEvt.u.LoadDll;    
    if(DllInfo.lpImageName == NULL)
    {
        return FALSE;
    }
    
    PMODLST pModLst = new MODLST;
    //RtlZeroMemory(pModLst, sizeof(MODLST));
    if(pModLst == NULL)
    {
        m_dwErrCount++;
        OutErrMsg(TEXT("OnLoadDll������ڵ�ʧ�ܣ�"));
        return FALSE;
    }
    
    //����DLL ��ַ
    pModLst->dwBaseAddr = (DWORD)DllInfo.lpBaseOfDll;
    
    //��ȡDLL ��ַ
    if (ReadProcessMemory(m_hDstProcess, DllInfo.lpImageName, \
        &lpString, sizeof(LPVOID), NULL) == NULL)
    {
        m_dwErrCount++;
        OutErrMsg(TEXT("OnLoadDll����ȡԶ�̵�ַʧ�ܣ�"));
        return FALSE;
    }
    
    //��ȡDLL·��
    if (ReadProcessMemory(m_hDstProcess, lpString, szBuf, \
        sizeof(szBuf) / sizeof(TCHAR), NULL) == NULL)
    {
        m_dwErrCount++;
        OutErrMsg(TEXT("OnLoadDll����ȡģ��·��ʧ�ܣ�"));
        return FALSE;
    }
    
    
    //ascii
    if(!DllInfo.fUnicode)
    {
        pModLst->strPath = szBuf;
    }
    //unicode
    else
    {
        //ת��UNICODEΪASCII
        _bstr_t bstrPath = (wchar_t*)szBuf;
        pModLst->strPath = (LPCTSTR)bstrPath;
    }
    
    m_ModuleLst.AddTail(pModLst);
    
    return TRUE;
}

BOOL CDeBug::WriteRemoteCode(LPVOID lpRemoteAddr, DWORD btInChar, DWORD& pbtOutChar)
{
    DWORD dwOldProtect = 0;
    DWORD dwReadLen = 0;
    
    //����ڴ汣������
    if(!VirtualProtectEx(m_hDstProcess, lpRemoteAddr, 1, PAGE_EXECUTE_READWRITE, &dwOldProtect))
    {
        m_dwErrCount++;
        OutErrMsg(TEXT("WriteRemoteCode����ȡ��������ʧ�ܣ�"));
        return FALSE;
    }
    
    //��ȡ�ɴ��벢����
    if(!ReadProcessMemory(m_hDstProcess, lpRemoteAddr, &pbtOutChar, sizeof(BYTE), &dwReadLen))
    {
        m_dwErrCount++;
        OutErrMsg(TEXT("WriteRemoteCode����ȡԶ���ڴ�ʧ�ܣ�"));
        return FALSE;
    }
    
    //д���´���
    if(!WriteProcessMemory(m_hDstProcess, lpRemoteAddr, &btInChar, sizeof(BYTE), &dwReadLen))
    {
        m_dwErrCount++;
        OutErrMsg(TEXT("WriteRemoteCode��д��Զ���ڴ�ʧ�ܣ�"));
        return FALSE;
    }
    
    //��ԭ�ڴ汣������
    if(!VirtualProtectEx(m_hDstProcess, lpRemoteAddr, 1, dwOldProtect, &dwOldProtect))
    {
        m_dwErrCount++;
        OutErrMsg(TEXT("WriteRemoteCode����ԭ��������ʧ�ܣ�"));
        return FALSE;
    }
    
    return TRUE;
}

BOOL CDeBug::CmdSetNormalBp(CMD_INFO& CmdInfo, LPVOID lpAddr)      //����һ��ϵ�
{
    if(CmdInfo.strCMD.GetLength() > 0)
    {
        //ת��������
        PTCHAR pTmp = NULL;
        int nAddr = _tcstol(CmdInfo.strCMD, &pTmp, CONVERT_HEX);
        
        //����Ƿ񳬷�Χ
        if((DWORD)nAddr > MAX_MEM)
        {
            _tprintf(TEXT("��֧�ֵĵ�ַ: %08X\r\n"), nAddr);
            return FALSE;
        }
        
        PMYBREAK_POINT ptagBp = new MYBREAK_POINT;
        if(ptagBp == NULL)
        {
            OutErrMsg(TEXT("CmdSetNormalBp���ڴ治�㣬����ϵ����Ա��"));
            m_dwErrCount++;
            tcout << TEXT("�ڴ�ϵ㣺δ֪��������ϵ����Ա��") << endl;
        }
        ZeroMemory(ptagBp, sizeof(MYBREAK_POINT));
        
        ptagBp->bpState = BP_NORMAL;
        ptagBp->lpAddr = (LPVOID)nAddr;
        ptagBp->dwCurOrder = NORMAL_CC;
        
        //����CC�ϵ�
        if(!WriteRemoteCode(ptagBp->lpAddr, ptagBp->dwCurOrder, ptagBp->dwOldOrder))
        {
            tcout << TEXT("��Ч�ڴ棬�޷����öϵ㣡@") << endl;
            
            //�ͷ���Դ
            if(ptagBp != NULL)
                delete ptagBp;
            
            return FALSE;
        }
        
        //��Ӷϵ�ڵ�
        m_NorMalBpLst.AddTail(ptagBp);
    }
    else
    {
        tcout << TEXT("һ��ϵ���Ҫ������") << endl;
        return FALSE;
    }
    
    return TRUE;
}


BOOL CDeBug::CmdShowHardBpLst(CMD_INFO& CmdInfo, LPVOID lpAddr)    //��ʾӲ���ϵ�
{    
    DWORD dwCount = 0;
    POSITION pos = m_HardBpLst.GetHeadPosition();
    tcout << TEXT("=====================Ӳ���ϵ�=====================") << endl;
    if(m_HardBpLst.IsEmpty())
    {
        tcout << TEXT("����") << endl;
    }
    else
    {
        while(pos)
        {
            MYBREAK_POINT& Bp = *m_HardBpLst.GetNext(pos);
            _tprintf(TEXT("\t��ţ�%d\t��ַ��0x%p\t"), dwCount++, (DWORD)Bp.lpAddr);
            switch(Bp.hbpStatus)
            {
            case BP_HARD_EXEC:
                _tprintf(TEXT("״̬��Ӳ��ִ��\r\n"));
                break;
            case BP_HARD_READ:
                _tprintf(TEXT("״̬��Ӳ����ȡ\r\n"));
                break;
            case BP_HARD_WRITE:
                _tprintf(TEXT("״̬��Ӳ��д��\r\n"));
                break;
            }
        }
    }
    
    tcout << TEXT("=====================Ӳ���ϵ�=====================") << endl;
    
    return TRUE;
}

BOOL CDeBug::CmdClearHardBpLst(CMD_INFO& CmdInfo, LPVOID lpAddr)    //��ʾӲ���ϵ�
{
    CmdShowHardBpLst(CmdInfo, lpAddr);
    if(!m_HardBpLst.IsEmpty())
    {
        BOOL bIsDel = FALSE;
        DWORD dwNum = 0;
        DWORD dwLstCount = m_HardBpLst.GetCount();
        
        while(TRUE)
        {
            //��ȡ�û�����
            tcout << TEXT("�������ţ�") ;
            _tscanf(TEXT("%d"), &dwNum);
            if(dwNum < 0 || dwNum >= dwLstCount)
            {
                tcout << TEXT("����������") << endl;
                continue;
            }
            
            //��������
            POSITION pos = m_HardBpLst.GetHeadPosition();
            POSITION posTmp = NULL;
            while(pos)
            {
                posTmp = pos;
                MYBREAK_POINT& bp = *m_HardBpLst.GetNext(pos);
                
                if(0 == dwNum--)
                {
                    //�Ƴ��ڵ�
                    m_HardBpLst.RemoveAt(posTmp);

                    //�������һ��Ӳ��Ϊ��
                    MYDR7& Dr7 = *(PMYDR7)(&m_DstContext.Dr7);
                    switch(m_HardBpLst.GetCount())
                    {
                    case 0:
                        m_DstContext.Dr0 = 0;   //��д��ַ
                        Dr7.L0 = HBP_UNSET;                   //����Ӳ���ϵ�����
                        Dr7.RW0 = 0;                //����Ӳ���ϵ��״̬
                        Dr7.LEN0 = 0;                   //�ϵ㳤�� 1 2 4
                        break;
                        
                    case 1:
                        m_DstContext.Dr1 = 0;
                        Dr7.L1 = HBP_UNSET;
                        Dr7.RW1 = 0;
                        Dr7.LEN1 = 0;
                        break;
                        
                    case 2:
                        m_DstContext.Dr2 = 0;
                        Dr7.L2 = HBP_UNSET;
                        Dr7.RW2 = 0;
                        Dr7.LEN2 = 0;
                        break;

                    case 3:
                        m_DstContext.Dr2 = 0;
                        Dr7.L3 = HBP_UNSET;
                        Dr7.RW3 = 0;
                        Dr7.LEN3 = 0;
                        break;
                    }//End Switch
                    
                    //����Ӳ���ϵ�
                    POSITION posHBP = m_HardBpLst.GetHeadPosition();
                    DWORD dwNodeCount = 0;

                    //��������
                    while(posHBP)
                    {
                        MYBREAK_POINT& hBP = *m_HardBpLst.GetNext(posHBP);
                        DWORD dwBPState = 0;

                        //��ȡ״̬
                        switch(hBP.hbpStatus)
                        {
                        case BP_HARD_EXEC:
                            dwBPState = HBP_INSTRUCTION_EXECUT;
                            break;
                        case BP_HARD_WRITE:
                            dwBPState = HBP_DATAS_READS_WRITES;
                            break;
                        case BP_HARD_READ:
                            dwBPState = HBP_DATAS_READS_WRITES;
                            break;
                        }//End Switch
                        
                        //����Ӳ��
                        switch(dwNodeCount++)
                        {
                        case 0:
                            m_DstContext.Dr0 = (DWORD)lpAddr;   //��д��ַ
                            Dr7.L0 = HBP_SET;                   //����Ӳ���ϵ�����
                            Dr7.RW0 = dwBPState;                //����Ӳ���ϵ��״̬
                            Dr7.LEN0 = hBP.dwLen;                   //�ϵ㳤�� 1 2 4
                            break;
        
                        case 1:
                            m_DstContext.Dr1 = (DWORD)lpAddr;
                            Dr7.L1 = HBP_SET;
                            Dr7.RW1 = dwBPState;
                            Dr7.LEN1 = hBP.dwLen;
                            break;
        
                        case 2:
                            m_DstContext.Dr2 = (DWORD)lpAddr;
                            Dr7.L2 = HBP_SET;
                            Dr7.RW2 = dwBPState;
                            Dr7.LEN2 = hBP.dwLen;
                            break;
                        }//End Switch
                    }//End  while(posHBP)
                    
                    bIsDel = TRUE;
                    break;
                }//End if(0 == dwNum--)
            }//End while(pos)
            if(bIsDel)
            {
                tcout << TEXT("�ɹ�") << endl;
                break;
            }
        }
    }
    return TRUE;
}

BOOL CDeBug::CmdSetHardBp(CMD_INFO& CmdInfo, LPVOID lpAddr)        //����Ӳ���ϵ�
{
    BOOL bRet = TRUE;
    //�ж�Ӳ���ϵ����û��λ��
    if(m_HardBpLst.GetCount() < 4)
    {
        int nPos = CmdInfo.strCMD.Find(TEXT(' '));
        if(nPos != -1)
        {
            //���ָ��
            int nAddr = 0;
            PCHAR pNoUse = NULL;
            CString strAddr = CmdInfo.strCMD.Left(nPos);
            CString strOther = CmdInfo.strCMD.Right(CmdInfo.strCMD.GetLength() - nPos - 1);

            //�жϵ�ַ�Ƿ񳬷�Χ
            nAddr = _tcstol(strAddr, &pNoUse, CONVERT_HEX);
            if(nAddr < 0 || (DWORD)nAddr > MAX_MEM)
            {
                tcout << TEXT("��ַ����Χ") << endl;
                return TRUE;
            }
            
            nPos = strOther.Find(TEXT(' '));
            if(nPos != -1)
            {
                strOther.Right(strOther.GetLength() - nPos - 1);
                
                //��ֲ�����
                nPos = strOther.Find(TEXT(' '));
                if(nPos != -1)
                {
                    int nNum = 0;
                    CString strOperate = strOther.Left(nPos);
                    CString strNum = strOther.Right(strOther.GetLength() - nPos - 1);
                    if(strNum.GetLength() > 0)
                    {
                        nNum = _tcstol(strNum, &pNoUse, CONVERT_HEX);
                        if(nNum < 1 || nNum > 4 || nNum == 3)
                        {
                            tcout << TEXT("��δ֧�ֵ�����") << endl;
                            return TRUE;
                        }
                    }
                    else
                    {
                        tcout << TEXT("��δ֧�ֵ�����") << endl;
                        return TRUE;
                    }

                    //Ӳ��ִ��
                    if(strOperate == TEXT("e"))
                    {
                        bRet = SetHardBreakPoint((LPVOID)nAddr, BP_HARD_EXEC);
                    }
                    else if(strOperate == TEXT("r"))
                    {
                        bRet = SetHardBreakPoint((LPVOID)nAddr, BP_HARD_READ, nNum);
                    }
                    else if(strOperate == TEXT("w"))
                    {
                        bRet = SetHardBreakPoint((LPVOID)nAddr, BP_HARD_WRITE, nNum);
                    }
                }
                else
                {
                    tcout << TEXT("��δ֧�ֵ�����") << endl;
                }
            }
            //Ӳ��ִ��
            else if(strOther.Find(TEXT("e")) != -1)
            {
                bRet = SetHardBreakPoint((LPVOID)nAddr, BP_HARD_EXEC);
            }
            else
            {
                tcout << TEXT("��δ֧�ֵ�����") << endl;
            }
        }
        else
        {
            tcout << TEXT("��δ֧�ֵ�����, ��ָ���ϵ����� R W E") << endl;
        }
    }
    else
    {
        tcout << TEXT("��ֻ֧��4��Ӳ���ϵ�!") << endl;
    }
    return bRet;
}


BOOL CDeBug::SetHardBreakPoint(LPVOID lpAddr, BPSTATE bpState, DWORD dwLen)  //����Ӳ���ϵ�
{
    PMYBREAK_POINT pBP = new MYBREAK_POINT;
    if(pBP == NULL)
    {
        OutErrMsg(TEXT("SetHardBreakPoint: ����ڵ�ʧ�ܣ�"));

        return FALSE;
    }
    ZeroMemory(pBP, sizeof(MYBREAK_POINT));

    pBP->lpAddr = lpAddr;
    pBP->bpState = BP_HARDWARE;
    pBP->hbpStatus = bpState;
    pBP->dwLen = dwLen;
    
    DWORD dwBPState = 0;
    switch(bpState)
    {
    case BP_HARD_EXEC:
        dwBPState = HBP_INSTRUCTION_EXECUT;
        break;
    case BP_HARD_WRITE:
        dwBPState = HBP_DATAS_READS_WRITES;
        break;
    case BP_HARD_READ:
        dwBPState = HBP_DATAS_READS_WRITES;
        break;
    }
    
    MYDR7& Dr7 = *(PMYDR7)(&m_DstContext.Dr7);
    switch(m_HardBpLst.GetCount())
    {
    case 0:
        m_DstContext.Dr0 = (DWORD)lpAddr;   //��д��ַ
        Dr7.L0 = HBP_SET;                   //����Ӳ���ϵ�����
        Dr7.RW0 = dwBPState;                //����Ӳ���ϵ��״̬
        Dr7.LEN0 = dwLen;                   //�ϵ㳤�� 1 2 4
        break;

    case 1:
        m_DstContext.Dr1 = (DWORD)lpAddr;
        Dr7.L1 = HBP_SET;
        Dr7.RW1 = dwBPState;
        Dr7.LEN1 = dwLen;
        break;

    case 2:
        m_DstContext.Dr2 = (DWORD)lpAddr;
        Dr7.L2 = HBP_SET;
        Dr7.RW2 = dwBPState;
        Dr7.LEN2 = dwLen;
        break;

    case 3:
        m_DstContext.Dr3 = (DWORD)lpAddr;
        Dr7.L3 = HBP_SET;
        Dr7.RW3 = dwBPState;
        Dr7.LEN3 = dwLen;
        break;
    }
    
    m_HardBpLst.AddTail(pBP);


    return TRUE;
}

BOOL CDeBug::CmdShowMemBpLst(CMD_INFO& CmdInfo, LPVOID lpAddr)     //��ʾ�ڴ�ϵ�
{
    DWORD dwCount = 0;
    POSITION pos = m_MemBpLst.GetHeadPosition();
    tcout << TEXT("=====================�ڴ�ϵ�=====================") << endl;
    if(m_MemBpLst.IsEmpty())
    {
        tcout << TEXT("����") << endl;
    }
    else
    {
        while(pos)
        {
            MYBREAK_POINT& Bp = *m_MemBpLst.GetNext(pos);
            _tprintf(TEXT("\t��ţ�%d\t��ַ��0x%p\t���ȣ�%d\r\n"), 
                    dwCount++, 
                    (DWORD)Bp.lpAddr, 
                    Bp.dwLen);
        }
    }
    
    tcout << TEXT("=====================�ڴ�ϵ�=====================") << endl;
    
    return TRUE;
}

BOOL CDeBug::CmdClearMemBp(CMD_INFO& CmdInfo, LPVOID lpAddr)       //ɾ���ڴ�ϵ�
{
    CmdShowMemBpLst(CmdInfo, lpAddr);
    if(!m_MemBpLst.IsEmpty())
    {
        BOOL bIsDel = FALSE;
        DWORD dwNum = 0;
        DWORD dwLstCount = m_MemBpLst.GetCount();
        
        while(TRUE)
        {
            //��ȡ�û�����
            tcout << TEXT("�������ţ�") ;
            _tscanf(TEXT("%d"), &dwNum);
            if(dwNum < 0 || dwNum >= dwLstCount)
            {
                tcout << TEXT("����������") << endl;
                continue;
            }
            
            //��������
            POSITION pos = m_MemBpLst.GetHeadPosition();
            POSITION posTmp = NULL;
            while(pos)
            {
                posTmp = pos;
                MYBREAK_POINT& bp = *m_MemBpLst.GetNext(pos);
                
                if(0 == dwNum--)
                {
                    DWORD dwOldProtect = 0;

                    //�޲��ϵ��ȡ���ڴ�����
                    if(VirtualProtectEx(m_hDstProcess, bp.lpAddr, bp.dwLen, bp.dwOldProtect, &dwOldProtect))
                    {
                        //�Ƴ��ڵ�
                        m_MemBpLst.RemoveAt(posTmp);
                        bIsDel = TRUE;
                    }
                    else
                    {
                        tcout << TEXT("�ڴ��������") << endl;
                        OutErrMsg(TEXT("CmdClearNormalBp���޲��ϵ�ʧ��"));
                        return FALSE;
                    }
                }
            }
            if(bIsDel)
            {
                tcout << TEXT("�ɹ�") << endl;
                break;
            }
        }
    }
    return TRUE;
}

BOOL CDeBug::CmdSetMemBp(CMD_INFO& CmdInfo, LPVOID lpAddr)         //�����ڴ�ϵ�
{
    if(CmdInfo.strCMD.GetLength() > 0)
    {
        //�������
        int nPos = CmdInfo.strCMD.Find(TEXT(' '));
        if(nPos != -1)
        {
            CString strAddr = CmdInfo.strCMD.Left(nPos);
            CString strSize = CmdInfo.strCMD.Right(CmdInfo.strCMD.GetLength() - nPos - 1);

            //ת��������
            PTCHAR pTmp = NULL;
            int nAddr = _tcstol(strAddr, &pTmp, CONVERT_HEX);
            
            //����Ƿ񳬷�Χ
            if((DWORD)nAddr > MAX_MEM)
            {
                _tprintf(TEXT("��֧�ֵĵ�ַ: %08X\r\n"), nAddr);
                return FALSE;
            }
            
            
            int nSize = _tcstol(strSize, &pTmp, CONVERT_HEX);
            
            //��ѯ��ҳ��Ϣ
            MEMORY_BASIC_INFORMATION mbi;  //�����Գ�����ڴ���Ϣ
            if(!VirtualQueryEx(m_hDstProcess, (LPVOID)nAddr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)))
            {
                _tprintf(TEXT("��֧�ֵĵ�ַ: %08X\r\n"), nAddr);
                return FALSE;
            }
            
            //�ж��ڴ�ϵ��Ƿ���ҳ
            if((DWORD)(nAddr + nSize) > (DWORD)((UINT)mbi.BaseAddress + mbi.RegionSize))
            {
                _tprintf(TEXT("��֧�ֵĵ�ַ: 0x%08X ���ȣ�0x%08X\r\n"), nAddr, nSize);
                return FALSE;
            }
            

            PMYBREAK_POINT ptagBp = new MYBREAK_POINT;
            if(ptagBp == NULL)
            {
                OutErrMsg(TEXT("CmdSetNormalBp���ڴ治�㣬����ϵ����Ա��"));
                m_dwErrCount++;
                tcout << TEXT("�ڴ�ϵ㣺δ֪��������ϵ����Ա��") << endl;
            }

            ZeroMemory(ptagBp, sizeof(MYBREAK_POINT));
            
            //��д�ڵ���Ϣ
            ptagBp->bpState = BP_MEM;
            ptagBp->lpAddr = (LPVOID)nAddr;
            ptagBp->dwLen = (DWORD)nSize;
            
            //�����ڴ�ϵ㣬��ȡ�ڴ�����
            if(!VirtualProtectEx(m_hDstProcess, 
                (LPVOID)nAddr, 
                (DWORD)nSize, 
                PAGE_NOACCESS, 
                &ptagBp->dwOldProtect))
            {
                tcout << TEXT("�����ڴ�ϵ�ʧ��") << endl;
                m_dwErrCount++;
                return FALSE;
            }
            
            //��Ӷϵ�ڵ�
            m_MemBpLst.AddTail(ptagBp);
        }//End if(nPos != -1)
        else
        {
            tcout << TEXT("�ڴ�ϵ���Ҫָ������") << endl;
            return FALSE;
        }
        
    }//End if(CmdInfo.strCMD.GetLength() > 0)
    else
    {
        tcout << TEXT("�ڴ�ϵ���Ҫ������") << endl;
        return FALSE;
    }
    
    return TRUE;
}

BOOL CDeBug::CmdSetOneStepInto(CMD_INFO& CmdInfo, LPVOID lpAddr)       //���õ���
{
    m_bIsMyStepInto = TRUE;
    
    //���õ�����־λ
    EFLAGS& eFlag = *(PEFLAGS)&m_DstContext.EFlags;
    //m_DstContext.EFlags |= TF;

    if(eFlag.dwTF != 1)
    {
        eFlag.dwTF = 1;
    }
    
    return TRUE;
}

BOOL CDeBug::CmdSetOneStepOver(CMD_INFO& CmdInfo, LPVOID lpAddr)   //��������
{
    DWORD dwCount = 0;
    CString strAsm = TEXT("");
    

    if(!GetOneAsm(lpAddr, dwCount, strAsm))
        m_dwErrCount++;
    
    strAsm.MakeLower();

    if(strAsm.Find(TEXT("call")) != -1)
    {
        PMYBREAK_POINT ptagBp = new MYBREAK_POINT;
        ZeroMemory(ptagBp, sizeof(MYBREAK_POINT));
        
        ptagBp->bpState = BP_ONCE;
        ptagBp->lpAddr = (LPVOID)((DWORD)lpAddr + dwCount);
        ptagBp->dwCurOrder = NORMAL_CC;
        
        if(!WriteRemoteCode(ptagBp->lpAddr, ptagBp->dwCurOrder, ptagBp->dwOldOrder))
        {
            tcout << TEXT("ϵͳ�ϵ�: ����BUG������ϵ����Ա��") << endl;
            
            //�ͷ���Դ
            if(ptagBp != NULL)
                delete ptagBp;
            
            return FALSE;
        }
        
        //��Ӷϵ�ڵ�
        m_NorMalBpLst.AddTail(ptagBp);
        
    }
    else
    {
        m_bIsMyStepInto = TRUE;
        
        //���õ�����־λ
        //m_DstContext.EFlags |= TF;
        //���õ�����־λ
        EFLAGS& eFlag = *(PEFLAGS)&m_DstContext.EFlags;
        //m_DstContext.EFlags |= TF;
        
        if(eFlag.dwTF != 1)
        {
            eFlag.dwTF = 1;
        }
    }
    return TRUE;
}


BOOL CDeBug::CmdShowReg(CMD_INFO& CmdInfo, LPVOID lpAddr)  //��ʾ
{
    BOOL bRet = TRUE;
    TCHAR szBuf[MAXBYTE] = {0};
    DWORD dwTmpReg = 0;
    bRet = ShowRemoteReg();
    while(TRUE)
    {
        //��ȡ�û�����
        tcout << TEXT("-������Ĵ���:");
        
        
        tcin.clear();
        tcin.sync();
        tcin.getline(szBuf, 4, TEXT('\n'));
        tcin.clear();
        tcin.sync();
        
        //_tscanf(TEXT("%[a-z, A-Z]4s"), szBuf);
        CString strBuf = szBuf;
        strBuf.MakeUpper();
        
        //EAX
        if(strBuf == TEXT("EAX"))
        {
            tcout << TEXT("-EAX: ");
            _tscanf("%X", &dwTmpReg);
            m_DstContext.Eax = dwTmpReg;
        }
        //EBX
        else if(strBuf == TEXT("EBX"))
        {
            tcout << TEXT("-EBX: ");
            _tscanf("%X", &dwTmpReg);
            m_DstContext.Ebx = dwTmpReg;
        }
        //ECX
        else if(strBuf == TEXT("ECX"))
        {
            tcout << TEXT("-ECX: ");
            _tscanf("%X", &dwTmpReg);
            m_DstContext.Ecx = dwTmpReg;
        }
        //EDX
        else if(strBuf == TEXT("EDX"))
        {
            tcout << TEXT("-EDX: ");
            _tscanf("%X", &dwTmpReg);
            m_DstContext.Edx = dwTmpReg;
        }
        //ESI
        else if(strBuf == TEXT("ESI"))
        {
            tcout << TEXT("-ESI: ");
            _tscanf("%X", &dwTmpReg);
            m_DstContext.Edx = dwTmpReg;
        }
        //EDI
        else if(strBuf == TEXT("EDI"))
        {
            tcout << TEXT("-EDI: ");
            _tscanf("%X", &dwTmpReg);
            m_DstContext.Edx = dwTmpReg;
        }
        //�س�
        else if(strBuf.GetLength() == 0)
        {
            break;
        }
        else
        {
            tcout << TEXT("��Ч����") << endl;
            continue;
        }
        
    }
    return bRet;
}


BOOL CDeBug::CmdShowNormalBpLst(CMD_INFO& CmdInfo, LPVOID lpAddr)  //��ʾһ��ϵ�
{
    DWORD dwCount = 0;
    POSITION pos = m_NorMalBpLst.GetHeadPosition();
    tcout << TEXT("=====================һ��ϵ�=====================") << endl;
    if(m_NorMalBpLst.IsEmpty())
    {
        tcout << TEXT("����") << endl;
    }
    else
    {
        while(pos)
        {
            MYBREAK_POINT& Bp = *m_NorMalBpLst.GetNext(pos);
            _tprintf(TEXT("\t\t��ţ�%d\t��ַ��0x%p\r\n"), dwCount++, (DWORD)Bp.lpAddr);
        }
    }
    
    tcout << TEXT("=====================һ��ϵ�=====================") << endl;
    
    return TRUE;
}

BOOL CDeBug::CmdClearNormalBp(CMD_INFO& CmdInfo, LPVOID lpAddr)
{
    CmdShowNormalBpLst(CmdInfo, lpAddr);
    if(!m_NorMalBpLst.IsEmpty())
    {
        BOOL bIsDel = FALSE;
        DWORD dwNum = 0;
        DWORD dwLstCount = m_NorMalBpLst.GetCount();
        
        while(TRUE)
        {
            //��ȡ�û�����
            tcout << TEXT("�������ţ�") ;
            _tscanf(TEXT("%d"), &dwNum);
            if(dwNum < 0 || dwNum >= dwLstCount)
            {
                tcout << TEXT("����������") << endl;
                continue;
            }
            
            //��������
            POSITION pos = m_NorMalBpLst.GetHeadPosition();
            POSITION posTmp = NULL;
            while(pos)
            {
                posTmp = pos;
                MYBREAK_POINT& bp = *m_NorMalBpLst.GetNext(pos);
                
                if(0 == dwNum--)
                {
                    //�޲��ϵ��ȡ�Ĵ���
                    if(WriteRemoteCode(bp.lpAddr, bp.dwOldOrder, bp.dwCurOrder))
                    {
                        //�Ƴ��ڵ�
                        m_NorMalBpLst.RemoveAt(posTmp);
                        bIsDel = TRUE;
                    }
                    else
                    {
                        tcout << TEXT("�ڴ��������") << endl;
                        OutErrMsg(TEXT("CmdClearNormalBp���޲��ϵ�ʧ��"));
                        return FALSE;
                    }
                }
            }
            if(bIsDel)
            {
                tcout << TEXT("�ɹ�") << endl;
                break;
            }
        }
    }
    return TRUE;
}

BOOL CDeBug::ShowMemLst(CMD_INFO& CmdInfo, LPVOID lpAddr)          //��ʾ�ڴ��б�
{
    MEMORY_BASIC_INFORMATION mbi;  //�����Գ�����ڴ���Ϣ
    PBYTE pAddress = NULL;
    
    _tprintf(_T("BaseAddr    Size    Type    State    AllocProtect    Protect\r\n"));
    
    while (true)
    {
        if (VirtualQueryEx(m_hDstProcess, pAddress, &mbi, sizeof(mbi)) != \
            sizeof(mbi))
        {
            break;
        }
        if ((mbi.AllocationBase != mbi.BaseAddress) && (mbi.State != MEM_FREE))
        {
            _tprintf(_T("%08x  %08x  "), mbi.BaseAddress, mbi.RegionSize);
        }
        else
        {
            _tprintf(_T("%08x  %08x  "), mbi.BaseAddress, mbi.RegionSize);
        }
        
        switch (mbi.Type)  //�ڴ������ MEM_IMAGE MEM_MAPPED MEM_PRIVATE
        {
        case MEM_IMAGE: 
            _tprintf(_T("%-8s"), _T("Imag")); 
            break;
        case MEM_MAPPED: 
            _tprintf(_T("%-8s"), _T("Map"));
            break;
        case MEM_PRIVATE: 
            _tprintf(_T("%-8s"), _T("Priv"));
            break;
        default:
            _tprintf(_T("%-8s"), _T(" --"));
            break;
        }
        
        switch (mbi.State)  //�ڴ��״̬
        {
        case MEM_COMMIT: 
            _tprintf(_T("%-13s"), _T("COMMIT")); 
            break;
        case MEM_RESERVE: 
            _tprintf(_T("%-13s"), _T("RESERVE"));
            break;
        case MEM_FREE: 
            _tprintf(_T("%-13s"), _T("FREE "));
            break;
        default:
            _tprintf(_T("%-13s")_T("--"));
            break;
        }
        
        switch (mbi.AllocationProtect)  //�ڴ���³��α���ʱ�ı�������
        {
        case PAGE_READONLY:
            _tprintf(_T("%-12s"), _T("R_ONLY"));
            break;
        case PAGE_READWRITE:
            _tprintf(_T("%-12s"), _T("R/W"));
            break;
        case PAGE_WRITECOPY: 
            _tprintf(_T("%-12s"), _T("W/COPY "));
            break;
        case PAGE_EXECUTE: 
            _tprintf(_T("%-12s"), _T("E"));
            break;
        case PAGE_EXECUTE_READ:
            _tprintf(_T("%-12s"), _T("E/R"));
            break;
        case PAGE_EXECUTE_READWRITE:
            _tprintf(_T("%-12s"), _T("E/R/W"));
            break;
        case PAGE_EXECUTE_WRITECOPY: 
            _tprintf(_T("%-12s"), _T("E/W/COPY"));
            break;
        case PAGE_GUARD: 
            _tprintf(_T("%-12s"), _T("GUARD "));
            break;
        case PAGE_NOACCESS: 
            _tprintf(_T("%-12s"), _T("NOACCESS "));
            break;
        case PAGE_NOCACHE: 
            _tprintf(_T("%-12s"), _T("NOCACHE "));
            break;
        default: 
            _tprintf(_T("%-12s"), _T("--"));
            break;
        }
        
        switch (mbi.Protect)  //�ڴ������
        {
        case PAGE_READONLY:
            _tprintf(_T("%s"), _T("R_ONLY"));
            break;
        case PAGE_READWRITE:
            _tprintf(_T("%s"), _T("R/W"));
            break;
        case PAGE_WRITECOPY:
            _tprintf(_T("%s"), _T("W/COPY "));
            break;
        case PAGE_EXECUTE:
            _tprintf(_T("%s"), _T("E"));
            break;
        case PAGE_EXECUTE_READ:
            _tprintf(_T("%s"), _T("E/R"));
            break;
        case PAGE_EXECUTE_READWRITE:
            _tprintf(_T("%s"), _T("E/R/W"));
            break;
        case PAGE_EXECUTE_WRITECOPY:
            _tprintf(_T("%s"), _T("E/W/COPY"));
            break;
        case PAGE_GUARD:
            _tprintf(_T("%s"), _T("GUARD "));
            break;
        case PAGE_NOACCESS:
            _tprintf(_T("%s"), _T("NOACCESS "));
            break;
        case PAGE_NOCACHE:
            _tprintf(_T("%s"), _T("NOCACHE "));
            break;
        default:
            _tprintf(_T("%s"), _T("--"));
            break;
        }
        _tprintf(_T("\r\n"));
        
        pAddress = ((PBYTE)mbi.BaseAddress + mbi.RegionSize);
    }
    
    return TRUE;
}

BOOL CDeBug::HandleCmd(CMD_INFO& CmdInfo, LPVOID lpAddr)          //ִ������
{
    BOOL bIsBreak = FALSE;
    switch(CmdInfo.dwState)
    {
        //��������
    case CMD_STEP:
        CmdSetOneStepInto(CmdInfo, lpAddr);
        bIsBreak = TRUE;
        break;
        
        //��������
    case CMD_STEPGO:
        CmdSetOneStepOver(CmdInfo, lpAddr);
        bIsBreak = TRUE;
        break;
        
        //����
    case CMD_RUN:
        bIsBreak = TRUE;
        break;
        
        //����
    case CMD_TRACE:
        break;
        
        //��ʾ�����
    case CMD_DISPLAY_ASMCODE:
        CmdShowAsm(CmdInfo, lpAddr);
        break;
        
        //��ʾ�ڴ�
    case CMD_DISPLAY_DATA:
        CmdShowMem(CmdInfo, lpAddr);
        break;
        
        //�Ĵ���
    case CMD_REGISTER:
        CmdShowReg(CmdInfo, lpAddr);
        break;
        
        //�޸��ڴ�  
    case CMD_EDIT_DATA:
        break;
        
        //һ��ϵ�
    case CMD_BREAK_POINT:
        CmdSetNormalBp(CmdInfo, lpAddr);
        break;
        
        //һ��ϵ��б�
    case CMD_BP_LIST:
        CmdShowNormalBpLst(CmdInfo, lpAddr);
        break;
        
        //���һ��ϵ�
    case CMD_CLEAR_NORMAL:
        CmdClearNormalBp(CmdInfo, lpAddr);
        break;
        
        //Ӳ���ϵ�
    case CMD_BP_HARD:
        CmdSetHardBp(CmdInfo, lpAddr);
        break;
        
        //Ӳ���ϵ��б�
    case CMD_BP_HARD_LIST:
        CmdShowHardBpLst(CmdInfo, lpAddr);
        break;
        
        //���Ӳ���ϵ��б�
    case CMD_CLEAR_BP_HARD:
        CmdClearHardBpLst(CmdInfo, lpAddr);
        break;
        
        //�ڴ�ϵ�
    case CMD_BP_MEMORY:
        CmdSetMemBp(CmdInfo, lpAddr);
        break;
        
        //�ڴ�ϵ��б�
    case CMD_BP_MEMORY_LIST:
        CmdShowMemBpLst(CmdInfo, lpAddr);
        break;
        
        //�ڴ��ҳ�ϵ��б�
    case CMD_BP_PAGE_LIST:
        break;
        
        //����ڴ�ϵ�
    case CMD_CLEAR_BP_MEMORY:
        CmdClearMemBp(CmdInfo, lpAddr);
        break;
        
        //���ؽű�
    case CMD_LOAD_SCRIPT:
        LoadScript(CmdInfo, lpAddr);
        break;
        
        //�����ű�
    case CMD_EXPORT_SCRIPT:
        SaveScript(CmdInfo, lpAddr);
        break;
        
        //�˳�����
    case CMD_QUIT:
        tcout << TEXT("ллʹ�ã�") << endl;
        system("pause");
        ExitProcess(0);
        break;
        
        //ģ���б�
    case CMD_MODULE_LIST:
        ShowDllLst();
        break;
        
        //�ڴ��б�
    case CMD_MEM_INFO_LIST:
        ShowMemLst(CmdInfo, lpAddr);
        break;
        
        //Ĭ������Ч
    case CMD_INVALID:
    default:
        break;
    }
    
    CmdInfo.bIsBreakInputLoop = bIsBreak;
    return TRUE;
}
