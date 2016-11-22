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
        CDeBug::OutErrMsg(TEXT("InitTree: �����ļ�ӳ��ʧ��"));
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
                    tcout << TEXT("Dumpʧ��") << endl;
                    return FALSE;
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

BOOL CPE::ReadRemoteMem(LPVOID lpAddr, PCHAR pBuf, DWORD dwSize)
{
    DWORD dwOutSize = 0;
    if(!ReadProcessMemory(m_pProcess, lpAddr, pBuf, dwSize, &dwOutSize))
    {
        CDeBug::OutErrMsg(TEXT("ReadRemoteMem: ��ȡԶ���ڴ�ʧ��"));
        return FALSE;
    }

    if(dwOutSize != dwSize)
    {
        CDeBug::OutErrMsg(TEXT("ReadRemoteMem����ȡ���ݲ�����"));
        return FALSE;
    }

    return TRUE;
}


BOOL CPE::InitModLst(CList<PMOD_INFO, PMOD_INFO&>& pModLst) //��ʼ��ģ������
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
    //���ļ� ���ļ�
    HANDLE hFile = CreateFile(strFilePath,              //·��
                              GENERIC_READ,                  //����ʽ��
                              FILE_SHARE_READ,               //�����
                              NULL,                          //��ȫ����һ��ΪNULL
                              OPEN_EXISTING,                 //ֻ��ȡ�Ѵ����ļ�
                              FILE_FLAG_SEQUENTIAL_SCAN,     //˳���ȡ
                              NULL);                         //һ��ΪNULL
    
    if(hFile == INVALID_HANDLE_VALUE)
    {
        tcout << TEXT("��������BUG ����ϵ����Ա��") << endl;
        CDeBug::OutErrMsg(TEXT("GetFunLst: ���ļ�ʧ��"));
        return FALSE;
    }

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
        CDeBug::OutErrMsg(TEXT("InitTree: �����ļ�ӳ��ʧ��"));
        SafeRet(hFile);
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
        SafeRet(hFile);
        //CloseHandle(m_hFileMap);
        //m_hFileMap = INVALID_HANDLE_VALUE;
        return FALSE;
    }
    
    //��ȡ��ִ���ļ�����Ϣ
    if(!GetPeInfo(m_lpData))
    {
        tcout << TEXT("��������BUG ����ϵ����Ա��") << endl;
        CDeBug::OutErrMsg(TEXT("GetFunLst: ��ȡ�ļ���Ϣ"));
        SafeRet(hFile);
        return FALSE;
    }

    //����������
    if(!WalkExportTabel(pModLst))
    {
        tcout << TEXT("��������BUG ����ϵ����Ա��") << endl;
        CDeBug::OutErrMsg(TEXT("GetFunLst: ��ȡ������ʧ��"));
        SafeRet(hFile);
        return FALSE;
    }

    //ShowLst(pModLst);
    SafeRet(hFile);
    return TRUE;
}

BOOL CPE::WalkExportTabel(CList<PMOD_EXPORT_FUN, PMOD_EXPORT_FUN&>& pModLst)
{
    //��ȡ������
    DWORD dwExportAddr = m_pDataDir[IMAGE_DIRECTORY_ENTRY_EXPORT]->VirtualAddress;
    DWORD dwExportSize = m_pDataDir[IMAGE_DIRECTORY_ENTRY_EXPORT]->Size;
    
    DWORD dwFa = 0;

    //�е���
    if(dwExportAddr != NULL && dwExportSize != 0)
    {
        //��ȡ�ļ���ַ
        if(!ConvertRvaToFa(m_pSection, m_dwSectionCount, dwExportAddr, dwFa))
        {
            CDeBug::OutErrMsg(TEXT("WalkExportTabel: ��ȡ������ʧ��"));
            return FALSE;
        }

        //���ļ���ַ
        dwFa += (DWORD)m_lpData;

        //�������Ϣ
        PIMAGE_EXPORT_DIRECTORY pExport = (PIMAGE_EXPORT_DIRECTORY)dwFa;
        DWORD dwBase = pExport->Base;                               //��Ż�ַ
        DWORD dwFunCount = pExport->NumberOfFunctions;              //���ٸ�����
        DWORD dwNameCount = pExport->NumberOfNames;                 //���ٸ������ֵĵ���
        DWORD dwFunLstAddr = pExport->AddressOfFunctions;           //�������ַ
        DWORD dwNameLstAddr = pExport->AddressOfNames;              //���Ʊ��ַ
        DWORD dwOrdinalsLstAddr = pExport->AddressOfNameOrdinals;   //��ű��ַ
        
        //С��һ���������򲻽���
        if(dwFunCount < 1)
            return TRUE;


        PDWORD lpFunLst = NULL;
        PDWORD lpNameLst = NULL;        
        PWORD lpOrdinalLst = NULL;

        //��ȡ������ַ����ļ���ַ
        if(!ConvertRvaToFa(m_pSection, m_dwSectionCount, dwFunLstAddr, dwFa))
        {
            CDeBug::OutErrMsg(TEXT("WalkExportTabel: ��ȡ������ʧ��"));
            return FALSE;
        }
        lpFunLst = (PDWORD)((PCHAR)m_lpData + dwFa);

        //��ȡ���Ƶ�ַ����ļ���ַ
        if(!ConvertRvaToFa(m_pSection, m_dwSectionCount, dwNameLstAddr, dwFa))
        {
            CDeBug::OutErrMsg(TEXT("WalkExportTabel: ��ȡ���Ʊ�ʧ��"));
            return FALSE;
        }
        lpNameLst = (PDWORD)((PCHAR)m_lpData + dwFa);

        //��ȡ��ű���ļ���ַ
        if(!ConvertRvaToFa(m_pSection, m_dwSectionCount, dwOrdinalsLstAddr, dwFa))
        {
            CDeBug::OutErrMsg(TEXT("WalkExportTabel: ��ȡ��ű�ʧ��"));
            return FALSE;
        }
        lpOrdinalLst = (PWORD)((PCHAR)m_lpData + dwFa);

        for(DWORD i = 0; i < dwFunCount; i++)
        {
            if(lpFunLst[i] == NULL)
                continue;

            //������Ϣ������ʼ��
            PMOD_EXPORT_FUN pFun = new MOD_EXPORT_FUN;
            if(pFun == NULL)
            {
                tcout << TEXT("��������BUG ����ϵ����Ա��") << endl; 
                CDeBug::OutErrMsg(TEXT("WalkExportTabel: ����ṹ��ʧ��"));
                return FALSE;
            }
            pFun->bIsName = FALSE;
            pFun->dwOrdinal = -1;
            pFun->strFunName = TEXT("");

            //������ַ
            pFun->dwFunAddr = (DWORD)m_pIns + lpFunLst[i];
            
            //������ű��麯������±�
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

            //����
            if(bIsFind)
            {
                //ͬ�±�����Ʊ����Ϊ�գ���Ϊ��ţ���Ϊ����Ϊ����
                if(lpNameLst[j] != NULL)
                {
                    //��ȡ�ļ���ַ
                    if(!ConvertRvaToFa(m_pSection, m_dwSectionCount, lpNameLst[j], dwFa))
                    {
                        CDeBug::OutErrMsg(TEXT("WalkExportTabel: ��ȡ��ű�ʧ��"));
                        return FALSE;
                    }
                    pFun->strFunName = (PCHAR)m_lpData + dwFa;
                    
                    pFun->bIsName = TRUE;
                }
            }//End if(bIsFind)
            //���
            else
            {
                pFun->bIsName = FALSE;
            }
            
            //���
            pFun->dwOrdinal = i + dwBase;

            pModLst.AddTail(pFun);
        }//End for(int i = 0; i < dwFunCount; i++)

    }//End if(dwExportAddr != NULL && dwExportSize != 0)

    return TRUE;
}

//���� ģ�鵼����
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
    
        //��ȡ��������
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
    
    //�����������ж�����
    for(DWORD i = 0; i < dwSectionCount; i++)
    {
        dwLowAddr = pTmpSection->VirtualAddress;     //����ʼλ��
        dwHiAddr = pTmpSection->VirtualAddress + pTmpSection->Misc.VirtualSize;    //�ڽ���λ��
        if(dwRva >= dwLowAddr && dwRva < dwHiAddr)
        {
            bIsFind = TRUE;
            break;
        }
        
        //��ָ��
        pTmpSection++;
    }
    
    //�Ƿ��ҵ�
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
        
        tcout << TEXT("��ţ�") << pMod->dwOrdinal << endl;
        
        CString strTmp = TEXT("");
        strTmp.Format(TEXT("��ַ: %08X"), pMod->dwFunAddr - (DWORD)m_pIns);
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
    //������
    //char pBuf[sizeof(MODULE_INFO_EX)];
    //ZeroMemory(pBuf, sizeof(pBuf));
    MODULE_LDR_INFO ModLdrInfo;
    ZeroMemory(&ModLdrInfo, sizeof(ModLdrInfo));
    
    LPVOID lpTmpAddr = lpLdrAddr;
    while(TRUE)
    {
        //����
        if(!ReadRemoteMem(lpTmpAddr, (PCHAR)&ModLdrInfo, sizeof(MODULE_LDR_INFO)))
            return FALSE;

        //��һ�����λ��
        lpTmpAddr = (LPVOID)ModLdrInfo.pForwardLst.Flink;
        
        //ģ����Ϣ��
        PMOD_INFO pModInfo = new MOD_INFO;
        if(pModInfo == NULL)
        {
            CDeBug::OutErrMsg(TEXT("GetModInfo: ����ṹ��ռ�ʧ��"));
            return FALSE;
        }

        if(ModLdrInfo.lpMouduleBase == NULL)
        {
            //��һ������ĵ�ַΪ��ַ�����˳�
            if(lpTmpAddr == lpLdrAddr)
            {
                break;
            }
            continue;
        }

        pModInfo->dwModBaseAddr = (DWORD)ModLdrInfo.lpMouduleBase;          //��ַ
        pModInfo->dwlpEntryPoint = (DWORD)ModLdrInfo.lpMouduleEntryPoint;   //���
        pModInfo->dwModSize = ModLdrInfo.dwMouduleSize;                     //��С
        
        DWORD dwNameSize = ModLdrInfo.dwNameStrSize; //���ƴ�С
        DWORD dwPathSize = ModLdrInfo.dwPathStrSize; //·����С
        
        //ģ����������
        PCHAR pNameBuf = new CHAR[dwNameSize];
        if(pNameBuf == NULL)
        {
            CDeBug::OutErrMsg(TEXT("GetModInfo: �������ƻ�����ʧ��"));
            return FALSE;
        }
        ZeroMemory(pNameBuf, dwNameSize);
        
        //���ģ����
        if(!ReadRemoteMem((LPVOID)ModLdrInfo.bstrFileName, pNameBuf, dwNameSize))
            return FALSE;
        
        _bstr_t bstrFileName = (LPCWSTR)pNameBuf;
        pModInfo->strModName = (LPSTR)bstrFileName;

        //ģ��·��������
        PCHAR pPathBuf = new CHAR[dwPathSize];
        if(pPathBuf == NULL)
        {
            CDeBug::OutErrMsg(TEXT("GetModInfo: �������ƻ�����ʧ��"));
            return FALSE;
        }
        ZeroMemory(pPathBuf, dwNameSize);
        
        //���ģ��·��
        if(!ReadRemoteMem((LPVOID)ModLdrInfo.bstrFilePath, pPathBuf, dwPathSize))
            return FALSE;
        
        bstrFileName = (LPCWSTR)pPathBuf;
        pModInfo->strModPath = (LPSTR)bstrFileName;


        //������
        pModLst.AddTail(pModInfo);

        //�ͷ���Դ
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
    //Dosͷ
    m_pDosHead = (PIMAGE_DOS_HEADER)lpData;
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

BOOL CPE::GetRemoteFSAddr(DWORD& dwOutAddr)    //��ȡԶ��FS��ַ
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

//��ȡģ����Ϣ��
BOOL CPE::GetLdrDataTable(DWORD& dwOutAddr)
{
    //��ȡFS��ַ
    DWORD dwFs = 0;
    if(!GetRemoteFSAddr(dwFs))
    {
        CDeBug::OutErrMsg(TEXT("GetLdrDataTable: ��ȡFS�ε�ַʧ�ܣ�"));
        return FALSE;
    }

    DWORD dwAddr = dwFs + 0x18; // NT_TIB

    DWORD dwReadLen = 0;
    try
    {
        //��ȡNT_TIB
        if(ReadProcessMemory(m_pProcess, (LPVOID)dwAddr, &dwAddr,
            sizeof(DWORD),&dwReadLen) == NULL)
        {
            return FALSE;
        }
        
        //��ȡpeb
        if(ReadProcessMemory(m_pProcess, (LPVOID)(dwAddr + 0x30), &dwAddr,
            sizeof(DWORD),&dwReadLen) == NULL)
        {
            return FALSE;
        }
        //��ȡLdr PEB_LDR_DATA ���� 
        if(ReadProcessMemory(m_pProcess, (LPVOID)(dwAddr + 0xC), &dwAddr,
            sizeof(DWORD),&dwReadLen) == NULL)
        {
            return FALSE;
        }
        //��ȡ _LDR_DATA_TABLE_ENTRY dwAddr + 0xC
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
    //�ر��ڴ�ӳ��
    if(m_lpData != NULL)
        UnmapViewOfFile(m_lpData);
    
    //�رվ��
    if(m_hFileMap != INVALID_HANDLE_VALUE)
        CloseHandle(m_hFileMap);

    //�رվ��
    if(hFile != NULL && hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
}