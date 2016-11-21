// PE.cpp: implementation of the CPE class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MyDebug.h"
#include "PE.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPE::CPE(DEBUG_EVENT* pDbgEvt, CONTEXT* pContext, HANDLE* pProcess, HANDLE* pThread)
    :m_pContext(*pContext), m_pDbgEvt(*pDbgEvt), m_pProcess(*pProcess), m_pThread(*pThread)
{
    m_lpData = NULL;
    m_hFileMap = INVALID_HANDLE_VALUE;

    m_pDosHead = NULL;         //Dosͷ
    m_pImgHead32 = NULL;         //32 PEͷ
    m_pImgHead64 = NULL;         //64 PEͷ
    m_pFileHead = NULL;        //�ļ�ͷ
    m_pOption32 = NULL;    //32ѡ��ͷ
    m_pOption64 = NULL;    //32ѡ��ͷ
    
    dwOptionSize = 0;                      //ѡ��ͷ��С
    
    m_pSection = NULL;     //�ڱ�
    m_dwSectionCount = 0;                    //�ڱ����
    
    m_pDataDir = NULL;//����Ŀ¼     
    
    m_bIsLoadFile = FALSE;               //�Ƿ��Ѿ������ļ�
    
    m_bIsX64 = FALSE;                    //�Ƿ�Ϊ64λPE
}

CPE::~CPE()
{

}

BOOL CPE::Dump(HANDLE hFile, LPVOID pInstance)
{
    //�����ļ�ӳ��
    m_hFileMap = CreateFileMapping(hFile,         //�ļ����
                            NULL,            //��ȫ����һ��ΪNULL
                            PAGE_READONLY,   //�������ԣ�ֻ��
                            0,               //��Ҫ�ڴ��С�ʹ��ļ���Сһ��
                            0,               //
                            NULL);           //ָ���ļ���
    
    if(m_hFileMap == NULL || m_hFileMap == INVALID_HANDLE_VALUE)
    {
        AfxMessageBox(TEXT("��������BUG ����ϵ����Ա��"));
        //OutErrMsg(TEXT("InitTree: �����ļ�ӳ��ʧ��"));
        return FALSE;
    }
    
    //�����ڴ�ӳ��
    m_lpData = MapViewOfFile(m_hFileMap,        //ӳ��Map�ľ��
                            FILE_MAP_READ,         //ֻ��ҳ
                            0,                     //��ͷ��ʼ��
                            0,                     //
                            0);                    //0 ��ʾȫ��
    if(m_lpData == NULL)
    {
        CloseHandle(m_hFileMap);
        m_hFileMap = INVALID_HANDLE_VALUE;
        
        return FALSE;
    }

    //��ȡ��ִ���ļ�����Ϣ
    if(!GetPeInfo())
    {
        SafeRet();
        return FALSE;
    }

    if(!CopyInfoToNewFile((DWORD)pInstance))
    {
        SafeRet();
        return FALSE;
    }

    SafeRet();
    return TRUE;
}

#define MAXNUM(X, Y) (X > Y ? X : Y)    //�ȴ�С

#define  OUT_FILE_NAME TEXT("Dump.exe")
BOOL CPE::CopyInfoToNewFile(DWORD pInstance)
{
    try
    {
        CString strExt = TEXT("Data Files (*.exe;*.dll)|*.exe;*.dll||All Files (*.*)|*.*||");
        CString strFileName = TEXT("Dump.exe");
        CFileDialog FileDlg(FALSE, strExt, strFileName);
        if(FileDlg.DoModal() != IDOK)
        {
            CDeBug::OutErrMsg(TEXT("CopyInfoToNewFile���û�ȡ��Dump"));
            return FALSE;
        }
        strFileName = FileDlg.GetPathName();

        PCHAR pSrcData = (PCHAR)m_lpData;
        CFile fout;
        fout.Open(strFileName, CFile::modeCreate | CFile::modeReadWrite | CFile::shareDenyRead | CFile::typeBinary);
        fout.SeekToBegin();

        DWORD dwSectionCount = m_pFileHead->NumberOfSections;
        //X32
        if(!m_bIsX64)
        {
            DWORD dwSizeOfHeaders = m_pOption32->SizeOfHeaders;     //�ļ�ͷ��С
            DWORD dwAlignmentOfFile = m_pOption32->FileAlignment;   //�ļ�����
            DWORD dwAlignmentOfMem = m_pOption32->SectionAlignment; //�ڴ����
            
            //д�ļ�ͷ
            fout.Write(m_lpData, dwSizeOfHeaders);
            fout.Flush();
            
            PCHAR lpMemCurBase = NULL;  //��ǰ���ڴ��ַ
            PIMAGE_SECTION_HEADER pTmpSec = m_pSection; //��ʱ�ڱ�ָ��
            DWORD dwSize = 0;   //��������С
            PCHAR pBuf = NULL;  //������

            //�����ڱ�
            while(pTmpSec->VirtualAddress != NULL || pTmpSec->PointerToRawData != NULL)
            {
                //�ڴ��ַ
                lpMemCurBase = (PCHAR)pTmpSec->VirtualAddress;  //RVA
                lpMemCurBase += pInstance;

                //��ȡ�ļ�ʵ�ʴ�С
                dwSize = pTmpSec->SizeOfRawData;

                //ȡ�ļ�����ֵ
                dwSize = MaxMemCurNum(dwSize, dwAlignmentOfFile);

                //���뻺����
                pBuf = new CHAR[dwSize];
                if(pBuf == NULL)
                {
                    CDeBug::OutErrMsg(TEXT("CopyInfoToNewFile: ���뻺����ʧ��! "));
                }
                ZeroMemory(pBuf, dwSize);

                //���ڴ�
                if(!ReadRemoteMem(lpMemCurBase, pBuf, dwSize))
                {
                    break;
                }

                //д�ļ�
                fout.Write(pBuf, dwSize);
                fout.Flush();

                //�ͷŻ�����
                if(pBuf != NULL)
                {
                    delete pBuf;
                    pBuf = NULL;
                }

                //�ƽṹ��ָ��
                pTmpSec++;
            }
            
        }
        else
        {
            
        }

        //�ر��ļ�
        fout.Close();
    }
    catch(CFileException & e)
    {
        char szBuf[MAX_PATH] = {0};
        e.GetErrorMessage(szBuf, MAX_PATH);
        CString strOut = szBuf;
        tcout << strOut << endl;
        return FALSE;
    }

    tcout << TEXT("Dump�ɹ�!") << endl;
    return TRUE;
}

DWORD CPE::MaxMemCurNum(DWORD dwSrc, DWORD dwDst) //ת������ֵ
{
    DWORD dwRet = 0;
    DWORD dwResidue = dwSrc % dwDst;    //ȡ��
    DWORD dwNum = dwSrc / dwDst;        //ȡ����
    if(dwResidue != 0)  //������
    {
        dwRet = dwNum * dwDst + MAXNUM(dwResidue, dwDst);
    }
    else    //������
    {
        dwRet = dwSrc;
    }
    return dwRet;
}

BOOL CPE::ReadRemoteMem(LPVOID lpAddr, PCHAR& pBuf, DWORD dwSize)
{
    if(!ReadProcessMemory(m_pProcess, lpAddr, pBuf, dwSize, NULL))
    {
        CDeBug::OutErrMsg(TEXT("ReadRemoteMem: ��ȡԶ���ڴ�ʧ��"));
        return FALSE;
    }

    return TRUE;
}

BOOL CPE::GetPeInfo()
{
    
    //Dosͷ
    m_pDosHead = (PIMAGE_DOS_HEADER)m_lpData;
    if(!m_bIsX64)
    {
        //���32λNTͷ
        m_pImgHead32 = (PIMAGE_NT_HEADERS32)((PCHAR)m_lpData + m_pDosHead->e_lfanew);
        
        //����ļ�ͷ
        m_pFileHead = &m_pImgHead32->FileHeader;
        
        //���32λѡ��ͷ
        m_pOption32 = &m_pImgHead32->OptionalHeader;
        
        //���Ŀ¼��ʼλ��
        m_pDataDir = &m_pOption32->DataDirectory;
        
        //���ѡ��ͷ��С
        dwOptionSize = m_pFileHead->SizeOfOptionalHeader;
        
        //��ý�������
        m_dwSectionCount = m_pFileHead->NumberOfSections;
        
        //��ý�����ʼλ��
        m_pSection = (PIMAGE_SECTION_HEADER)((PCHAR)m_pOption32 + dwOptionSize);
    }
    else
    {
        //���64λNTͷ
        m_pImgHead64 = (PIMAGE_NT_HEADERS64)((PCHAR)m_lpData + m_pDosHead->e_lfanew);
        
        //����ļ�ͷ
        m_pFileHead = &m_pImgHead64->FileHeader;
        
        //���64λѡ��ͷ
        m_pOption64 = &m_pImgHead64->OptionalHeader;
        
        //���Ŀ¼�׵�ַ
        m_pDataDir = &m_pOption64->DataDirectory;
        
        //���ѡ��ͷ��С
        dwOptionSize = m_pFileHead->SizeOfOptionalHeader;
        
        //��ý�������
        m_dwSectionCount = m_pFileHead->NumberOfSections;
        
        //��ý�����ʼλ��
        m_pSection = (PIMAGE_SECTION_HEADER)((PCHAR)m_pOption64 + dwOptionSize);
    }
    
    m_bIsLoadFile = TRUE;
    return TRUE;
}

void CPE::SafeRet()
{
    //�ر��ڴ�ӳ��
    if(m_lpData != NULL)
        UnmapViewOfFile(m_lpData);
    
    //�رվ��
    if(m_hFileMap != INVALID_HANDLE_VALUE)
        CloseHandle(m_hFileMap);
}