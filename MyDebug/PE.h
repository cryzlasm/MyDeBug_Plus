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

    DWORD MaxMemCurNum(DWORD dwSrc, DWORD dwDst);   //转换对齐值
    
    BOOL ReadRemoteMem(LPVOID lpAddr, PCHAR& pBuf, DWORD dwSize);   //获得远程信息

    void SafeRet(); //安全返回
    BOOL GetPeInfo();       //获取可执行文件的信息
    BOOL CopyInfoToNewFile(DWORD pInstance);    //拷贝信息到新文件

    //HANDLE m_hFile; //文件句柄

    DEBUG_EVENT& m_pDbgEvt;   //目标进程调试事件
    CONTEXT& m_pContext;   //目标进程上下文
    HANDLE& m_pProcess;   //目标进程当前进程句柄
    HANDLE& m_pThread;    //目标进程当前线程
    
    LPVOID m_lpData;    //文件映射首地址
    HANDLE m_hFileMap;    //文件映射句柄

    LPVOID m_pIns;   //m_pInstance   目标基址

    PIMAGE_DOS_HEADER   m_pDosHead;         //Dos头
    PIMAGE_NT_HEADERS32   m_pImgHead32;         //32 PE头
    PIMAGE_NT_HEADERS64   m_pImgHead64;         //64 PE头
    PIMAGE_FILE_HEADER  m_pFileHead;        //文件头
    PIMAGE_OPTIONAL_HEADER32 m_pOption32;    //32选项头
    PIMAGE_OPTIONAL_HEADER64 m_pOption64;    //32选项头
    
    DWORD dwOptionSize;                      //选项头大小
    
    PIMAGE_SECTION_HEADER    m_pSection;     //节表
    DWORD m_dwSectionCount;                    //节表个数
    
    IMAGE_DATA_DIRECTORY (*m_pDataDir)[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];//数据目录     
    
    BOOL m_bIsLoadFile;               //是否已经加载文件
    
    BOOL m_bIsX64;                    //是否为64位PE
};

#endif // !defined(AFX_PE_H__27F51A13_64AD_4E0C_8E8B_B08D7C582643__INCLUDED_)
