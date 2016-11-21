// ResolveCMD.cpp: implementation of the CResolveCMD class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MyDebug.h"
#include "ResolveCMD.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CResolveCMD::CResolveCMD()
{

}

CResolveCMD::~CResolveCMD()
{

}


BOOL CResolveCMD::Resolve(CMD_INFO& CmdInfo)
{
    BOOL bRet = TRUE;
    BOOL bIsSupport = TRUE;
    DWORD dwStrLen = CmdInfo.strCMD.GetLength();

    //截取命令和操作数
    int nPos = CmdInfo.strCMD.Find(TEXT(' '));
    if(nPos != -1)
    {
        //获取命令
        CString strOrder = CmdInfo.strCMD.Left(nPos);
        
        //获取操作数
        CString strOperand = CmdInfo.strCMD.Right(dwStrLen - nPos - 1);
        
        //一般断点
        if(strOrder == TEXT("bp"))
        {
            CmdInfo.dwState = CMD_BREAK_POINT;
        }
        //硬件断点
        else if(strOrder == TEXT("bh"))
        {
            CmdInfo.dwState = CMD_BP_HARD;
        }
        //内存断点
        else if(strOrder == TEXT("bm"))
        {
            CmdInfo.dwState = CMD_BP_MEMORY;
        }
        //查看内存
        else if(strOrder == TEXT("d"))
        {
            CmdInfo.dwState = CMD_DISPLAY_DATA;
        }
        //查看反汇编
        else if(strOrder == TEXT("u"))
        {
            CmdInfo.dwState = CMD_DISPLAY_ASMCODE;
        }
        //修改内存数据
        else if(strOrder == TEXT("e"))
        {
            CmdInfo.dwState = CMD_EDIT_DATA;
        }
        //跟踪
        else if(strOrder == TEXT("trace"))
        {
            CmdInfo.dwState = CMD_TRACE;
        }
        else
        {
            //未知命令
            bIsSupport = FALSE;
            bRet = FALSE;
            CmdInfo.dwState = CMD_INVALID;
        }

        if(bIsSupport)
            CmdInfo.strCMD = strOperand;
    }
    //单命令
    else
    {
        //退出
        if(CmdInfo.strCMD == TEXT("q") || CmdInfo.strCMD == TEXT("exit"))
        {
            CmdInfo.dwState = CMD_QUIT;
        }
        //单步步入
        else if(CmdInfo.strCMD == TEXT("t"))
        {
            CmdInfo.dwState = CMD_STEP;
        }
        //单步步过
        else if(CmdInfo.strCMD == TEXT("p"))
        {
            CmdInfo.dwState = CMD_STEPGO;
        }
        //运行
        else if(CmdInfo.strCMD == TEXT("g"))
        {
            CmdInfo.dwState = CMD_RUN;
        }
        //寄存器
        else if(CmdInfo.strCMD == TEXT("r"))
        {
            CmdInfo.dwState = CMD_REGISTER;
        }
        //一般断点
        else if(CmdInfo.strCMD == TEXT("bpl"))
        {
            CmdInfo.dwState = CMD_BP_LIST;
        }
        //硬件断点表
        else if(CmdInfo.strCMD == TEXT("bhl"))
        {
            CmdInfo.dwState = CMD_BP_HARD_LIST;
        }
        //清除硬件断点表
        else if(CmdInfo.strCMD == TEXT("bhc"))
        {
            CmdInfo.dwState = CMD_CLEAR_BP_HARD;
        }
        //内存断点表
        else if(CmdInfo.strCMD == TEXT("bml"))
        {
            CmdInfo.dwState = CMD_BP_MEMORY_LIST;
        }
        //内存断点表
        else if(CmdInfo.strCMD == TEXT("bmc"))
        {
            CmdInfo.dwState = CMD_CLEAR_BP_MEMORY;
        }
        //导入脚本
        else if(CmdInfo.strCMD == TEXT("ls"))
        {
            CmdInfo.dwState = CMD_LOAD_SCRIPT;
        }
        //导出脚本
        else if(CmdInfo.strCMD == TEXT("es"))
        {
            CmdInfo.dwState = CMD_EXPORT_SCRIPT;
        }
        //查看模块
        else if(CmdInfo.strCMD == TEXT("ml"))
        {
            CmdInfo.dwState = CMD_MODULE_LIST;
        }
        //修改内存数据
        else if(CmdInfo.strCMD == TEXT("e"))
        {
            CmdInfo.dwState = CMD_EDIT_DATA;
        }
        //显示内存数据
        else if(CmdInfo.strCMD == TEXT("d"))
        {
            CmdInfo.dwState = CMD_DISPLAY_DATA;
        }
        //反汇编
        else if(CmdInfo.strCMD == TEXT("u"))
        {
            CmdInfo.dwState = CMD_DISPLAY_ASMCODE;
        }
        //清除一般断点
        else if(CmdInfo.strCMD == TEXT("bpc"))
        {
            CmdInfo.dwState = CMD_CLEAR_NORMAL;
        }
        //遍历内存页
        else if(CmdInfo.strCMD == TEXT("mil"))
        {
            CmdInfo.dwState = CMD_MEM_INFO_LIST;
        }
        else
        {
            //未知命令
            bRet = FALSE;
            bIsSupport = FALSE;
            CmdInfo.dwState = CMD_INVALID;
        }

        CmdInfo.strCMD = TEXT("");
    }

    //未知命令
    if(!bIsSupport)
        tcout << TEXT("暂未支持此命令") << endl;

    return bRet;
}
