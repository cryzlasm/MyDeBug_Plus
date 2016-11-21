// DeBug.h: interface for the CDeBug class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEBUG_H__CCFCD8C7_A313_4B5F_9343_626594F87C8E__INCLUDED_)
#define AFX_DEBUG_H__CCFCD8C7_A313_4B5F_9343_626594F87C8E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "TagDeal.h"
#include "ResolveCMD.h"
#include "Decode2Asm.h"
#include "PE.h"

#include <afxtempl.h>
#include <COMDEF.H>

#pragma comment(lib, "Decode2Asm.lib")

//=========================================================================
// #define CONTEXT_ALL             (CONTEXT_CONTROL | CONTEXT_INTEGER | \
//                                 CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT | \
//                                 CONTEXT_DEBUG_REGISTERS)

typedef HANDLE (__stdcall *PFN_OpenThread)(
                                           DWORD dwDesiredAccess,  // access right
                                           BOOL bInheritHandle,    // handle inheritance option
                                           DWORD dwThreadId        // thread identifier
                                           );

#define _OUT_       //输出参数
#define _IN_        //输入参数
//=========================================================================
using namespace std;
#ifdef _UNICODE
#define tcout wcout
#define tcin  wcin

#else
#define tcout cout
#define tcin  cin

#endif
//=========================================================================

class CDeBug  
{
public:
    CDeBug();
    virtual ~CDeBug();
    BOOL GetFun();                      //获取OpenThread
    BOOL Start(_IN_ TCHAR* argv[]);		//程序开始
    BOOL EventLoop();                   //事件消息循环
    BOOL Interaction(LPVOID lpAddr, BOOL bIsShowDbgInfo = TRUE);                 //人机交互
    BOOL GetUserInput(CMD_INFO& CmdInfo);                //获取用户输入, 带出输入状态
    BOOL HandleCmd(CMD_INFO& CmdInfo, LPVOID lpAddr);          //执行命令
    
    //写入远程字节，返回旧字节
    BOOL WriteRemoteCode(_IN_ LPVOID lpRemoteAddr, _IN_ DWORD btInChar, _OUT_ DWORD& pbtOutChar);
    //判断地址是否在链表中
    BOOL IsAddrInBpList(_IN_ LPVOID lpAddr,
                        _IN_ CList<PMYBREAK_POINT,
                        PMYBREAK_POINT&>& bpSrcLst,
                        _OUT_ POSITION& dwOutPos,
                        BOOL bIsNextAddr =  FALSE);
    
    BOOL OnExceptionEvent();        //处理异常事件
    
    BOOL OnCreateProcessEvent();    //处理创建进程事件
    
    BOOL OnLoadDll();               //模块加载
    BOOL OnUnLoadDll();             //模块卸载
    BOOL ShowDllLst();              //显示当前存在的模块信息
    
    BOOL OnBreakPointEvent();       //一般断点
    BOOL OnSingleStepEvent();       //单步异常
    BOOL OnAccessVolationEvent();   //内存访问异常
    
    //显示当前所有调试信息,默认显示一条
    BOOL ShowCurAllDbg(LPVOID lpAddr, CMDSTATE cmdState = CMD_SHOWONCE);    
    
    BOOL ShowRemoteMem(LPVOID lpAddr, DWORD& dwOutCurAddr);           //显示远程内存
    BOOL ShowRemoteReg();                        //显示远程寄存器

    //显示远程反汇编,默认十条
    BOOL ShowRemoteDisAsm(LPVOID lpAddr, DWORD& dwOutCurAddr, DWORD dwAsmCount = 10);      

    //反汇编指定地址一条数据
    BOOL GetOneAsm(LPVOID lpAddr, DWORD& dwOrderCount, CString& strOutAsm);     

    BOOL CmdShowMem(CMD_INFO& CmdInfo, LPVOID lpAddr);  //显示内存
    BOOL CmdShowAsm(CMD_INFO& CmdInfo, LPVOID lpAddr);  //显示反汇编
    BOOL CmdShowReg(CMD_INFO& CmdInfo, LPVOID lpAddr);  //显示寄存器

    BOOL CmdSetNormalBp(CMD_INFO& CmdInfo, LPVOID lpAddr);      //设置一般断点
    BOOL CmdSetHardBp(CMD_INFO& CmdInfo, LPVOID lpAddr);        //设置硬件断点
    BOOL CmdSetMemBp(CMD_INFO& CmdInfo, LPVOID lpAddr);         //设置内存断点
    BOOL CmdSetOneStepInto(CMD_INFO& CmdInfo, LPVOID lpAddr);   //单步步入
    BOOL CmdSetOneStepOver(CMD_INFO& CmdInfo, LPVOID lpAddr);   //单步步过

    BOOL CmdShowNormalBpLst(CMD_INFO& CmdInfo, LPVOID lpAddr);  //显示一般断点
    BOOL CmdShowHardBpLst(CMD_INFO& CmdInfo, LPVOID lpAddr);    //显示硬件断点
    BOOL CmdShowMemBpLst(CMD_INFO& CmdInfo, LPVOID lpAddr);     //显示内存断点

    BOOL CmdClearNormalBp(CMD_INFO& CmdInfo, LPVOID lpAddr);    //删除一般断点
    BOOL CmdClearHardBpLst(CMD_INFO& CmdInfo, LPVOID lpAddr);   //删除硬件断点
    BOOL CmdClearMemBp(CMD_INFO& CmdInfo, LPVOID lpAddr);       //删除内存断点

    BOOL ShowMemLst(CMD_INFO& CmdInfo, LPVOID lpAddr);          //显示内存列表

    BOOL SaveScript(CMD_INFO& CmdInfo, LPVOID lpAddr);          //保存脚本
    BOOL LoadScript(CMD_INFO& CmdInfo, LPVOID lpAddr);          //导入脚本


    static void __stdcall OutErrMsg(_IN_ LPCTSTR strErrMsg);    //输出错误信息

    CPE m_PE; //PE相关

private:
    LPVOID m_lpInstance; //目标的映像基址

    BOOL SetHardBreakPoint(LPVOID lpAddr, BPSTATE dwState, DWORD dwLen = 0);  //设置硬件断点

    CResolveCMD m_CMD;          //解析CMD

    DWORD m_dwErrCount;           //错误计数, 十次错误
    
    HANDLE m_hFile; //目标文件句柄

    DEBUG_EVENT m_DbgEvt;   //目标进程调试事件
    CONTEXT m_DstContext;   //目标进程上下文
    HANDLE m_hDstProcess;   //目标进程当前进程句柄
    HANDLE m_hDstThread;    //目标进程当前线程

    BOOL    m_bIsNormalStep;       //是否为待修补常规断点的单步
    LPVOID  m_lpTmpStepAddr;  //临时存储，修补单步断点的地址

    BOOL m_bIsMyStepInto;       //是否为我自己的单步步入
    BOOL m_bIsMyStepOver;       //是否为我自己的单步步过

    BOOL m_bIsMyHardReSet;         //硬件断点断步配合
    DWORD m_dwWhichHardReg;        //哪个断点被触发

    BOOL m_bIsMemStep;              //内存断点断步配合
    LPVOID m_lpTmpMemExec;          //内存断点记录异常指令位置

    BOOL m_bIsScript;           //是否为脚本状态
    BOOL m_bIsInput;            //且为无输入状态

    CList<PMODLST, PMODLST&>   m_ModuleLst;     //模块链表
    CList<PMYBREAK_POINT, PMYBREAK_POINT&> m_NorMalBpLst;    //一般断点列表
    CList<PMYBREAK_POINT, PMYBREAK_POINT&> m_HardBpLst;      //硬件断点列表
    CList<PMYBREAK_POINT, PMYBREAK_POINT&> m_MemBpLst;      //硬件断点列表

    CList<CString*, CString*&> m_CmdOrderLst;                //命令序列列表
    
    PFN_OpenThread m_pfnOpenThread;
};

#endif // !defined(AFX_DEBUG_H__CCFCD8C7_A313_4B5F_9343_626594F87C8E__INCLUDED_)
