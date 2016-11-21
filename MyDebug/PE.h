// PE.h: interface for the CPE class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PE_H__27F51A13_64AD_4E0C_8E8B_B08D7C582643__INCLUDED_)
#define AFX_PE_H__27F51A13_64AD_4E0C_8E8B_B08D7C582643__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CPE  
{
public:
	CPE(DEBUG_EVENT* pDbgEvt, CONTEXT* pContext, HANDLE* pProcess, HANDLE* pThread);
	virtual ~CPE();

    BOOL Dump(HANDLE hFile, LPVOID pInstance);    //Dump

    DWORD MaxMemCurNum(DWORD dwSrc, DWORD dwDst);   //ת������ֵ
    
    BOOL ReadRemoteMem(LPVOID lpAddr, PCHAR& pBuf, DWORD dwSize);   //���Զ����Ϣ

    void SafeRet(); //��ȫ����
    BOOL GetPeInfo();       //��ȡ��ִ���ļ�����Ϣ
    BOOL CopyInfoToNewFile(DWORD pInstance);    //������Ϣ�����ļ�

    //HANDLE m_hFile; //�ļ����

    DEBUG_EVENT& m_pDbgEvt;   //Ŀ����̵����¼�
    CONTEXT& m_pContext;   //Ŀ�����������
    HANDLE& m_pProcess;   //Ŀ����̵�ǰ���̾��
    HANDLE& m_pThread;    //Ŀ����̵�ǰ�߳�
    
    LPVOID m_lpData;    //�ļ�ӳ���׵�ַ
    HANDLE m_hFileMap;    //�ļ�ӳ����

    LPVOID m_pIns;   //m_pInstance   Ŀ���ַ

    PIMAGE_DOS_HEADER   m_pDosHead;         //Dosͷ
    PIMAGE_NT_HEADERS32   m_pImgHead32;         //32 PEͷ
    PIMAGE_NT_HEADERS64   m_pImgHead64;         //64 PEͷ
    PIMAGE_FILE_HEADER  m_pFileHead;        //�ļ�ͷ
    PIMAGE_OPTIONAL_HEADER32 m_pOption32;    //32ѡ��ͷ
    PIMAGE_OPTIONAL_HEADER64 m_pOption64;    //32ѡ��ͷ
    
    DWORD dwOptionSize;                      //ѡ��ͷ��С
    
    PIMAGE_SECTION_HEADER    m_pSection;     //�ڱ�
    DWORD m_dwSectionCount;                    //�ڱ����
    
    IMAGE_DATA_DIRECTORY (*m_pDataDir)[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];//����Ŀ¼     
    
    BOOL m_bIsLoadFile;               //�Ƿ��Ѿ������ļ�
    
    BOOL m_bIsX64;                    //�Ƿ�Ϊ64λPE
};

#endif // !defined(AFX_PE_H__27F51A13_64AD_4E0C_8E8B_B08D7C582643__INCLUDED_)
