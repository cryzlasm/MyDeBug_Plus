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

    m_pDosHead = NULL;         //Dos头
    m_pImgHead32 = NULL;         //32 PE头
    m_pImgHead64 = NULL;         //64 PE头
    m_pFileHead = NULL;        //文件头
    m_pOption32 = NULL;    //32选项头
    m_pOption64 = NULL;    //32选项头
    
    dwOptionSize = 0;                      //选项头大小
    
    m_pSection = NULL;     //节表
    m_dwSectionCount = 0;                    //节表个数
    
    m_pDataDir = NULL;//数据目录     
    
    m_bIsLoadFile = FALSE;               //是否已经加载文件
    
    m_bIsX64 = FALSE;                    //是否为64位PE
}

CPE::~CPE()
{

}

BOOL CPE::Dump(HANDLE hFile, LPVOID pInstance)
{
    //创建文件映射
    m_hFileMap = CreateFileMapping(hFile,         //文件句柄
                            NULL,            //安全属性一般为NULL
                            PAGE_READONLY,   //保护属性，只读
                            0,               //需要内存大小和打开文件大小一样
                            0,               //
                            NULL);           //指定文件名
    
    if(m_hFileMap == NULL || m_hFileMap == INVALID_HANDLE_VALUE)
    {
        AfxMessageBox(TEXT("遭遇严重BUG 请联系管理员。"));
        //OutErrMsg(TEXT("InitTree: 创建文件映射失败"));
        return FALSE;
    }
    
    //建立内存映射
    m_lpData = MapViewOfFile(m_hFileMap,        //映射Map的句柄
                            FILE_MAP_READ,         //只读页
                            0,                     //从头开始读
                            0,                     //
                            0);                    //0 表示全读
    if(m_lpData == NULL)
    {
        CloseHandle(m_hFileMap);
        m_hFileMap = INVALID_HANDLE_VALUE;
        
        return FALSE;
    }

    //获取可执行文件的信息
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

#define MAXNUM(X, Y) (X > Y ? X : Y)    //比大小

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
            CDeBug::OutErrMsg(TEXT("CopyInfoToNewFile：用户取消Dump"));
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
            DWORD dwSizeOfHeaders = m_pOption32->SizeOfHeaders;     //文件头大小
            DWORD dwAlignmentOfFile = m_pOption32->FileAlignment;   //文件对齐
            DWORD dwAlignmentOfMem = m_pOption32->SectionAlignment; //内存对齐
            
            //写文件头
            fout.Write(m_lpData, dwSizeOfHeaders);
            fout.Flush();
            
            PCHAR lpMemCurBase = NULL;  //当前的内存基址
            PIMAGE_SECTION_HEADER pTmpSec = m_pSection; //临时节表指针
            DWORD dwSize = 0;   //缓冲区大小
            PCHAR pBuf = NULL;  //缓冲区

            //遍历节表
            while(pTmpSec->VirtualAddress != NULL || pTmpSec->PointerToRawData != NULL)
            {
                //内存基址
                lpMemCurBase = (PCHAR)pTmpSec->VirtualAddress;  //RVA
                lpMemCurBase += pInstance;

                //获取文件实际大小
                dwSize = pTmpSec->SizeOfRawData;

                //取文件对齐值
                dwSize = MaxMemCurNum(dwSize, dwAlignmentOfFile);

                //申请缓冲区
                pBuf = new CHAR[dwSize];
                if(pBuf == NULL)
                {
                    CDeBug::OutErrMsg(TEXT("CopyInfoToNewFile: 申请缓冲区失败! "));
                }
                ZeroMemory(pBuf, dwSize);

                //读内存
                if(!ReadRemoteMem(lpMemCurBase, pBuf, dwSize))
                {
                    break;
                }

                //写文件
                fout.Write(pBuf, dwSize);
                fout.Flush();

                //释放缓冲区
                if(pBuf != NULL)
                {
                    delete pBuf;
                    pBuf = NULL;
                }

                //推结构体指针
                pTmpSec++;
            }
            
        }
        else
        {
            
        }

        //关闭文件
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

    tcout << TEXT("Dump成功!") << endl;
    return TRUE;
}

DWORD CPE::MaxMemCurNum(DWORD dwSrc, DWORD dwDst) //转换对齐值
{
    DWORD dwRet = 0;
    DWORD dwResidue = dwSrc % dwDst;    //取余
    DWORD dwNum = dwSrc / dwDst;        //取倍数
    if(dwResidue != 0)  //有余数
    {
        dwRet = dwNum * dwDst + MAXNUM(dwResidue, dwDst);
    }
    else    //无余数
    {
        dwRet = dwSrc;
    }
    return dwRet;
}

BOOL CPE::ReadRemoteMem(LPVOID lpAddr, PCHAR& pBuf, DWORD dwSize)
{
    if(!ReadProcessMemory(m_pProcess, lpAddr, pBuf, dwSize, NULL))
    {
        CDeBug::OutErrMsg(TEXT("ReadRemoteMem: 读取远程内存失败"));
        return FALSE;
    }

    return TRUE;
}

BOOL CPE::GetPeInfo()
{
    
    //Dos头
    m_pDosHead = (PIMAGE_DOS_HEADER)m_lpData;
    if(!m_bIsX64)
    {
        //获得32位NT头
        m_pImgHead32 = (PIMAGE_NT_HEADERS32)((PCHAR)m_lpData + m_pDosHead->e_lfanew);
        
        //获得文件头
        m_pFileHead = &m_pImgHead32->FileHeader;
        
        //获得32位选项头
        m_pOption32 = &m_pImgHead32->OptionalHeader;
        
        //获得目录起始位置
        m_pDataDir = &m_pOption32->DataDirectory;
        
        //获得选项头大小
        dwOptionSize = m_pFileHead->SizeOfOptionalHeader;
        
        //获得节区总数
        m_dwSectionCount = m_pFileHead->NumberOfSections;
        
        //获得节区起始位置
        m_pSection = (PIMAGE_SECTION_HEADER)((PCHAR)m_pOption32 + dwOptionSize);
    }
    else
    {
        //获得64位NT头
        m_pImgHead64 = (PIMAGE_NT_HEADERS64)((PCHAR)m_lpData + m_pDosHead->e_lfanew);
        
        //获得文件头
        m_pFileHead = &m_pImgHead64->FileHeader;
        
        //获得64位选项头
        m_pOption64 = &m_pImgHead64->OptionalHeader;
        
        //获得目录首地址
        m_pDataDir = &m_pOption64->DataDirectory;
        
        //获得选项头大小
        dwOptionSize = m_pFileHead->SizeOfOptionalHeader;
        
        //获得节区总数
        m_dwSectionCount = m_pFileHead->NumberOfSections;
        
        //获得节区起始位置
        m_pSection = (PIMAGE_SECTION_HEADER)((PCHAR)m_pOption64 + dwOptionSize);
    }
    
    m_bIsLoadFile = TRUE;
    return TRUE;
}

void CPE::SafeRet()
{
    //关闭内存映射
    if(m_lpData != NULL)
        UnmapViewOfFile(m_lpData);
    
    //关闭句柄
    if(m_hFileMap != INVALID_HANDLE_VALUE)
        CloseHandle(m_hFileMap);
}