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
        tcout << TEXT("获取函数失败！请联系管理员！") << endl;
        return FALSE;
    }
    
    return TRUE;
}

BOOL CDeBug::Start(TCHAR* argv[])			//程序开始
{
    BOOL bRet = FALSE;
    CString strArgv = argv[1];
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    
    if(strArgv.Find(TEXT(".exe")) == -1)
    {
        //建立调试关系
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
            tcout << TEXT("请输入正确的被调试程序名！") << endl;
            return FALSE;
        }
    }
    else
    {
        //建立调试关系
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

BOOL CDeBug::EventLoop()       //消息循环
{
    DWORD bpState = DBG_EXCEPTION_NOT_HANDLED;
    BOOL bRet = FALSE;
    
    while(TRUE == WaitForDebugEvent(&m_DbgEvt, INFINITE))
    {
        if(m_dwErrCount > 10)
        {
            tcout << TEXT("错误次数过多，请联系管理员！") << endl;
            break;
        }
        
        m_hDstProcess = OpenProcess(PROCESS_ALL_ACCESS, 
            FALSE, 
            m_DbgEvt.dwProcessId);
        if(m_hDstProcess == NULL)
        {
            tcout << TEXT("打开调试进程失败！") << endl;
            m_dwErrCount++;
            continue;
        }
        
        m_hDstThread = m_pfnOpenThread(THREAD_ALL_ACCESS, 
            FALSE, 
            m_DbgEvt.dwThreadId);
        if(m_hDstThread == NULL)
        {
            tcout << TEXT("打开调试进程失败！") << endl;
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
            tcout << TEXT("被调试程序已退出!") << endl;	
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
            tcout << TEXT("被调试程序有调试信息输出，调试器并未进行捕获.") << endl;		
            break;
            
        default:
            break;
        }
        
        //如果已经处理，则返回已处理，否则默认返回没处理
        if(bRet)
            bpState = DBG_CONTINUE;
        
        //m_DstContext.Dr6 = 0;
        
        //设置线程上下文
        if(!SetThreadContext(m_hDstThread, &m_DstContext))
        {
            tcout << TEXT("设置线程信息失败，请联系管理员") << endl;
        }
        
        //关闭进程句柄
        if (m_hDstProcess != NULL)
        {
            CloseHandle(m_hDstProcess);
            m_hDstProcess = NULL;
        }
        
        //关闭线程句柄
        if (m_hDstThread != NULL)
        {
            CloseHandle(m_hDstThread);
            m_hDstThread = NULL;
        }
        
        //设置处理状态
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
BOOL CDeBug::Interaction(LPVOID lpAddr, BOOL bIsShowDbgInfo)                //人机交互
{   
    //检查地址是否越界
    if((DWORD)lpAddr >= MAX_MEM)
    {
        tcout << TEXT("无效区域，超出程序领空！") << endl;
        return TRUE;
    }
    
    if(bIsShowDbgInfo)
    {
        //显示当前调试信息
        if(!ShowCurAllDbg(lpAddr))
        {
            m_dwErrCount++;
            OutErrMsg(TEXT("Interaction：未知程序显示错误！"));
            return FALSE;
        }
    }
    
    //初始化获取用户输入
    CMD_INFO CmdInfo;
    //ZeroMemory(&CmdInfo, sizeof(CMD_INFO));
    CmdInfo.dwState = CMD_INVALID;
    CmdInfo.bIsBreakInputLoop = FALSE;
    CmdInfo.dwPreAddr = NULL;
    
    while(TRUE)
    {
        //获取用户输入
        BOOL bRet = GetUserInput(CmdInfo);
        if(bRet == FALSE)
        {
            m_dwErrCount++;
            OutErrMsg(TEXT("Interaction：未知程序输入错误！"));
            return FALSE;
        }
        else if(!m_bIsScript && !m_bIsInput)
        {
            m_bIsInput = TRUE;
            continue;
        }
        
        
        //处理用户输入
        if(!HandleCmd(CmdInfo, lpAddr))
        {
            m_dwErrCount++;
            OutErrMsg(TEXT("Interaction：未知程序执行错误！"));
            return FALSE;
        }
        
        if(CmdInfo.bIsBreakInputLoop)
            break;
    }
    
    return TRUE;
}

#define  FILE_NAME TEXT("out.txt")
BOOL CDeBug::SaveScript(CMD_INFO& CmdInfo, LPVOID lpAddr)          //保存脚本
{
//     TCHAR szFileName[MAX_PATH] = {0};
//     tcout << TEXT("请输入文件名") ;
//     _tscanf(TEXT("%255s"), szFileName);

    //回车换行
    char szBuf[3] = {0x0d, 0x0a, 0};
    
    try
    {
        CStdioFile File;
        if(!File.Open(FILE_NAME, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary | CFile::shareDenyRead))
        {
            tcout << TEXT("创建文件失败") << endl;
            return FALSE;
        }
        
        //删除最后一个命令
        m_CmdOrderLst.RemoveTail();
        
        //遍历链表
        POSITION pos = m_CmdOrderLst.GetHeadPosition();
        while(pos)
        {
            CString & strOut = *m_CmdOrderLst.GetNext(pos);
            File.Write((LPCTSTR)strOut, strOut.GetLength());
            File.Write(szBuf, sizeof(szBuf) - 1);
            File.Flush();
        }
        File.Close();
        tcout << TEXT("导出成功") << endl;
    }
    catch (CFileException* e)
    {
        TCHAR szBuf[MAXBYTE] = {0};

        try
        {
            e->GetErrorMessage(szBuf, MAXBYTE);
            tcout << TEXT("文件操作出错") << endl;
            tcout << szBuf << endl;
        }
        catch(...)
        {}
    }

    

    return TRUE;
}

BOOL CDeBug::LoadScript(CMD_INFO& CmdInfo, LPVOID lpAddr)          //导入脚本
{
    try
    {
        CStdioFile File;
        if(!File.Open(FILE_NAME, CFile::modeRead))
        {
            tcout << TEXT("读取文件错误") << endl;
            m_dwErrCount++;
            return FALSE;
        }

        //清空命令序列
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

        //读取文件
        CString strRead = TEXT("");

        while(File.ReadString(strRead))
        {
            CString* pStr = new CString;
            if(pStr == NULL)
            {
                OutErrMsg(TEXT("申请节点失败！"));
                m_dwErrCount++;
            }
            *pStr = strRead;
            m_CmdOrderLst.AddTail(pStr);
        }

        m_bIsScript = TRUE;
        m_bIsInput = FALSE;
        
        File.Close();

        tcout << TEXT("导入成功") << endl;
    }
    catch(...)
    {
        tcout << TEXT("文件操作出错") << endl;    
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
            //获取输入
            TCHAR szBuf[MAX_INPUT] = {0};
            tcout << TEXT('>');
            
            tcin.clear();
            tcin.sync();
            tcin.getline(szBuf, MAX_INPUT, TEXT('\n'));
            tcin.clear();
            tcin.sync();
            
            //缓存命令
            CmdInfo.strCMD = szBuf;
            
            //命令小写
            CmdInfo.strCMD.MakeLower();
            
            
            CString strTmp = CmdInfo.strCMD;
            //转换为操作码
            if(m_CMD.Resolve(CmdInfo))
            {
                //缓存命令序列
                CString* pStrOrder = new CString;
                if(pStrOrder == NULL)
                {
                    OutErrMsg("GetUserInput：申请命令节点失败");
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

BOOL CDeBug::OnBreakPointEvent()       //一般断点
{
    //static BOOL bIsFirstInto = TRUE;
    EXCEPTION_RECORD& pExceptionRecord = m_DbgEvt.u.Exception.ExceptionRecord; 
    POSITION pos = NULL;
    
    //     //第一次来，是系统断点，用于断在入口点
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
    
    //其他断点
    if(IsAddrInBpList(pExceptionRecord.ExceptionAddress, m_NorMalBpLst, pos))
    {
        PMYBREAK_POINT bp = m_NorMalBpLst.GetAt(pos);
        //还原代码
        if(!WriteRemoteCode(bp->lpAddr, bp->dwOldOrder, bp->dwCurOrder))
        {
            return FALSE;
        }
        
        //修改目标线程EIP
        m_DstContext.Eip = m_DstContext.Eip - 1;
        
        //系统一次性断点，用于断在入口点
        if(bp->bpState == BP_SYS || bp->bpState == BP_ONCE)
        {
            m_NorMalBpLst.RemoveAt(pos);
            delete bp;
        }
        //常规断点
        else if(bp->bpState == BP_NORMAL)
        {
            //设置单步标志位
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

BOOL CDeBug::OnSingleStepEvent()       //单步异常
{
    BOOL bRet = FALSE;
    EXCEPTION_RECORD& pExceptionRecord = m_DbgEvt.u.Exception.ExceptionRecord; 
    POSITION pos = NULL;
    PMYDR6 Dr6 = (PMYDR6)&(m_DstContext.Dr6);
    PMYDR7 Dr7 = (PMYDR7)&(m_DstContext.Dr7);

    //一般断点
    if(m_bIsNormalStep)
    {
        if(IsAddrInBpList(m_lpTmpStepAddr, m_NorMalBpLst, pos))
        {
            PMYBREAK_POINT bp = m_NorMalBpLst.GetAt(pos);
            if(bp->bIsSingleStep == TRUE)
            {
                //重设断点
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

    //内存断点
    if(m_bIsMemStep)
    {
        if(IsAddrInBpList(m_lpTmpStepAddr, m_MemBpLst, pos))
        {
            PMYBREAK_POINT bp = m_MemBpLst.GetAt(pos);
            if(bp->bIsSingleStep == TRUE)
            {
                tcout << TEXT("内存断点命中") << endl;
                //重设断点
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
    
    //F7单步
    if(m_bIsMyStepInto)
    {
        m_bIsMyStepInto = FALSE;
        //如果单步与一般同时来，则不显示第二次输入
        if(!bRet)
        {
            bRet = Interaction(pExceptionRecord.ExceptionAddress);
        }
    }
    //硬件断点的断步配合
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

        //如果单步与硬断同时来，则不显示第二次输入
        if(!bRet)
        {
            bRet = Interaction((LPVOID)lpAddr, FALSE);
        }
        
    }

    //硬断1号命中
    if(Dr6->B0 == HBP_HIT)
    {
        m_dwWhichHardReg = 0;
        Dr7->L0 = HBP_UNSET;
        if(Dr7->RW0 == HBP_INSTRUCTION_EXECUT)
        {
            ShowCurAllDbg(pExceptionRecord.ExceptionAddress, CMD_SHOWFIVE);
            
            _tprintf(TEXT("硬件执行断点命中\r\n"));

            //断步配合,重设断点
            m_bIsMyHardReSet = TRUE;
            m_DstContext.EFlags |= TF;
            bRet = TRUE;
        }
        else
        {
            _tprintf(TEXT("硬件操作断点命中, , 当前指令为断点下一条.\r\n"));
            bRet = Interaction(pExceptionRecord.ExceptionAddress);
        }
    }
    //硬断2号命中
    else if(Dr6->B1 == HBP_HIT)
    {
        m_dwWhichHardReg = 1;
        Dr7->L1 = HBP_UNSET;
        if(Dr7->RW1 == HBP_INSTRUCTION_EXECUT)
        {
            ShowCurAllDbg(pExceptionRecord.ExceptionAddress, CMD_SHOWFIVE);
            
            _tprintf(TEXT("硬件执行断点命中\r\n"));
            
            //断步配合,重设断点
            m_bIsMyHardReSet = TRUE;
            m_DstContext.EFlags |= TF;
            bRet = TRUE;
        }
        else
        {
            _tprintf(TEXT("硬件操作断点命中, 当前指令为断点下一条.\r\n"));
            bRet = Interaction(pExceptionRecord.ExceptionAddress);
        }
    }
    //硬断3号命中
    else if(Dr6->B2 == HBP_HIT)
    {
        m_dwWhichHardReg = 2;
        Dr7->L2 = HBP_UNSET;
        if(Dr7->RW2 == HBP_INSTRUCTION_EXECUT)
        {
            ShowCurAllDbg(pExceptionRecord.ExceptionAddress, CMD_SHOWFIVE);
            
            _tprintf(TEXT("硬件执行断点命中\r\n"));
            
            //断步配合,重设断点
            m_bIsMyHardReSet = TRUE;
            m_DstContext.EFlags |= TF;
            bRet = TRUE;
        }
        else
        {
            _tprintf(TEXT("硬件操作断点命中, 当前指令为断点下一条.\r\n"));
            bRet = Interaction(pExceptionRecord.ExceptionAddress);
        }
    }
    //硬断4号命中
    else if(Dr6->B3 == HBP_HIT)
    {
        m_dwWhichHardReg = 3;
        Dr7->L3 = HBP_UNSET;
        if(Dr7->RW3 == HBP_INSTRUCTION_EXECUT)
        {
            ShowCurAllDbg(pExceptionRecord.ExceptionAddress, CMD_SHOWFIVE);
            
            _tprintf(TEXT("硬件执行断点命中\r\n"));
            
            //断步配合,重设断点
            m_bIsMyHardReSet = TRUE;
            m_DstContext.EFlags |= TF;
            bRet = TRUE;
        }
        else
        {
            _tprintf(TEXT("硬件操作断点命中, 当前指令为断点下一条.\r\n"));
            bRet = Interaction(pExceptionRecord.ExceptionAddress);
        }
    }

    
    return bRet;
}

BOOL CDeBug::OnAccessVolationEvent()   //内存访问异常
{
    //static BOOL bIsFirstInto = TRUE;
    EXCEPTION_RECORD& pExceptionRecord = m_DbgEvt.u.Exception.ExceptionRecord; 
    POSITION pos = NULL;

    //其他断点
    if(IsAddrInBpList(pExceptionRecord.ExceptionAddress, m_MemBpLst, pos))
    {
        MYBREAK_POINT& bp = *m_MemBpLst.GetAt(pos);
        //还原代码
        DWORD dwOldProtect = 0;
        
        //常规断点
        if(bp.bpState == BP_MEM)
        {
            if(!VirtualProtectEx(m_hDstProcess, bp.lpAddr, bp.dwLen, bp.dwOldProtect, &dwOldProtect))
            {
                OutErrMsg(TEXT("还原内存属性失败。"));
                return FALSE;
            }
            //设置单步标志位
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

BOOL CDeBug::ShowCurAllDbg(LPVOID lpAddr, CMDSTATE cmdState)  //显示当前所有调试信息
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
    
    //显示反汇编
    if(!ShowRemoteDisAsm(lpAddr, dwNotUse, dwCount))
        return FALSE;
    
    return TRUE;
}

BOOL CDeBug::CmdShowAsm(CMD_INFO& CmdInfo, LPVOID lpAddr)  //显示反汇编
{
    BOOL bRet = TRUE;
    
    int nAddr = 0;

    //查看指定内存
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
    //查看当前内存
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

#define RemoteOneReadSize 0x60  //一次读取远程数据的长度
BOOL CDeBug::ShowRemoteMem(LPVOID lpAddr, DWORD& dwOutCurAddr)           //显示远程内存
{
    DWORD dwAddr = (DWORD)lpAddr;
    DWORD dwRead = 0;
    UCHAR szBuf[RemoteOneReadSize] = {0};
    PUCHAR pszBuf = szBuf;
    
    //读取远程内存信息
    if(!ReadProcessMemory(m_hDstProcess, lpAddr, szBuf, RemoteOneReadSize, &dwRead))
    {
        OutErrMsg(TEXT("ShowRemoteDisAsm：读取远程内存失败！"));
        return FALSE;
    }
    
    //输出内存信息
    int nCount = dwRead / 0X10;
    for(int i = 0; i < nCount; i++)
    {
        //输出地址
        _tprintf(TEXT("%08X   "), dwAddr);
        //tcout << ios::hex << dwAddr << TEXT("    ");
        
        //输出十六进制值
        for(int j = 0; j < 0x10; j++)
        {
            _tprintf(TEXT("%02X "), pszBuf[j]);
            //tcout << ios::hex << pszBuf[j] << TEXT(' ');
        }
        
        tcout << TEXT("  ");
        
        //输出解析字符串
        for(int n = 0; n < 0x10; n++)
        {
            if(pszBuf[n] < 32 || pszBuf[n] > 127)
                _puttchar(TEXT('.'));
            else
                _puttchar(pszBuf[n]);
        }
        
        //补回车换行
        tcout << endl;
        
        dwAddr += 0x10;
        pszBuf += 0x10;
    }
    dwOutCurAddr = dwAddr;
    return TRUE;
}

BOOL CDeBug::ShowRemoteReg()           //显示远程寄存器
{
    // EAX=00000000   EBX=00000000   ECX=B2A10000   EDX=0008E3C8   ESI=FFFFFFFE
    // EDI=00000000   EIP=7703103C   ESP=0018FB08   EBP=0018FB34   DS =0000002B
    // ES =0000002B   SS =0000002B   FS =00000053   GS =0000002B   CS =00000023
    //获取EFlags
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


//反汇编指定地址一条数据
BOOL CDeBug::GetOneAsm(LPVOID lpAddr, DWORD& dwOrderCount, CString& strOutAsm)
{
    UINT unCodeAddress = (DWORD)lpAddr;    //基址
    DWORD dwRead = 0;
    UCHAR szBuf[MAXBYTE] = {0};         //远程内存缓冲区
    //BYTE btCode[MAXBYTE] = {0};         //
    char szAsmBuf[MAXBYTE] = {0};       //反汇编缓冲区
    char szOpcodeBuf[MAXBYTE] = {0};    //操作码缓冲区
    
    PBYTE pCode = szBuf;
    UINT unCodeSize = 0;
    
    
    //获取远程信息
    if(!ReadProcessMemory(m_hDstProcess, lpAddr, szBuf, RemoteOneReadSize, &dwRead))
    {
        OutErrMsg(TEXT("ShowRemoteDisAsm：读取远程内存失败！"));
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
BOOL CDeBug::ShowRemoteDisAsm(LPVOID lpAddr, DWORD& dwOutCurAddr, DWORD dwAsmCount)        //显示远程反汇编
{
    DWORD dwRead = 0;
    UCHAR szBuf[MAXBYTE] = {0};
    
    BOOL bRet = FALSE;
    DWORD dwReadLen = 0;
    //BYTE btCode[MAXBYTE] = {0};
    char szAsmBuf[MAXBYTE] = {0};           //反汇编指令缓冲区
    char szOpcodeBuf[MAXBYTE] = {0};        //机器码缓冲区
    UINT unCodeSize = 0;
    UINT unCount = 0;
    UINT unCodeAddress = (DWORD)lpAddr;
    PBYTE pCode = szBuf;
    char szFmt[MAXBYTE *2] = {0};
    
    //获取远程信息
    if(!ReadProcessMemory(m_hDstProcess, lpAddr, szBuf, RemoteOneReadSize, &dwRead))
    {
        OutErrMsg(TEXT("ShowRemoteDisAsm：读取远程内存失败！"));
        return FALSE;
    }
    
    //转换5条汇编码
    DWORD dwRemaining = 0;
    while(unCount < dwAsmCount)
    {
        Decode2AsmOpcode(pCode, szAsmBuf, szOpcodeBuf,
            &unCodeSize, unCodeAddress);
        
        _tprintf(TEXT("%p:%-20s%s\r\n"), unCodeAddress, szOpcodeBuf, szAsmBuf);
        
        //         dwRemaining = 0x18 - _tcsclen(szOpcodeBuf);
        // 
        //         //补空格
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
    case EXCEPTION_BREAKPOINT:          //断点
        bRet = OnBreakPointEvent();
        break;
        
    case EXCEPTION_SINGLE_STEP:         //单步
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
    //设置入口点断点，
    CREATE_PROCESS_DEBUG_INFO& pCreateEvent = m_DbgEvt.u.CreateProcessInfo;
    LPVOID lpEntryPoint = pCreateEvent.lpStartAddress;
    
    PMYBREAK_POINT ptagBp = new MYBREAK_POINT;
    ZeroMemory(ptagBp, sizeof(MYBREAK_POINT));
    
    ptagBp->bpState = BP_SYS;
    ptagBp->lpAddr = lpEntryPoint;
    ptagBp->dwCurOrder = NORMAL_CC;
    
    if(!WriteRemoteCode(lpEntryPoint, ptagBp->dwCurOrder, ptagBp->dwOldOrder))
    {
        tcout << TEXT("系统断点: 严重BUG，请联系管理员！") << endl;
        
        //释放资源
        if(ptagBp != NULL)
            delete ptagBp;
        
        return FALSE;
    }
    
    //添加断点节点
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
    
    //遍历链表
    while(pos)
    {
        posTmp = pos;
        MYBREAK_POINT& bp = *bpSrcLst.GetNext(pos);

        //一般断点
        if(bp.bpState == BP_NORMAL ||
           bp.bpState == BP_SYS ||
           bp.bpState == BP_ONCE)
        {
            if(bp.lpAddr == lpAddr)
            {
                bRet = TRUE;
                //已找到
                dwOutPos = posTmp;
                break;
            }
        }
        //内存断点
        else if(bp.bpState == BP_MEM)
        {
            //内存断点命中范围
            if(bp.lpAddr <= lpAddr || 
               (LPVOID)((DWORD)bp.lpAddr + bp.dwLen) >= lpAddr)
            {
                bRet = TRUE;
                //已找到
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
    //遍历链表
    while(pos)
    {
        
        MODLST& ModLst = *m_ModuleLst.GetNext(pos);
        _tprintf(TEXT("地址:%08X\t路径:%s\r\n"),
            ModLst.dwBaseAddr,
            (LPCTSTR)ModLst.strPath);
        //         tcout << TEXT("地址:") << ios::hex << ModLst.dwBaseAddr << TEXT("\t")
        //               << TEXT("路径:") << ModLst.strPath << endl;
    }
    tcout << TEXT("==============================================================") << endl;
    return TRUE;
}

BOOL CDeBug::OnUnLoadDll()
{
    BOOL bRet = FALSE;
    POSITION pos = m_ModuleLst.GetHeadPosition();
    POSITION posTmp = NULL;
    
    //遍历链表
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
    
    //生成链表节点
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
        OutErrMsg(TEXT("OnLoadDll：申请节点失败！"));
        return FALSE;
    }
    
    //保存DLL 基址
    pModLst->dwBaseAddr = (DWORD)DllInfo.lpBaseOfDll;
    
    //读取DLL 地址
    if (ReadProcessMemory(m_hDstProcess, DllInfo.lpImageName, \
        &lpString, sizeof(LPVOID), NULL) == NULL)
    {
        m_dwErrCount++;
        OutErrMsg(TEXT("OnLoadDll：读取远程地址失败！"));
        return FALSE;
    }
    
    //读取DLL路径
    if (ReadProcessMemory(m_hDstProcess, lpString, szBuf, \
        sizeof(szBuf) / sizeof(TCHAR), NULL) == NULL)
    {
        m_dwErrCount++;
        OutErrMsg(TEXT("OnLoadDll：读取模块路径失败！"));
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
        //转换UNICODE为ASCII
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
    
    //抽掉内存保护属性
    if(!VirtualProtectEx(m_hDstProcess, lpRemoteAddr, 1, PAGE_EXECUTE_READWRITE, &dwOldProtect))
    {
        m_dwErrCount++;
        OutErrMsg(TEXT("WriteRemoteCode：抽取保护属性失败！"));
        return FALSE;
    }
    
    //读取旧代码并保存
    if(!ReadProcessMemory(m_hDstProcess, lpRemoteAddr, &pbtOutChar, sizeof(BYTE), &dwReadLen))
    {
        m_dwErrCount++;
        OutErrMsg(TEXT("WriteRemoteCode：读取远程内存失败！"));
        return FALSE;
    }
    
    //写入新代码
    if(!WriteProcessMemory(m_hDstProcess, lpRemoteAddr, &btInChar, sizeof(BYTE), &dwReadLen))
    {
        m_dwErrCount++;
        OutErrMsg(TEXT("WriteRemoteCode：写入远程内存失败！"));
        return FALSE;
    }
    
    //还原内存保护属性
    if(!VirtualProtectEx(m_hDstProcess, lpRemoteAddr, 1, dwOldProtect, &dwOldProtect))
    {
        m_dwErrCount++;
        OutErrMsg(TEXT("WriteRemoteCode：还原保护属性失败！"));
        return FALSE;
    }
    
    return TRUE;
}

BOOL CDeBug::CmdSetNormalBp(CMD_INFO& CmdInfo, LPVOID lpAddr)      //设置一般断点
{
    if(CmdInfo.strCMD.GetLength() > 0)
    {
        //转换操作数
        PTCHAR pTmp = NULL;
        int nAddr = _tcstol(CmdInfo.strCMD, &pTmp, CONVERT_HEX);
        
        //检查是否超范围
        if((DWORD)nAddr > MAX_MEM)
        {
            _tprintf(TEXT("不支持的地址: %08X\r\n"), nAddr);
            return FALSE;
        }
        
        PMYBREAK_POINT ptagBp = new MYBREAK_POINT;
        if(ptagBp == NULL)
        {
            OutErrMsg(TEXT("CmdSetNormalBp：内存不足，请联系管理员！"));
            m_dwErrCount++;
            tcout << TEXT("内存断点：未知错误，请联系管理员！") << endl;
        }
        ZeroMemory(ptagBp, sizeof(MYBREAK_POINT));
        
        ptagBp->bpState = BP_NORMAL;
        ptagBp->lpAddr = (LPVOID)nAddr;
        ptagBp->dwCurOrder = NORMAL_CC;
        
        //设置CC断点
        if(!WriteRemoteCode(ptagBp->lpAddr, ptagBp->dwCurOrder, ptagBp->dwOldOrder))
        {
            tcout << TEXT("无效内存，无法设置断点！@") << endl;
            
            //释放资源
            if(ptagBp != NULL)
                delete ptagBp;
            
            return FALSE;
        }
        
        //添加断点节点
        m_NorMalBpLst.AddTail(ptagBp);
    }
    else
    {
        tcout << TEXT("一般断点需要操作数") << endl;
        return FALSE;
    }
    
    return TRUE;
}


BOOL CDeBug::CmdShowHardBpLst(CMD_INFO& CmdInfo, LPVOID lpAddr)    //显示硬件断点
{    
    DWORD dwCount = 0;
    POSITION pos = m_HardBpLst.GetHeadPosition();
    tcout << TEXT("=====================硬件断点=====================") << endl;
    if(m_HardBpLst.IsEmpty())
    {
        tcout << TEXT("暂无") << endl;
    }
    else
    {
        while(pos)
        {
            MYBREAK_POINT& Bp = *m_HardBpLst.GetNext(pos);
            _tprintf(TEXT("\t序号：%d\t地址：0x%p\t"), dwCount++, (DWORD)Bp.lpAddr);
            switch(Bp.hbpStatus)
            {
            case BP_HARD_EXEC:
                _tprintf(TEXT("状态：硬件执行\r\n"));
                break;
            case BP_HARD_READ:
                _tprintf(TEXT("状态：硬件读取\r\n"));
                break;
            case BP_HARD_WRITE:
                _tprintf(TEXT("状态：硬件写入\r\n"));
                break;
            }
        }
    }
    
    tcout << TEXT("=====================硬件断点=====================") << endl;
    
    return TRUE;
}

BOOL CDeBug::CmdClearHardBpLst(CMD_INFO& CmdInfo, LPVOID lpAddr)    //显示硬件断点
{
    CmdShowHardBpLst(CmdInfo, lpAddr);
    if(!m_HardBpLst.IsEmpty())
    {
        BOOL bIsDel = FALSE;
        DWORD dwNum = 0;
        DWORD dwLstCount = m_HardBpLst.GetCount();
        
        while(TRUE)
        {
            //获取用户输入
            tcout << TEXT("请输入编号：") ;
            _tscanf(TEXT("%d"), &dwNum);
            if(dwNum < 0 || dwNum >= dwLstCount)
            {
                tcout << TEXT("输入编号有误！") << endl;
                continue;
            }
            
            //遍历链表
            POSITION pos = m_HardBpLst.GetHeadPosition();
            POSITION posTmp = NULL;
            while(pos)
            {
                posTmp = pos;
                MYBREAK_POINT& bp = *m_HardBpLst.GetNext(pos);
                
                if(0 == dwNum--)
                {
                    //移除节点
                    m_HardBpLst.RemoveAt(posTmp);

                    //设置最后一个硬断为空
                    MYDR7& Dr7 = *(PMYDR7)(&m_DstContext.Dr7);
                    switch(m_HardBpLst.GetCount())
                    {
                    case 0:
                        m_DstContext.Dr0 = 0;   //填写地址
                        Dr7.L0 = HBP_UNSET;                   //设置硬件断点启用
                        Dr7.RW0 = 0;                //设置硬件断点的状态
                        Dr7.LEN0 = 0;                   //断点长度 1 2 4
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
                    
                    //重设硬件断点
                    POSITION posHBP = m_HardBpLst.GetHeadPosition();
                    DWORD dwNodeCount = 0;

                    //遍历链表
                    while(posHBP)
                    {
                        MYBREAK_POINT& hBP = *m_HardBpLst.GetNext(posHBP);
                        DWORD dwBPState = 0;

                        //获取状态
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
                        
                        //设置硬断
                        switch(dwNodeCount++)
                        {
                        case 0:
                            m_DstContext.Dr0 = (DWORD)lpAddr;   //填写地址
                            Dr7.L0 = HBP_SET;                   //设置硬件断点启用
                            Dr7.RW0 = dwBPState;                //设置硬件断点的状态
                            Dr7.LEN0 = hBP.dwLen;                   //断点长度 1 2 4
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
                tcout << TEXT("成功") << endl;
                break;
            }
        }
    }
    return TRUE;
}

BOOL CDeBug::CmdSetHardBp(CMD_INFO& CmdInfo, LPVOID lpAddr)        //设置硬件断点
{
    BOOL bRet = TRUE;
    //判断硬件断点表还有没有位置
    if(m_HardBpLst.GetCount() < 4)
    {
        int nPos = CmdInfo.strCMD.Find(TEXT(' '));
        if(nPos != -1)
        {
            //拆分指令
            int nAddr = 0;
            PCHAR pNoUse = NULL;
            CString strAddr = CmdInfo.strCMD.Left(nPos);
            CString strOther = CmdInfo.strCMD.Right(CmdInfo.strCMD.GetLength() - nPos - 1);

            //判断地址是否超范围
            nAddr = _tcstol(strAddr, &pNoUse, CONVERT_HEX);
            if(nAddr < 0 || (DWORD)nAddr > MAX_MEM)
            {
                tcout << TEXT("地址超范围") << endl;
                return TRUE;
            }
            
            nPos = strOther.Find(TEXT(' '));
            if(nPos != -1)
            {
                strOther.Right(strOther.GetLength() - nPos - 1);
                
                //拆分操作码
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
                            tcout << TEXT("暂未支持的命令") << endl;
                            return TRUE;
                        }
                    }
                    else
                    {
                        tcout << TEXT("暂未支持的命令") << endl;
                        return TRUE;
                    }

                    //硬件执行
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
                    tcout << TEXT("暂未支持的命令") << endl;
                }
            }
            //硬件执行
            else if(strOther.Find(TEXT("e")) != -1)
            {
                bRet = SetHardBreakPoint((LPVOID)nAddr, BP_HARD_EXEC);
            }
            else
            {
                tcout << TEXT("暂未支持的命令") << endl;
            }
        }
        else
        {
            tcout << TEXT("暂未支持的命令, 请指明断点类型 R W E") << endl;
        }
    }
    else
    {
        tcout << TEXT("暂只支持4个硬件断点!") << endl;
    }
    return bRet;
}


BOOL CDeBug::SetHardBreakPoint(LPVOID lpAddr, BPSTATE bpState, DWORD dwLen)  //设置硬件断点
{
    PMYBREAK_POINT pBP = new MYBREAK_POINT;
    if(pBP == NULL)
    {
        OutErrMsg(TEXT("SetHardBreakPoint: 申请节点失败！"));

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
        m_DstContext.Dr0 = (DWORD)lpAddr;   //填写地址
        Dr7.L0 = HBP_SET;                   //设置硬件断点启用
        Dr7.RW0 = dwBPState;                //设置硬件断点的状态
        Dr7.LEN0 = dwLen;                   //断点长度 1 2 4
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

BOOL CDeBug::CmdShowMemBpLst(CMD_INFO& CmdInfo, LPVOID lpAddr)     //显示内存断点
{
    DWORD dwCount = 0;
    POSITION pos = m_MemBpLst.GetHeadPosition();
    tcout << TEXT("=====================内存断点=====================") << endl;
    if(m_MemBpLst.IsEmpty())
    {
        tcout << TEXT("暂无") << endl;
    }
    else
    {
        while(pos)
        {
            MYBREAK_POINT& Bp = *m_MemBpLst.GetNext(pos);
            _tprintf(TEXT("\t序号：%d\t地址：0x%p\t长度：%d\r\n"), 
                    dwCount++, 
                    (DWORD)Bp.lpAddr, 
                    Bp.dwLen);
        }
    }
    
    tcout << TEXT("=====================内存断点=====================") << endl;
    
    return TRUE;
}

BOOL CDeBug::CmdClearMemBp(CMD_INFO& CmdInfo, LPVOID lpAddr)       //删除内存断点
{
    CmdShowMemBpLst(CmdInfo, lpAddr);
    if(!m_MemBpLst.IsEmpty())
    {
        BOOL bIsDel = FALSE;
        DWORD dwNum = 0;
        DWORD dwLstCount = m_MemBpLst.GetCount();
        
        while(TRUE)
        {
            //获取用户输入
            tcout << TEXT("请输入编号：") ;
            _tscanf(TEXT("%d"), &dwNum);
            if(dwNum < 0 || dwNum >= dwLstCount)
            {
                tcout << TEXT("输入编号有误！") << endl;
                continue;
            }
            
            //遍历链表
            POSITION pos = m_MemBpLst.GetHeadPosition();
            POSITION posTmp = NULL;
            while(pos)
            {
                posTmp = pos;
                MYBREAK_POINT& bp = *m_MemBpLst.GetNext(pos);
                
                if(0 == dwNum--)
                {
                    DWORD dwOldProtect = 0;

                    //修补断点抽取的内存属性
                    if(VirtualProtectEx(m_hDstProcess, bp.lpAddr, bp.dwLen, bp.dwOldProtect, &dwOldProtect))
                    {
                        //移除节点
                        m_MemBpLst.RemoveAt(posTmp);
                        bIsDel = TRUE;
                    }
                    else
                    {
                        tcout << TEXT("内存操作有误！") << endl;
                        OutErrMsg(TEXT("CmdClearNormalBp：修补断点失败"));
                        return FALSE;
                    }
                }
            }
            if(bIsDel)
            {
                tcout << TEXT("成功") << endl;
                break;
            }
        }
    }
    return TRUE;
}

BOOL CDeBug::CmdSetMemBp(CMD_INFO& CmdInfo, LPVOID lpAddr)         //设置内存断点
{
    if(CmdInfo.strCMD.GetLength() > 0)
    {
        //拆分命令
        int nPos = CmdInfo.strCMD.Find(TEXT(' '));
        if(nPos != -1)
        {
            CString strAddr = CmdInfo.strCMD.Left(nPos);
            CString strSize = CmdInfo.strCMD.Right(CmdInfo.strCMD.GetLength() - nPos - 1);

            //转换操作数
            PTCHAR pTmp = NULL;
            int nAddr = _tcstol(strAddr, &pTmp, CONVERT_HEX);
            
            //检查是否超范围
            if((DWORD)nAddr > MAX_MEM)
            {
                _tprintf(TEXT("不支持的地址: %08X\r\n"), nAddr);
                return FALSE;
            }
            
            
            int nSize = _tcstol(strSize, &pTmp, CONVERT_HEX);
            
            //查询分页信息
            MEMORY_BASIC_INFORMATION mbi;  //被调试程序的内存信息
            if(!VirtualQueryEx(m_hDstProcess, (LPVOID)nAddr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)))
            {
                _tprintf(TEXT("不支持的地址: %08X\r\n"), nAddr);
                return FALSE;
            }
            
            //判断内存断点是否跨分页
            if((DWORD)(nAddr + nSize) > (DWORD)((UINT)mbi.BaseAddress + mbi.RegionSize))
            {
                _tprintf(TEXT("不支持的地址: 0x%08X 长度：0x%08X\r\n"), nAddr, nSize);
                return FALSE;
            }
            

            PMYBREAK_POINT ptagBp = new MYBREAK_POINT;
            if(ptagBp == NULL)
            {
                OutErrMsg(TEXT("CmdSetNormalBp：内存不足，请联系管理员！"));
                m_dwErrCount++;
                tcout << TEXT("内存断点：未知错误，请联系管理员！") << endl;
            }

            ZeroMemory(ptagBp, sizeof(MYBREAK_POINT));
            
            //填写节点信息
            ptagBp->bpState = BP_MEM;
            ptagBp->lpAddr = (LPVOID)nAddr;
            ptagBp->dwLen = (DWORD)nSize;
            
            //设置内存断点，抽取内存属性
            if(!VirtualProtectEx(m_hDstProcess, 
                (LPVOID)nAddr, 
                (DWORD)nSize, 
                PAGE_NOACCESS, 
                &ptagBp->dwOldProtect))
            {
                tcout << TEXT("设置内存断点失败") << endl;
                m_dwErrCount++;
                return FALSE;
            }
            
            //添加断点节点
            m_MemBpLst.AddTail(ptagBp);
        }//End if(nPos != -1)
        else
        {
            tcout << TEXT("内存断点需要指定长度") << endl;
            return FALSE;
        }
        
    }//End if(CmdInfo.strCMD.GetLength() > 0)
    else
    {
        tcout << TEXT("内存断点需要操作数") << endl;
        return FALSE;
    }
    
    return TRUE;
}

BOOL CDeBug::CmdSetOneStepInto(CMD_INFO& CmdInfo, LPVOID lpAddr)       //设置单步
{
    m_bIsMyStepInto = TRUE;
    
    //设置单步标志位
    EFLAGS& eFlag = *(PEFLAGS)&m_DstContext.EFlags;
    //m_DstContext.EFlags |= TF;

    if(eFlag.dwTF != 1)
    {
        eFlag.dwTF = 1;
    }
    
    return TRUE;
}

BOOL CDeBug::CmdSetOneStepOver(CMD_INFO& CmdInfo, LPVOID lpAddr)   //单步步过
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
            tcout << TEXT("系统断点: 严重BUG，请联系管理员！") << endl;
            
            //释放资源
            if(ptagBp != NULL)
                delete ptagBp;
            
            return FALSE;
        }
        
        //添加断点节点
        m_NorMalBpLst.AddTail(ptagBp);
        
    }
    else
    {
        m_bIsMyStepInto = TRUE;
        
        //设置单步标志位
        //m_DstContext.EFlags |= TF;
        //设置单步标志位
        EFLAGS& eFlag = *(PEFLAGS)&m_DstContext.EFlags;
        //m_DstContext.EFlags |= TF;
        
        if(eFlag.dwTF != 1)
        {
            eFlag.dwTF = 1;
        }
    }
    return TRUE;
}


BOOL CDeBug::CmdShowReg(CMD_INFO& CmdInfo, LPVOID lpAddr)  //显示
{
    BOOL bRet = TRUE;
    TCHAR szBuf[MAXBYTE] = {0};
    DWORD dwTmpReg = 0;
    bRet = ShowRemoteReg();
    while(TRUE)
    {
        //获取用户输入
        tcout << TEXT("-请输入寄存器:");
        
        
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
        //回车
        else if(strBuf.GetLength() == 0)
        {
            break;
        }
        else
        {
            tcout << TEXT("无效命令") << endl;
            continue;
        }
        
    }
    return bRet;
}


BOOL CDeBug::CmdShowNormalBpLst(CMD_INFO& CmdInfo, LPVOID lpAddr)  //显示一般断点
{
    DWORD dwCount = 0;
    POSITION pos = m_NorMalBpLst.GetHeadPosition();
    tcout << TEXT("=====================一般断点=====================") << endl;
    if(m_NorMalBpLst.IsEmpty())
    {
        tcout << TEXT("暂无") << endl;
    }
    else
    {
        while(pos)
        {
            MYBREAK_POINT& Bp = *m_NorMalBpLst.GetNext(pos);
            _tprintf(TEXT("\t\t序号：%d\t地址：0x%p\r\n"), dwCount++, (DWORD)Bp.lpAddr);
        }
    }
    
    tcout << TEXT("=====================一般断点=====================") << endl;
    
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
            //获取用户输入
            tcout << TEXT("请输入编号：") ;
            _tscanf(TEXT("%d"), &dwNum);
            if(dwNum < 0 || dwNum >= dwLstCount)
            {
                tcout << TEXT("输入编号有误！") << endl;
                continue;
            }
            
            //遍历链表
            POSITION pos = m_NorMalBpLst.GetHeadPosition();
            POSITION posTmp = NULL;
            while(pos)
            {
                posTmp = pos;
                MYBREAK_POINT& bp = *m_NorMalBpLst.GetNext(pos);
                
                if(0 == dwNum--)
                {
                    //修补断点抽取的代码
                    if(WriteRemoteCode(bp.lpAddr, bp.dwOldOrder, bp.dwCurOrder))
                    {
                        //移除节点
                        m_NorMalBpLst.RemoveAt(posTmp);
                        bIsDel = TRUE;
                    }
                    else
                    {
                        tcout << TEXT("内存操作有误！") << endl;
                        OutErrMsg(TEXT("CmdClearNormalBp：修补断点失败"));
                        return FALSE;
                    }
                }
            }
            if(bIsDel)
            {
                tcout << TEXT("成功") << endl;
                break;
            }
        }
    }
    return TRUE;
}

BOOL CDeBug::ShowMemLst(CMD_INFO& CmdInfo, LPVOID lpAddr)          //显示内存列表
{
    MEMORY_BASIC_INFORMATION mbi;  //被调试程序的内存信息
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
        
        switch (mbi.Type)  //内存块类型 MEM_IMAGE MEM_MAPPED MEM_PRIVATE
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
        
        switch (mbi.State)  //内存块状态
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
        
        switch (mbi.AllocationProtect)  //内存块呗初次保留时的保护属性
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
        
        switch (mbi.Protect)  //内存块属性
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

BOOL CDeBug::HandleCmd(CMD_INFO& CmdInfo, LPVOID lpAddr)          //执行命令
{
    BOOL bIsBreak = FALSE;
    switch(CmdInfo.dwState)
    {
        //单步步入
    case CMD_STEP:
        CmdSetOneStepInto(CmdInfo, lpAddr);
        bIsBreak = TRUE;
        break;
        
        //单步步过
    case CMD_STEPGO:
        CmdSetOneStepOver(CmdInfo, lpAddr);
        bIsBreak = TRUE;
        break;
        
        //运行
    case CMD_RUN:
        bIsBreak = TRUE;
        break;
        
        //跟踪
    case CMD_TRACE:
        break;
        
        //显示反汇编
    case CMD_DISPLAY_ASMCODE:
        CmdShowAsm(CmdInfo, lpAddr);
        break;
        
        //显示内存
    case CMD_DISPLAY_DATA:
        CmdShowMem(CmdInfo, lpAddr);
        break;
        
        //寄存器
    case CMD_REGISTER:
        CmdShowReg(CmdInfo, lpAddr);
        break;
        
        //修改内存  
    case CMD_EDIT_DATA:
        break;
        
        //一般断点
    case CMD_BREAK_POINT:
        CmdSetNormalBp(CmdInfo, lpAddr);
        break;
        
        //一般断点列表
    case CMD_BP_LIST:
        CmdShowNormalBpLst(CmdInfo, lpAddr);
        break;
        
        //清除一般断点
    case CMD_CLEAR_NORMAL:
        CmdClearNormalBp(CmdInfo, lpAddr);
        break;
        
        //硬件断点
    case CMD_BP_HARD:
        CmdSetHardBp(CmdInfo, lpAddr);
        break;
        
        //硬件断点列表
    case CMD_BP_HARD_LIST:
        CmdShowHardBpLst(CmdInfo, lpAddr);
        break;
        
        //清除硬件断点列表
    case CMD_CLEAR_BP_HARD:
        CmdClearHardBpLst(CmdInfo, lpAddr);
        break;
        
        //内存断点
    case CMD_BP_MEMORY:
        CmdSetMemBp(CmdInfo, lpAddr);
        break;
        
        //内存断点列表
    case CMD_BP_MEMORY_LIST:
        CmdShowMemBpLst(CmdInfo, lpAddr);
        break;
        
        //内存分页断点列表
    case CMD_BP_PAGE_LIST:
        break;
        
        //清除内存断点
    case CMD_CLEAR_BP_MEMORY:
        CmdClearMemBp(CmdInfo, lpAddr);
        break;
        
        //加载脚本
    case CMD_LOAD_SCRIPT:
        LoadScript(CmdInfo, lpAddr);
        break;
        
        //导出脚本
    case CMD_EXPORT_SCRIPT:
        SaveScript(CmdInfo, lpAddr);
        break;
        
        //退出程序
    case CMD_QUIT:
        tcout << TEXT("谢谢使用！") << endl;
        system("pause");
        ExitProcess(0);
        break;
        
        //模块列表
    case CMD_MODULE_LIST:
        ShowDllLst();
        break;
        
        //内存列表
    case CMD_MEM_INFO_LIST:
        ShowMemLst(CmdInfo, lpAddr);
        break;
        
        //默认与无效
    case CMD_INVALID:
    default:
        break;
    }
    
    CmdInfo.bIsBreakInputLoop = bIsBreak;
    return TRUE;
}
