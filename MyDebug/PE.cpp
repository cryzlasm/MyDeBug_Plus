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
        CDeBug::OutErrMsg(TEXT("InitTree: 创建文件映射失败"));
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
    if(!GetPeInfo(m_lpData))
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
                    tcout << TEXT("Dump失败") << endl;
                    return FALSE;
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

BOOL CPE::ReadRemoteMem(LPVOID lpAddr, PCHAR pBuf, DWORD dwSize)
{
    DWORD dwOutSize = 0;
    if(!ReadProcessMemory(m_pProcess, lpAddr, pBuf, dwSize, &dwOutSize))
    {
        CDeBug::OutErrMsg(TEXT("ReadRemoteMem: 读取远程内存失败"));
        return FALSE;
    }

    if(dwOutSize != dwSize)
    {
        CDeBug::OutErrMsg(TEXT("ReadRemoteMem：读取数据不完整"));
        return FALSE;
    }

    return TRUE;
}


BOOL CPE::InitModLst(CList<PMOD_INFO, PMOD_INFO&>& pModLst) //初始化模块链表
{
    LPVOID lpLdrAddr = 0;
    if(!GetLdrDataTable((DWORD&)lpLdrAddr))
    {
        return FALSE;
    }
    
    if(!GetModInfo(pModLst, lpLdrAddr))
    {
        return FALSE;
    }

    if(!GetFunLstOfMod(pModLst))
    {
        return FALSE;
    }

    //ShowLst(pModLst);

    return TRUE;
}

BOOL CPE::GetFunLst(CList<PMOD_EXPORT_FUN, PMOD_EXPORT_FUN&>& pModLst, CString& strFilePath)
{
    //打开文件 读文件
    HANDLE hFile = CreateFile(strFilePath,              //路径
                              GENERIC_READ,                  //读方式打开
                              FILE_SHARE_READ,               //共享读
                              NULL,                          //安全属性一般为NULL
                              OPEN_EXISTING,                 //只读取已存在文件
                              FILE_FLAG_SEQUENTIAL_SCAN,     //顺序读取
                              NULL);                         //一般为NULL
    
    if(hFile == INVALID_HANDLE_VALUE)
    {
        tcout << TEXT("遭遇严重BUG 请联系管理员。") << endl;
        CDeBug::OutErrMsg(TEXT("GetFunLst: 打开文件失败"));
        return FALSE;
    }

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
        CDeBug::OutErrMsg(TEXT("InitTree: 创建文件映射失败"));
        SafeRet(hFile);
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
        SafeRet(hFile);
        //CloseHandle(m_hFileMap);
        //m_hFileMap = INVALID_HANDLE_VALUE;
        return FALSE;
    }
    
    //获取可执行文件的信息
    if(!GetPeInfo(m_lpData))
    {
        tcout << TEXT("遭遇严重BUG 请联系管理员。") << endl;
        CDeBug::OutErrMsg(TEXT("GetFunLst: 获取文件信息"));
        SafeRet(hFile);
        return FALSE;
    }

    //遍历导出表
    if(!WalkExportTabel(pModLst))
    {
        tcout << TEXT("遭遇严重BUG 请联系管理员。") << endl;
        CDeBug::OutErrMsg(TEXT("GetFunLst: 获取导出表失败"));
        SafeRet(hFile);
        return FALSE;
    }

    //ShowLst(pModLst);
    SafeRet(hFile);
    return TRUE;
}

BOOL CPE::WalkExportTabel(CList<PMOD_EXPORT_FUN, PMOD_EXPORT_FUN&>& pModLst)
{
    //获取导出表
    DWORD dwExportAddr = m_pDataDir[IMAGE_DIRECTORY_ENTRY_EXPORT]->VirtualAddress;
    DWORD dwExportSize = m_pDataDir[IMAGE_DIRECTORY_ENTRY_EXPORT]->Size;
    
    DWORD dwFa = 0;

    //有导出
    if(dwExportAddr != NULL && dwExportSize != 0)
    {
        //获取文件地址
        if(!ConvertRvaToFa(m_pSection, m_dwSectionCount, dwExportAddr, dwFa))
        {
            CDeBug::OutErrMsg(TEXT("WalkExportTabel: 获取导出表失败"));
            return FALSE;
        }

        //加文件基址
        dwFa += (DWORD)m_lpData;

        //导入表信息
        PIMAGE_EXPORT_DIRECTORY pExport = (PIMAGE_EXPORT_DIRECTORY)dwFa;
        DWORD dwBase = pExport->Base;                               //序号基址
        DWORD dwFunCount = pExport->NumberOfFunctions;              //多少个导出
        DWORD dwNameCount = pExport->NumberOfNames;                 //多少个有名字的导出
        DWORD dwFunLstAddr = pExport->AddressOfFunctions;           //函数表地址
        DWORD dwNameLstAddr = pExport->AddressOfNames;              //名称表地址
        DWORD dwOrdinalsLstAddr = pExport->AddressOfNameOrdinals;   //序号表地址
        
        //小于一个导出，则不解析
        if(dwFunCount < 1)
            return TRUE;


        PDWORD lpFunLst = NULL;
        PDWORD lpNameLst = NULL;        
        PWORD lpOrdinalLst = NULL;

        //获取函数地址表的文件地址
        if(!ConvertRvaToFa(m_pSection, m_dwSectionCount, dwFunLstAddr, dwFa))
        {
            CDeBug::OutErrMsg(TEXT("WalkExportTabel: 获取函数表失败"));
            return FALSE;
        }
        lpFunLst = (PDWORD)((PCHAR)m_lpData + dwFa);

        //获取名称地址表的文件地址
        if(!ConvertRvaToFa(m_pSection, m_dwSectionCount, dwNameLstAddr, dwFa))
        {
            CDeBug::OutErrMsg(TEXT("WalkExportTabel: 获取名称表失败"));
            return FALSE;
        }
        lpNameLst = (PDWORD)((PCHAR)m_lpData + dwFa);

        //获取序号表的文件地址
        if(!ConvertRvaToFa(m_pSection, m_dwSectionCount, dwOrdinalsLstAddr, dwFa))
        {
            CDeBug::OutErrMsg(TEXT("WalkExportTabel: 获取序号表失败"));
            return FALSE;
        }
        lpOrdinalLst = (PWORD)((PCHAR)m_lpData + dwFa);

        for(DWORD i = 0; i < dwFunCount; i++)
        {
            if(lpFunLst[i] == NULL)
                continue;

            //申请信息表，并初始化
            PMOD_EXPORT_FUN pFun = new MOD_EXPORT_FUN;
            if(pFun == NULL)
            {
                tcout << TEXT("遭遇严重BUG 请联系管理员。") << endl; 
                CDeBug::OutErrMsg(TEXT("WalkExportTabel: 申请结构体失败"));
                return FALSE;
            }
            pFun->bIsName = FALSE;
            pFun->dwOrdinal = -1;
            pFun->strFunName = TEXT("");

            //函数地址
            pFun->dwFunAddr = (DWORD)m_pIns + lpFunLst[i];
            
            //遍历序号表，查函数表的下标
            BOOL bIsFind = FALSE;
            DWORD j = 0;
            for(j = 0; j < dwNameCount; j++)
            {
                if(lpOrdinalLst[j] == i)
                {
                    bIsFind = TRUE;
                    break;
                }
            }

            //名称
            if(bIsFind)
            {
                //同下标查名称表，如果为空，则为序号，不为空则为名称
                if(lpNameLst[j] != NULL)
                {
                    //获取文件地址
                    if(!ConvertRvaToFa(m_pSection, m_dwSectionCount, lpNameLst[j], dwFa))
                    {
                        CDeBug::OutErrMsg(TEXT("WalkExportTabel: 获取序号表失败"));
                        return FALSE;
                    }
                    pFun->strFunName = (PCHAR)m_lpData + dwFa;
                    
                    pFun->bIsName = TRUE;
                }
            }//End if(bIsFind)
            //序号
            else
            {
                pFun->bIsName = FALSE;
            }
            
            //序号
            pFun->dwOrdinal = i + dwBase;

            pModLst.AddTail(pFun);
        }//End for(int i = 0; i < dwFunCount; i++)

    }//End if(dwExportAddr != NULL && dwExportSize != 0)

    return TRUE;
}

//解析 模块导出表
BOOL CPE::GetFunLstOfMod(CList<PMOD_INFO, PMOD_INFO&>& pModLst)
{
    POSITION pos = pModLst.GetHeadPosition();
    while(pos)
    {
        PMOD_INFO pModInfo = pModLst.GetNext(pos);
        m_pIns = (LPVOID)pModInfo->dwModBaseAddr;
        if(!GetFunLst(pModInfo->FunLst, pModInfo->strModPath))
        {
            return FALSE;
        }
    
        //获取函数个数
        pModInfo->dwFunCount = pModInfo->FunLst.GetCount();
    }
    return TRUE;
}

BOOL CPE::ConvertRvaToFa(PIMAGE_SECTION_HEADER pSection, DWORD dwSectionCount, DWORD dwRva, DWORD& dwOutFa)
{
    if(dwSectionCount < 1)
    {
        return FALSE;
    }
    
    PIMAGE_SECTION_HEADER pTmpSection = pSection;
    
    DWORD dwLowAddr = 0;
    DWORD dwHiAddr = 0;
    BOOL bIsFind = FALSE;
    
    //遍历节区表，判断命中
    for(DWORD i = 0; i < dwSectionCount; i++)
    {
        dwLowAddr = pTmpSection->VirtualAddress;     //节起始位置
        dwHiAddr = pTmpSection->VirtualAddress + pTmpSection->Misc.VirtualSize;    //节结束位置
        if(dwRva >= dwLowAddr && dwRva < dwHiAddr)
        {
            bIsFind = TRUE;
            break;
        }
        
        //推指针
        pTmpSection++;
    }
    
    //是否找到
    if(!bIsFind)
    {
        dwOutFa = 0;
        return FALSE;
    }
    
    DWORD dwRvaOfSection = dwRva - pTmpSection->VirtualAddress;
    dwOutFa = dwRvaOfSection + pTmpSection->PointerToRawData;
    
    return TRUE;
}

BOOL CPE::ShowLst(CList<PMOD_EXPORT_FUN, PMOD_EXPORT_FUN&>& pModLst)
{
    POSITION pos = pModLst.GetHeadPosition();
    while(pos)
    {
        PMOD_EXPORT_FUN pMod = pModLst.GetNext(pos);
        
        tcout << TEXT("序号：") << pMod->dwOrdinal << endl;
        
        CString strTmp = TEXT("");
        strTmp.Format(TEXT("地址: %08X"), pMod->dwFunAddr - (DWORD)m_pIns);
        tcout << (LPCTSTR)strTmp << endl;
        if(pMod->bIsName)
        {
            tcout << (LPCTSTR)pMod->strFunName << endl;
        }

        //system("pause");
    }

    return TRUE;
}

BOOL CPE::GetModInfo(CList<PMOD_INFO, PMOD_INFO&>& pModLst, LPVOID lpLdrAddr)
{
    //缓冲区
    //char pBuf[sizeof(MODULE_INFO_EX)];
    //ZeroMemory(pBuf, sizeof(pBuf));
    MODULE_LDR_INFO ModLdrInfo;
    ZeroMemory(&ModLdrInfo, sizeof(ModLdrInfo));
    
    LPVOID lpTmpAddr = lpLdrAddr;
    while(TRUE)
    {
        //读表
        if(!ReadRemoteMem(lpTmpAddr, (PCHAR)&ModLdrInfo, sizeof(MODULE_LDR_INFO)))
            return FALSE;

        //下一个表的位置
        lpTmpAddr = (LPVOID)ModLdrInfo.pForwardLst.Flink;
        
        //模块信息表
        PMOD_INFO pModInfo = new MOD_INFO;
        if(pModInfo == NULL)
        {
            CDeBug::OutErrMsg(TEXT("GetModInfo: 申请结构体空间失败"));
            return FALSE;
        }

        if(ModLdrInfo.lpMouduleBase == NULL)
        {
            //下一个链表的地址为基址，则退出
            if(lpTmpAddr == lpLdrAddr)
            {
                break;
            }
            continue;
        }

        pModInfo->dwModBaseAddr = (DWORD)ModLdrInfo.lpMouduleBase;          //基址
        pModInfo->dwlpEntryPoint = (DWORD)ModLdrInfo.lpMouduleEntryPoint;   //入口
        pModInfo->dwModSize = ModLdrInfo.dwMouduleSize;                     //大小
        
        DWORD dwNameSize = ModLdrInfo.dwNameStrSize; //名称大小
        DWORD dwPathSize = ModLdrInfo.dwPathStrSize; //路径大小
        
        //模块名缓冲区
        PCHAR pNameBuf = new CHAR[dwNameSize];
        if(pNameBuf == NULL)
        {
            CDeBug::OutErrMsg(TEXT("GetModInfo: 申请名称缓冲区失败"));
            return FALSE;
        }
        ZeroMemory(pNameBuf, dwNameSize);
        
        //获得模块名
        if(!ReadRemoteMem((LPVOID)ModLdrInfo.bstrFileName, pNameBuf, dwNameSize))
            return FALSE;
        
        _bstr_t bstrFileName = (LPCWSTR)pNameBuf;
        pModInfo->strModName = (LPSTR)bstrFileName;

        //模块路径缓冲区
        PCHAR pPathBuf = new CHAR[dwPathSize];
        if(pPathBuf == NULL)
        {
            CDeBug::OutErrMsg(TEXT("GetModInfo: 申请名称缓冲区失败"));
            return FALSE;
        }
        ZeroMemory(pPathBuf, dwNameSize);
        
        //获得模块路径
        if(!ReadRemoteMem((LPVOID)ModLdrInfo.bstrFilePath, pPathBuf, dwPathSize))
            return FALSE;
        
        bstrFileName = (LPCWSTR)pPathBuf;
        pModInfo->strModPath = (LPSTR)bstrFileName;


        //加链表
        pModLst.AddTail(pModInfo);

        //释放资源
        if(pNameBuf != NULL)
        {
            delete pNameBuf;
            pNameBuf = NULL;
        }

        if(pPathBuf != NULL)
        {
            delete pPathBuf;
            pPathBuf = NULL;
        }
    }

    return TRUE;
}

BOOL CPE::GetPeInfo(LPVOID lpData)
{
    //Dos头
    m_pDosHead = (PIMAGE_DOS_HEADER)lpData;
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

BOOL CPE::GetRemoteFSAddr(DWORD& dwOutAddr)    //获取远程FS地址
{
    LDT_ENTRY  ldtEntry = {0};
    
    if(!GetThreadSelectorEntry(m_pThread, m_pContext.SegFs, &ldtEntry))
    {
        dwOutAddr = 0;
        return FALSE;
    }

    dwOutAddr = (ldtEntry.HighWord.Bits.BaseHi << 24) | 
                (ldtEntry.HighWord.Bits.BaseMid << 16) |
                ldtEntry.BaseLow;
    return TRUE;
}

//获取模块信息表
BOOL CPE::GetLdrDataTable(DWORD& dwOutAddr)
{
    //获取FS地址
    DWORD dwFs = 0;
    if(!GetRemoteFSAddr(dwFs))
    {
        CDeBug::OutErrMsg(TEXT("GetLdrDataTable: 获取FS段地址失败！"));
        return FALSE;
    }

    DWORD dwAddr = dwFs + 0x18; // NT_TIB

    DWORD dwReadLen = 0;
    try
    {
        //获取NT_TIB
        if(ReadProcessMemory(m_pProcess, (LPVOID)dwAddr, &dwAddr,
            sizeof(DWORD),&dwReadLen) == NULL)
        {
            return FALSE;
        }
        
        //获取peb
        if(ReadProcessMemory(m_pProcess, (LPVOID)(dwAddr + 0x30), &dwAddr,
            sizeof(DWORD),&dwReadLen) == NULL)
        {
            return FALSE;
        }
        //获取Ldr PEB_LDR_DATA 链表 
        if(ReadProcessMemory(m_pProcess, (LPVOID)(dwAddr + 0xC), &dwAddr,
            sizeof(DWORD),&dwReadLen) == NULL)
        {
            return FALSE;
        }
        //获取 _LDR_DATA_TABLE_ENTRY dwAddr + 0xC
        if(ReadProcessMemory(m_pProcess, (LPVOID)(dwAddr + 0xC), &dwAddr,
            sizeof(DWORD),&dwReadLen) == NULL)
        {
            return FALSE;
        }
        dwOutAddr = dwAddr;
    }
    catch(...)
    {
        return FALSE;
    }
    return TRUE;
}

void CPE::SafeRet(HANDLE hFile)
{
    //关闭内存映射
    if(m_lpData != NULL)
        UnmapViewOfFile(m_lpData);
    
    //关闭句柄
    if(m_hFileMap != INVALID_HANDLE_VALUE)
        CloseHandle(m_hFileMap);

    //关闭句柄
    if(hFile != NULL && hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
}