// PE.h: interface for the CPE class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PE_H__27F51A13_64AD_4E0C_8E8B_B08D7C582643__INCLUDED_)
#define AFX_PE_H__27F51A13_64AD_4E0C_8E8B_B08D7C582643__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define ONE_PAGE (0x1000)   //一个分页
class CPE  
{
public:
	CPE(DEBUG_EVENT* pDbgEvt, CONTEXT* pContext, HANDLE* pProcess, HANDLE* pThread);
	virtual ~CPE();

    BOOL Dump(HANDLE hFile, LPVOID pInstance);    //Dump

    DWORD MaxMemCurNum(DWORD dwSrc, DWORD dwDst);   //转换对齐值
    
    BOOL ReadRemoteMem(LPVOID lpAddr, PCHAR pBuf, DWORD dwSize);   //获得远程信息

    void SafeRet(HANDLE hFile = INVALID_HANDLE_VALUE); //安全返回
    BOOL GetPeInfo(LPVOID lpData);       //获取可执行文件的信息
    BOOL CopyInfoToNewFile(DWORD pInstance);    //拷贝信息到新文件

    BOOL InitModLst(CList<PMOD_INFO, PMOD_INFO&>& pModLst); //初始化模块链表

    BOOL GetRemoteFSAddr(DWORD& dwOutAddr);    //获取远程fs段地址
    BOOL GetLdrDataTable(DWORD& dwOutAddr);    //获取LDR

    BOOL GetModInfo(CList<PMOD_INFO, PMOD_INFO&>& pModLst, LPVOID lpLdrAddr);//获取模块信息
    BOOL GetFunLstOfMod(CList<PMOD_INFO, PMOD_INFO&>& pModLst);    //获取模块的导出函数
    BOOL GetFunLst(CList<PMOD_EXPORT_FUN, PMOD_EXPORT_FUN&>& pModLst, CString& strFilePath);  //解析一个模块的导出表
    BOOL WalkExportTabel(CList<PMOD_EXPORT_FUN, PMOD_EXPORT_FUN&>& pModLst);

    BOOL ShowLst(CList<PMOD_INFO, PMOD_INFO&>& pModLst);
    BOOL ConvertRvaToFa(PIMAGE_SECTION_HEADER pSection, DWORD dwSectionCount, DWORD dwRva, DWORD& dwOutFa);//转换 RVA -> FA
    BOOL ConvertFaToRva(PIMAGE_SECTION_HEADER pSection, DWORD dwSectionCount, DWORD dwFa, DWORD& dwOutRva);//转换 FA -> RVA
    
    //修复导入表
    BOOL FixImport(CList<PMOD_INFO, PMOD_INFO&>& pModLst, DWORD lpImportAddr, DWORD dwSize);
    BOOL ResolveIAT(CList<PMOD_INFO, PMOD_INFO&>& pModLst, PDWORD pStart, PDWORD pEnd);  //解析IAT
    BOOL ConvertIAT(CString& strFile, DWORD dwIATBase);  //转换IAT 开文件
    BOOL CPE::HandleIAT(LPVOID lpBuf, DWORD dwIATBase);  //转换IAT 到导入表 中间表

    PDWORD m_pdwImport;     //读取的远程导入表

    CList<PIMPORT_MOD_INFO, PIMPORT_MOD_INFO&> m_ImportLst;     //修复的导入表链表

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
