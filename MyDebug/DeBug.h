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

#define _OUT_       //�������
#define _IN_        //�������
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
    BOOL GetFun();                      //��ȡOpenThread
    BOOL Start(_IN_ TCHAR* argv[]);		//����ʼ
    BOOL EventLoop();                   //�¼���Ϣѭ��
    BOOL Interaction(LPVOID lpAddr, BOOL bIsShowDbgInfo = TRUE);                 //�˻�����
    BOOL GetUserInput(CMD_INFO& CmdInfo);                //��ȡ�û�����, ��������״̬
    BOOL HandleCmd(CMD_INFO& CmdInfo, LPVOID lpAddr);          //ִ������
    
    //д��Զ���ֽڣ����ؾ��ֽ�
    BOOL WriteRemoteCode(_IN_ LPVOID lpRemoteAddr, _IN_ DWORD btInChar, _OUT_ DWORD& pbtOutChar);
    //�жϵ�ַ�Ƿ���������
    BOOL IsAddrInBpList(_IN_ LPVOID lpAddr,
                        _IN_ CList<PMYBREAK_POINT,
                        PMYBREAK_POINT&>& bpSrcLst,
                        _OUT_ POSITION& dwOutPos,
                        BOOL bIsNextAddr =  FALSE);
    
    BOOL OnExceptionEvent();        //�����쳣�¼�
    
    BOOL OnCreateProcessEvent();    //�����������¼�
    
    BOOL OnLoadDll();               //ģ�����
    BOOL OnUnLoadDll();             //ģ��ж��
    BOOL ShowDllLst();              //��ʾ��ǰ���ڵ�ģ����Ϣ
    
    BOOL OnBreakPointEvent();       //һ��ϵ�
    BOOL OnSingleStepEvent();       //�����쳣
    BOOL OnAccessVolationEvent();   //�ڴ�����쳣
    
    //��ʾ��ǰ���е�����Ϣ,Ĭ����ʾһ��
    BOOL ShowCurAllDbg(LPVOID lpAddr, CMDSTATE cmdState = CMD_SHOWONCE);    
    
    BOOL ShowRemoteMem(LPVOID lpAddr, DWORD& dwOutCurAddr);           //��ʾԶ���ڴ�
    BOOL ShowRemoteReg();                        //��ʾԶ�̼Ĵ���

    //��ʾԶ�̷����,Ĭ��ʮ��
    BOOL ShowRemoteDisAsm(LPVOID lpAddr, DWORD& dwOutCurAddr, DWORD dwAsmCount = 10);      

    //�����ָ����ַһ������
    BOOL GetOneAsm(LPVOID lpAddr, DWORD& dwOrderCount, CString& strOutAsm);     

    BOOL CmdShowMem(CMD_INFO& CmdInfo, LPVOID lpAddr);  //��ʾ�ڴ�
    BOOL CmdShowAsm(CMD_INFO& CmdInfo, LPVOID lpAddr);  //��ʾ�����
    BOOL CmdShowReg(CMD_INFO& CmdInfo, LPVOID lpAddr);  //��ʾ�Ĵ���

    BOOL CmdSetNormalBp(CMD_INFO& CmdInfo, LPVOID lpAddr);      //����һ��ϵ�
    BOOL CmdSetHardBp(CMD_INFO& CmdInfo, LPVOID lpAddr);        //����Ӳ���ϵ�
    BOOL CmdSetMemBp(CMD_INFO& CmdInfo, LPVOID lpAddr);         //�����ڴ�ϵ�
    BOOL CmdSetOneStepInto(CMD_INFO& CmdInfo, LPVOID lpAddr);   //��������
    BOOL CmdSetOneStepOver(CMD_INFO& CmdInfo, LPVOID lpAddr);   //��������

    BOOL CmdShowNormalBpLst(CMD_INFO& CmdInfo, LPVOID lpAddr);  //��ʾһ��ϵ�
    BOOL CmdShowHardBpLst(CMD_INFO& CmdInfo, LPVOID lpAddr);    //��ʾӲ���ϵ�
    BOOL CmdShowMemBpLst(CMD_INFO& CmdInfo, LPVOID lpAddr);     //��ʾ�ڴ�ϵ�

    BOOL CmdClearNormalBp(CMD_INFO& CmdInfo, LPVOID lpAddr);    //ɾ��һ��ϵ�
    BOOL CmdClearHardBpLst(CMD_INFO& CmdInfo, LPVOID lpAddr);   //ɾ��Ӳ���ϵ�
    BOOL CmdClearMemBp(CMD_INFO& CmdInfo, LPVOID lpAddr);       //ɾ���ڴ�ϵ�

    BOOL ShowMemLst(CMD_INFO& CmdInfo, LPVOID lpAddr);          //��ʾ�ڴ��б�

    BOOL SaveScript(CMD_INFO& CmdInfo, LPVOID lpAddr);          //����ű�
    BOOL LoadScript(CMD_INFO& CmdInfo, LPVOID lpAddr);          //����ű�


    static void __stdcall OutErrMsg(_IN_ LPCTSTR strErrMsg);    //���������Ϣ

    CPE m_PE; //PE���

private:
    LPVOID m_lpInstance; //Ŀ���ӳ���ַ

    BOOL SetHardBreakPoint(LPVOID lpAddr, BPSTATE dwState, DWORD dwLen = 0);  //����Ӳ���ϵ�

    CResolveCMD m_CMD;          //����CMD

    DWORD m_dwErrCount;           //�������, ʮ�δ���
    
    HANDLE m_hFile; //Ŀ���ļ����

    DEBUG_EVENT m_DbgEvt;   //Ŀ����̵����¼�
    CONTEXT m_DstContext;   //Ŀ�����������
    HANDLE m_hDstProcess;   //Ŀ����̵�ǰ���̾��
    HANDLE m_hDstThread;    //Ŀ����̵�ǰ�߳�

    BOOL    m_bIsNormalStep;       //�Ƿ�Ϊ���޲�����ϵ�ĵ���
    LPVOID  m_lpTmpStepAddr;  //��ʱ�洢���޲������ϵ�ĵ�ַ

    BOOL m_bIsMyStepInto;       //�Ƿ�Ϊ���Լ��ĵ�������
    BOOL m_bIsMyStepOver;       //�Ƿ�Ϊ���Լ��ĵ�������

    BOOL m_bIsMyHardReSet;         //Ӳ���ϵ�ϲ����
    DWORD m_dwWhichHardReg;        //�ĸ��ϵ㱻����

    BOOL m_bIsMemStep;              //�ڴ�ϵ�ϲ����
    LPVOID m_lpTmpMemExec;          //�ڴ�ϵ��¼�쳣ָ��λ��

    BOOL m_bIsScript;           //�Ƿ�Ϊ�ű�״̬
    BOOL m_bIsInput;            //��Ϊ������״̬

    CList<PMODLST, PMODLST&>   m_ModuleLst;     //ģ������
    CList<PMYBREAK_POINT, PMYBREAK_POINT&> m_NorMalBpLst;    //һ��ϵ��б�
    CList<PMYBREAK_POINT, PMYBREAK_POINT&> m_HardBpLst;      //Ӳ���ϵ��б�
    CList<PMYBREAK_POINT, PMYBREAK_POINT&> m_MemBpLst;      //Ӳ���ϵ��б�

    CList<CString*, CString*&> m_CmdOrderLst;                //���������б�
    
    PFN_OpenThread m_pfnOpenThread;
};

#endif // !defined(AFX_DEBUG_H__CCFCD8C7_A313_4B5F_9343_626594F87C8E__INCLUDED_)
