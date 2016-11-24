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

    m_pdwImport = NULL;

}

CPE::~CPE()
{
    if(m_pdwImport != NULL)
    {
        delete m_pdwImport;
    }

    POSITION pos = m_ImportLst.GetHeadPosition();
    while(pos)
    {
        PIMPORT_MOD_INFO pMod = m_ImportLst.GetNext(pos);
        
        if(pMod != NULL)
        {
            POSITION subPos = pMod->FunLst.GetHeadPosition();
            while(subPos)
            {
                PIMPORT_FUN_INFO pFun = pMod->FunLst.GetNext(subPos);
                if(pFun != NULL)
                {
                    delete pFun;
                    pFun = NULL;
                }
            }

            delete pMod;
            pMod = NULL;
        }
    }
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

        //遍历函数表
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
    
    //节内偏移
    DWORD dwRvaOfSection = dwRva - pTmpSection->VirtualAddress;
    dwOutFa = dwRvaOfSection + pTmpSection->PointerToRawData;
    
    return TRUE;
}

BOOL CPE::ConvertFaToRva(PIMAGE_SECTION_HEADER pSection, DWORD dwSectionCount, DWORD dwFa, DWORD& dwOutRva)
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
        dwLowAddr = pTmpSection->PointerToRawData;     //节起始位置
        dwHiAddr = pTmpSection->PointerToRawData + pTmpSection->SizeOfRawData;    //节结束位置
        if(dwFa >= dwLowAddr && dwFa < dwHiAddr)
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
        dwOutRva = 0;
        return FALSE;
    }
    
    //节内偏移
    DWORD dwRvaOfSection = dwFa - pTmpSection->PointerToRawData;
    dwOutRva = dwRvaOfSection + pTmpSection->VirtualAddress;
    
    return TRUE;
}

//#define ONE
//#define TWO
#define THREE

BOOL CPE::ShowLst(CList<PMOD_INFO, PMOD_INFO&>& pModLst)
{
#ifdef ONE
    POSITION pos = m_ImportLst.GetHeadPosition();
    while(pos)
    {
        PIMPORT_MOD_INFO pMod = m_ImportLst.GetNext(pos);
        
        tcout << TEXT("库名：") << (LPCTSTR)pMod->strModName << endl;
        
        tcout << TEXT("函数个数：") << pMod->dwFunCount << endl;

        POSITION subPos = pMod->FunLst.GetHeadPosition();
        while(subPos)
        {
            PIMPORT_FUN_INFO pFun = pMod->FunLst.GetNext(subPos);
            //_tprintf(TEXT("\tRVA: %08X"), pFun->dwOrdinal);
            if(pFun->bIsName)
            {
                tcout << TEXT("\t函数名：") << (LPCTSTR)pFun->strFunName << endl;
            }
            else
            {
                CString strNum = TEXT("");
                strNum.Format(TEXT("%04X"), pFun->dwOrdinal);
                tcout << TEXT("\t序号：") << (LPCTSTR)strNum << endl;
            }
        }

        //system("pause");
    }
#endif

#ifdef TWO
    POSITION pos = pModLst.GetHeadPosition();
    while(pos)
    {
        PMOD_INFO pMod = pModLst.GetNext(pos);

        
        POSITION subPos = pMod->FunLst.GetHeadPosition();
        while(subPos)
        {
            PMOD_EXPORT_FUN pFun = pMod->FunLst.GetNext(subPos);
            //_tprintf(TEXT("\tRVA: %08X"), pFun->dwOrdinal);
            if(pFun->bIsName)
            {
                tcout << TEXT("\t函数名：") << (LPCTSTR)pFun->strFunName << endl;
            }
            else
            {
                CString strNum = TEXT("");
                strNum.Format(TEXT("%04X"), pFun->dwOrdinal);
                tcout << TEXT("\t序号：") << (LPCTSTR)strNum << endl;
            }
        }
        
        tcout << TEXT("库名：") << (LPCTSTR)pMod->strModName << endl;
        
        tcout << TEXT("函数个数：") << pMod->dwFunCount << endl;
        
        system("pause");
    }
#endif

#ifdef THREE
    POSITION pos = m_ImportLst.GetHeadPosition();
    while(pos)
    {
        PIMPORT_MOD_INFO pMod = m_ImportLst.GetNext(pos);
        
        tcout << TEXT("库名：") << (LPCTSTR)pMod->strModName << endl;
        
        tcout << TEXT("修复成功：") <<  pMod->dwFixSucceed << endl;

        tcout << TEXT("修复失败：") <<  pMod->dwFixFailure << endl;
        
        tcout << TEXT("函数个数：") << pMod->dwFunCount << endl;
        
        //system("pause");
    }
#endif

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


BOOL CPE::FixImport(CList<PMOD_INFO, PMOD_INFO&>& pModLst, DWORD lpImportAddr, DWORD dwSize)
{
    if(m_pdwImport != NULL)
    {
        delete m_pdwImport;
        m_pdwImport = NULL;
    }

    //去除尾数 % DWORD 对齐
    dwSize = (dwSize / sizeof(DWORD)) * sizeof(DWORD);
    
    //申请缓冲区
    m_pdwImport = (PDWORD)new CHAR[dwSize];
    if(m_pdwImport == NULL)
    {
        CDeBug::OutErrMsg(TEXT("FixImport：申请空间失败！"));
        return FALSE;
    }
    ZeroMemory((PCHAR)m_pdwImport, dwSize);

    //读远程内存
    if(!ReadRemoteMem((LPVOID)lpImportAddr, (PCHAR)m_pdwImport, dwSize))
    {
        CDeBug::OutErrMsg(TEXT("FixImport：读取远程内存失败！"));
        return FALSE;
    }

    //解析IAT
    PDWORD pTmpImport = m_pdwImport;
    PCHAR  pEnd = (PCHAR)m_pdwImport + dwSize;
    
    if(!ResolveIAT(pModLst, pTmpImport, (PDWORD)pEnd))
    {
        CDeBug::OutErrMsg(TEXT("FixImport：转换IAT失败！"));
        return FALSE;
    }

    tcout << TEXT("选择文件\r\n文件名：") << endl;
    CFileDialog FileDlg(TRUE);
    
    //获取文件名
    CString strFilePath = TEXT("");
    if(FileDlg.DoModal() == IDOK)
    {
        strFilePath = FileDlg.GetPathName();
    }
    
    tcout << (LPCTSTR)strFilePath << endl;

    CString strNewFile = strFilePath + TEXT(".bak");

    //备份文件
    if(!CopyFile(strFilePath, strNewFile, FALSE))
    {
        CDeBug::OutErrMsg(TEXT("FixImport：备份文件失败"));
        return FALSE;
    }

    //遍历模块，转换IAT 的 VA 为 RVA
    POSITION pos = pModLst.GetHeadPosition();
    while(pos)
    {
        PMOD_INFO pMod = pModLst.GetNext(pos);

        //判断VA 命中
        if(lpImportAddr >= pMod->dwModBaseAddr &&
            lpImportAddr < (pMod->dwModBaseAddr + pMod->dwModSize))
        {
            lpImportAddr -= pMod->dwModBaseAddr;
            break;
        }
    }

    //转换IAT
    if(!ConvertIAT(strFilePath, lpImportAddr))
    {
        //删除备份
        //DeleteFile(strNewFile);
        CDeBug::OutErrMsg(TEXT("FixImport：转换IAT到中间表失败！"));
        return FALSE;
    }

    ShowLst(pModLst);
    return TRUE;
}

BOOL CPE::ConvertIAT(CString& strFile, DWORD dwIATBase)
{
    //获取文件大小
    DWORD dwFileSize = 0;
    try
    {
        CFile File;
        File.Open(strFile, CFile::modeRead);
        dwFileSize = File.GetLength();
        File.Close();
    }
    catch(...)
    {}

    //打开文件 读文件
    HANDLE hFile = CreateFile(strFile,              //路径
        GENERIC_READ | GENERIC_WRITE,                  //读方式打开
        FILE_SHARE_READ | FILE_SHARE_WRITE,               //共享读
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
    
    //预留空间留待写导入表
    dwFileSize += ONE_PAGE * 3;

    //创建文件映射
    m_hFileMap = CreateFileMapping(hFile,         //文件句柄
        NULL,            //安全属性一般为NULL
        PAGE_READWRITE,   //保护属性，只读
        0,               //需要内存大小和打开文件大小一样
        dwFileSize,               //
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
        FILE_MAP_READ | FILE_MAP_WRITE,         //只读页
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
    
    //转换到IAT
    if(!HandleIAT(m_lpData, dwIATBase))
    {
        tcout << TEXT("遭遇严重BUG 请联系管理员。") << endl;
        CDeBug::OutErrMsg(TEXT("GetFunLst: 获取文件信息"));
        SafeRet(hFile);
        return FALSE;
    }
    
    //ShowLst(pModLst);
    SafeRet(hFile);
    return TRUE;
}

BOOL CPE::HandleIAT(LPVOID lpBuf, DWORD dwIATBase)
{
    DWORD dwRva = 0;        //RVA 缓冲区
    DWORD dwFA = 0;         //FA  缓冲区
    DWORD dwNumOfIAT = 0;   //IAT 下标
    //找到最后一个节表
    PIMAGE_SECTION_HEADER pTmpSection = m_pSection;
    DWORD dwSectionCount = m_dwSectionCount;
    while(dwSectionCount-- != 1)
    {
        pTmpSection++;
    }
    
    if(!m_bIsX64)
    {
        try
        {
            //修改SizeOfImage
            DWORD dwSizeOfImage = m_pOption32->SizeOfImage;
            dwSizeOfImage += ONE_PAGE * 2;
            m_pOption32->SizeOfImage = dwSizeOfImage;

            //获取内存文件节区大小
            DWORD dwVirtualSize = pTmpSection->Misc.VirtualSize;
            dwVirtualSize = MaxMemCurNum(dwVirtualSize, m_pOption32->SectionAlignment);
            dwVirtualSize += ONE_PAGE * 2;
            pTmpSection->Misc.VirtualSize = dwVirtualSize;


            //获取文件节区大小
            DWORD dwSectionSize = pTmpSection->SizeOfRawData;
            DWORD dwSize = 0;   //对齐后的大小
            
            //对齐到文件尾
            dwSize = MaxMemCurNum(dwSectionSize, m_pOption32->FileAlignment);
            
            //文件末尾的位置，新开分页的起始位置
            DWORD dwFileBase = dwSectionSize + pTmpSection->PointerToRawData;
            
            
            //增加三个分页作为导入表的信息
            //导入表的起始
            PCHAR pImport = (PCHAR) lpBuf + dwSize + pTmpSection->PointerToRawData;
            dwSize += ONE_PAGE;

            ZeroMemory(pImport, ONE_PAGE);

            //字符串起始
            PCHAR pStr = (PCHAR) lpBuf + dwSize + pTmpSection->PointerToRawData;
            dwSize += ONE_PAGE;
            ZeroMemory(pImport, ONE_PAGE);

            //IAT起始
            dwRva = dwIATBase;  //RVA
            if(!ConvertRvaToFa(m_pSection, m_dwSectionCount, dwRva, dwFA))
            {
                CDeBug::OutErrMsg(TEXT("HandleIAT: 转换FA到RVA失败"));
                return FALSE;
            }
            PCHAR pIAT = (PCHAR)lpBuf + dwFA;
            ZeroMemory(pImport, ONE_PAGE);
            //dwSize += ONE_PAGE;

            //填写文件大小
            pTmpSection->SizeOfRawData = dwSize;

//             //置属性
//             SECTION_NATURE& SecNature = *(PSECTION_NATURE)&pTmpSection->Characteristics;
//             SecNature.m_dwRead = TRUE;
//             SecNature.m_dwWrite = TRUE;
//             SecNature.m_dwExecute = TRUE;


            //1、pStr位置填写模块名，函数名
            //2、pIAT位置填写函数名的RVA
            //3、pImport 位置 填写 导入表
            
            DWORD dwImportSize = 0;         //导入表大小
            DWORD dwBase = (DWORD)lpBuf;    //文件基址
            PDWORD pdwTmpIAT = (PDWORD)pIAT;  //IAT指针
            PCHAR  pszTmpStr = (PCHAR)pStr;   //Str指针
            PIMAGE_IMPORT_DESCRIPTOR pTmpImport = (PIMAGE_IMPORT_DESCRIPTOR)pImport;    //导入表指针
            
            DWORD dwIAT = 0;
            DWORD dwStrCount = 0;

            //遍历模块信息
            POSITION pos = m_ImportLst.GetHeadPosition();
            while(pos)
            {
                PIMPORT_MOD_INFO pMod = m_ImportLst.GetNext(pos);

                //从模块导入函数小于1个，则为不填写导入表
                if(pMod->dwFunCount < 1)
                    continue;

                //设置库名RVA
                dwFA = (DWORD)pszTmpStr - dwBase;
                if(!ConvertFaToRva(m_pSection, m_dwSectionCount, dwFA, dwRva))
                {
                    CDeBug::OutErrMsg(TEXT("HandleIAT: 转换FA到RVA失败"));
                    return FALSE;
                }
                pTmpImport->Name = dwRva;       //库名
                
                //建立IAT关系
                dwFA = (DWORD)pdwTmpIAT - dwBase;
                if(!ConvertFaToRva(m_pSection, m_dwSectionCount, dwFA, dwRva))
                {
                    CDeBug::OutErrMsg(TEXT("HandleIAT: 转换FA到RVA失败"));
                    return FALSE;
                }
                pTmpImport->FirstThunk = dwRva;     //IAT

                //拷贝库名
                dwStrCount = pMod->strModName.GetLength() + 1;
                memcpy(pszTmpStr, (LPCTSTR)pMod->strModName, dwStrCount);
                pszTmpStr += dwStrCount;

                //模2对齐 补0
                if(dwStrCount % 2 != 0)
                {
                    pszTmpStr++;
                }
                
                //遍历从模块中导入的函数
                POSITION subPos = pMod->FunLst.GetHeadPosition();
                while(subPos)
                {
                    PIMPORT_FUN_INFO pFun = pMod->FunLst.GetNext(subPos);
                    
                    SYS_IMPORT_FUN_INFO SysFunInfo = {0};
                    
                    if(dwNumOfIAT == pFun->dwNumOfIAT)
                    {
                        //为名称，高位为0
                        if(pFun->bIsName)
                        {
                            //设置IAT数据
                            SysFunInfo.m_dwState = 0;
                            dwFA = (DWORD)pszTmpStr - dwBase;
                            if(!ConvertFaToRva(m_pSection, m_dwSectionCount, dwFA, dwRva))
                            {
                                CDeBug::OutErrMsg(TEXT("HandleIAT: 转换FA到RVA失败"));
                                return FALSE;
                            }
                            SysFunInfo.m_dwNum = dwRva;     //IAT INFO
                            
                            //拷贝函数名
                            dwStrCount = pFun->strFunName.GetLength() + 1;
                            WORD wHint = dwStrCount;
                            
                            //Hint
                            memcpy(pszTmpStr, (PCHAR)&wHint, sizeof(WORD));
                            pszTmpStr += sizeof(WORD);
                            
                            memcpy(pszTmpStr, (LPCTSTR)pFun->strFunName, dwStrCount);
                            pszTmpStr += dwStrCount;
                            
                            //模2对齐 补0
                            if(dwStrCount % 2 != 0)
                            {
                                pszTmpStr++;
                            }
                            
                            *pdwTmpIAT = *(PDWORD)&SysFunInfo;
                        }
                        //为序号，高位为1
                        else
                        {
                            //设置序号
                            SysFunInfo.m_dwState = 1;
                            SysFunInfo.m_dwState = pFun->dwOrdinal;
                            
                            *pdwTmpIAT = *(PDWORD)&SysFunInfo;
                        }
                    }//End if(dwNumOfIAT == pFun->dwNumOfIAT)
                    
                    //推IAT指针
                    pdwTmpIAT++;
                    dwNumOfIAT++;
                }//End while(subPos)

                //IAT尾部补NULL
                *pdwTmpIAT = NULL;

                //推IAT指针
                pdwTmpIAT++;
                dwNumOfIAT++;

                //推导入表结构体
                pTmpImport++;

                //导入表大小自加
                dwImportSize += sizeof(IMAGE_IMPORT_DESCRIPTOR);
            }

            //修改数据目录
            dwFA = (DWORD)pImport - dwBase;
            if(!ConvertFaToRva(m_pSection, m_dwSectionCount, dwFA, dwRva))
            {
                CDeBug::OutErrMsg(TEXT("HandleIAT: 转换FA到RVA失败"));
                return FALSE;
            }
            IMAGE_DATA_DIRECTORY& pImportDir = (*m_pDataDir)[IMAGE_DIRECTORY_ENTRY_IMPORT];
            pImportDir.VirtualAddress = dwRva;
            pImportDir.Size = dwImportSize + sizeof(IMAGE_IMPORT_DESCRIPTOR);

        }
        catch (...)
        {
        }
    }
    else
    {
        
    }

    return TRUE;
}

BOOL CPE::ResolveIAT(CList<PMOD_INFO, PMOD_INFO&>& pModLst, PDWORD pStart, PDWORD pEnd)
{
    DWORD dwTime = 0;   //次数

    //保留开始位置
    PDWORD pTmpStart = pStart;

    //函数位置序号
    DWORD dwNum = 0;
    while(pStart < pEnd)
    {
        if(*pStart == NULL)
        {
            pStart++;
//             if(pStart >= pEnd && dwTime < 1)
//             {
//                 pStart = pTmpStart;
//                 dwTime++;
//             }
            continue;
        }

        PIMPORT_MOD_INFO pDstMod = new IMPORT_MOD_INFO;
        if(pDstMod == NULL)
        {
            CDeBug::OutErrMsg(TEXT("ResolveIAT： 申请函数空间失败"));
            return FALSE;
        }
        PIMPORT_FUN_INFO pDstFun = NULL;
        
        //初始化计数器
        DWORD dwImportFixSucceed = 0; //成功个数
        DWORD dwImportAllCount   = 0; //总个数

        //遍历模块
        POSITION pos = pModLst.GetHeadPosition();
        while(pos)
        {
            BOOL bIsFunFind = FALSE;    // 是否找到函数
            BOOL bIsModFind = FALSE;    // 是否找到模块

            //判断模块命中
            PMOD_INFO pSrcMod = pModLst.GetNext(pos);

            DWORD dwModBase = pSrcMod->dwModBaseAddr;
            if(*pStart >= pSrcMod->dwModBaseAddr &&
               *pStart < (pSrcMod->dwModBaseAddr + pSrcMod->dwModSize))
            {
                
                while(TRUE)
                {
                    pDstFun = new IMPORT_FUN_INFO;
                    if(pDstFun == NULL)
                    {
                        CDeBug::OutErrMsg(TEXT("ResolveIAT： 申请函数空间失败"));
                        return FALSE;
                    }

                    //遍历函数表
                    POSITION subPos = pSrcMod->FunLst.GetHeadPosition();
                    while(subPos)
                    {
                        PMOD_EXPORT_FUN pSrcFun = pSrcMod->FunLst.GetNext(subPos);

                        //判断函数命中
                        if(*pStart == pSrcFun->dwFunAddr)
                        {
                            pDstFun->bIsName = pSrcFun->bIsName;
                            pDstFun->dwFunAddrRVA = pSrcFun->dwFunAddr - dwModBase;
                            pDstFun->dwOrdinal = pSrcFun->dwOrdinal;
                            pDstFun->strFunName = pSrcFun->strFunName;
                            pDstFun->dwNumOfIAT = dwNum;

                            bIsFunFind = TRUE;
                            bIsModFind = TRUE;

                            dwImportFixSucceed++;
                            
                            //找到一个则IAT清除一个
                            *pStart = 0;

                            pDstMod->FunLst.AddTail(pDstFun);
                            break;
                        }
                    }// End while(subPos)
                
                    // 推指针
                    pStart++;
                    dwNum++;
                    dwImportAllCount++;
                
                    //当前模块结束
                    if(*pStart == NULL || pStart >= pEnd)
                    {
                        if(!bIsFunFind)
                        {
                            if(pDstFun != NULL)
                            {
                                delete pDstFun;
                                pDstFun = NULL;
                            }
                        }
                        
                        if(bIsModFind)
                        {
                            pDstMod->dwFixFailure = dwImportAllCount - dwImportFixSucceed;
                            pDstMod->dwFixSucceed = dwImportFixSucceed;
                            pDstMod->dwFunCount = pDstMod->FunLst.GetCount();
                        }
                        break;
                    }
                }//End While(TRUE)
            }//End if

            //找到
            if(bIsModFind)
            {
                pDstMod->dwModBase = pSrcMod->dwModBaseAddr;    //模块基址
                pDstMod->dwModSize = pSrcMod->dwModSize;        //模块大小
                pDstMod->strModName = pSrcMod->strModName;      //模块名
                break;
            }

        }//End while(pos)

        //如果获取到函数，则加链表，没获取则释放资源
        if(pDstMod->dwFunCount < 1)
        {
            delete pDstMod;
            pDstMod = NULL;
        }
        else
        {
            m_ImportLst.AddTail(pDstMod);
        }

        //推指针
        pStart++;
        dwNum++;

//         if(pStart >= pEnd && dwTime < 1)
//         {
//             pStart = pTmpStart;
//             dwTime++;
//         }
    }// End while(pStart < pEnd)
    return TRUE;
}