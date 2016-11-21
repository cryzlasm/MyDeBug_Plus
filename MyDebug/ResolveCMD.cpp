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

    //��ȡ����Ͳ�����
    int nPos = CmdInfo.strCMD.Find(TEXT(' '));
    if(nPos != -1)
    {
        //��ȡ����
        CString strOrder = CmdInfo.strCMD.Left(nPos);
        
        //��ȡ������
        CString strOperand = CmdInfo.strCMD.Right(dwStrLen - nPos - 1);
        
        //һ��ϵ�
        if(strOrder == TEXT("bp"))
        {
            CmdInfo.dwState = CMD_BREAK_POINT;
        }
        //Ӳ���ϵ�
        else if(strOrder == TEXT("bh"))
        {
            CmdInfo.dwState = CMD_BP_HARD;
        }
        //�ڴ�ϵ�
        else if(strOrder == TEXT("bm"))
        {
            CmdInfo.dwState = CMD_BP_MEMORY;
        }
        //�鿴�ڴ�
        else if(strOrder == TEXT("d"))
        {
            CmdInfo.dwState = CMD_DISPLAY_DATA;
        }
        //�鿴�����
        else if(strOrder == TEXT("u"))
        {
            CmdInfo.dwState = CMD_DISPLAY_ASMCODE;
        }
        //�޸��ڴ�����
        else if(strOrder == TEXT("e"))
        {
            CmdInfo.dwState = CMD_EDIT_DATA;
        }
        //����
        else if(strOrder == TEXT("trace"))
        {
            CmdInfo.dwState = CMD_TRACE;
        }
        else
        {
            //δ֪����
            bIsSupport = FALSE;
            bRet = FALSE;
            CmdInfo.dwState = CMD_INVALID;
        }

        if(bIsSupport)
            CmdInfo.strCMD = strOperand;
    }
    //������
    else
    {
        //�˳�
        if(CmdInfo.strCMD == TEXT("q") || CmdInfo.strCMD == TEXT("exit"))
        {
            CmdInfo.dwState = CMD_QUIT;
        }
        //��������
        else if(CmdInfo.strCMD == TEXT("t"))
        {
            CmdInfo.dwState = CMD_STEP;
        }
        //��������
        else if(CmdInfo.strCMD == TEXT("p"))
        {
            CmdInfo.dwState = CMD_STEPGO;
        }
        //����
        else if(CmdInfo.strCMD == TEXT("g"))
        {
            CmdInfo.dwState = CMD_RUN;
        }
        //�Ĵ���
        else if(CmdInfo.strCMD == TEXT("r"))
        {
            CmdInfo.dwState = CMD_REGISTER;
        }
        //һ��ϵ�
        else if(CmdInfo.strCMD == TEXT("bpl"))
        {
            CmdInfo.dwState = CMD_BP_LIST;
        }
        //Ӳ���ϵ��
        else if(CmdInfo.strCMD == TEXT("bhl"))
        {
            CmdInfo.dwState = CMD_BP_HARD_LIST;
        }
        //���Ӳ���ϵ��
        else if(CmdInfo.strCMD == TEXT("bhc"))
        {
            CmdInfo.dwState = CMD_CLEAR_BP_HARD;
        }
        //�ڴ�ϵ��
        else if(CmdInfo.strCMD == TEXT("bml"))
        {
            CmdInfo.dwState = CMD_BP_MEMORY_LIST;
        }
        //�ڴ�ϵ��
        else if(CmdInfo.strCMD == TEXT("bmc"))
        {
            CmdInfo.dwState = CMD_CLEAR_BP_MEMORY;
        }
        //����ű�
        else if(CmdInfo.strCMD == TEXT("ls"))
        {
            CmdInfo.dwState = CMD_LOAD_SCRIPT;
        }
        //�����ű�
        else if(CmdInfo.strCMD == TEXT("es"))
        {
            CmdInfo.dwState = CMD_EXPORT_SCRIPT;
        }
        //�鿴ģ��
        else if(CmdInfo.strCMD == TEXT("ml"))
        {
            CmdInfo.dwState = CMD_MODULE_LIST;
        }
        //�޸��ڴ�����
        else if(CmdInfo.strCMD == TEXT("e"))
        {
            CmdInfo.dwState = CMD_EDIT_DATA;
        }
        //��ʾ�ڴ�����
        else if(CmdInfo.strCMD == TEXT("d"))
        {
            CmdInfo.dwState = CMD_DISPLAY_DATA;
        }
        //�����
        else if(CmdInfo.strCMD == TEXT("u"))
        {
            CmdInfo.dwState = CMD_DISPLAY_ASMCODE;
        }
        //���һ��ϵ�
        else if(CmdInfo.strCMD == TEXT("bpc"))
        {
            CmdInfo.dwState = CMD_CLEAR_NORMAL;
        }
        //�����ڴ�ҳ
        else if(CmdInfo.strCMD == TEXT("mil"))
        {
            CmdInfo.dwState = CMD_MEM_INFO_LIST;
        }
        else
        {
            //δ֪����
            bRet = FALSE;
            bIsSupport = FALSE;
            CmdInfo.dwState = CMD_INVALID;
        }

        CmdInfo.strCMD = TEXT("");
    }

    //δ֪����
    if(!bIsSupport)
        tcout << TEXT("��δ֧�ִ�����") << endl;

    return bRet;
}
