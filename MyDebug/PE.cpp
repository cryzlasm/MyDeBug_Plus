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

        //����������
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
    
    //����ƫ��
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
    
    //�����������ж�����
    for(DWORD i = 0; i < dwSectionCount; i++)
    {
        dwLowAddr = pTmpSection->PointerToRawData;     //����ʼλ��
        dwHiAddr = pTmpSection->PointerToRawData + pTmpSection->SizeOfRawData;    //�ڽ���λ��
        if(dwFa >= dwLowAddr && dwFa < dwHiAddr)
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
        dwOutRva = 0;
        return FALSE;
    }
    
    //����ƫ��
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
        
        tcout << TEXT("������") << (LPCTSTR)pMod->strModName << endl;
        
        tcout << TEXT("����������") << pMod->dwFunCount << endl;

        POSITION subPos = pMod->FunLst.GetHeadPosition();
        while(subPos)
        {
            PIMPORT_FUN_INFO pFun = pMod->FunLst.GetNext(subPos);
            //_tprintf(TEXT("\tRVA: %08X"), pFun->dwOrdinal);
            if(pFun->bIsName)
            {
                tcout << TEXT("\t��������") << (LPCTSTR)pFun->strFunName << endl;
            }
            else
            {
                CString strNum = TEXT("");
                strNum.Format(TEXT("%04X"), pFun->dwOrdinal);
                tcout << TEXT("\t��ţ�") << (LPCTSTR)strNum << endl;
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
                tcout << TEXT("\t��������") << (LPCTSTR)pFun->strFunName << endl;
            }
            else
            {
                CString strNum = TEXT("");
                strNum.Format(TEXT("%04X"), pFun->dwOrdinal);
                tcout << TEXT("\t��ţ�") << (LPCTSTR)strNum << endl;
            }
        }
        
        tcout << TEXT("������") << (LPCTSTR)pMod->strModName << endl;
        
        tcout << TEXT("����������") << pMod->dwFunCount << endl;
        
        system("pause");
    }
#endif

#ifdef THREE
    POSITION pos = m_ImportLst.GetHeadPosition();
    while(pos)
    {
        PIMPORT_MOD_INFO pMod = m_ImportLst.GetNext(pos);
        
        tcout << TEXT("������") << (LPCTSTR)pMod->strModName << endl;
        
        tcout << TEXT("�޸��ɹ���") <<  pMod->dwFixSucceed << endl;

        tcout << TEXT("�޸�ʧ�ܣ�") <<  pMod->dwFixFailure << endl;
        
        tcout << TEXT("����������") << pMod->dwFunCount << endl;
        
        //system("pause");
    }
#endif

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


BOOL CPE::FixImport(CList<PMOD_INFO, PMOD_INFO&>& pModLst, DWORD lpImportAddr, DWORD dwSize)
{
    if(m_pdwImport != NULL)
    {
        delete m_pdwImport;
        m_pdwImport = NULL;
    }

    //ȥ��β�� % DWORD ����
    dwSize = (dwSize / sizeof(DWORD)) * sizeof(DWORD);
    
    //���뻺����
    m_pdwImport = (PDWORD)new CHAR[dwSize];
    if(m_pdwImport == NULL)
    {
        CDeBug::OutErrMsg(TEXT("FixImport������ռ�ʧ�ܣ�"));
        return FALSE;
    }
    ZeroMemory((PCHAR)m_pdwImport, dwSize);

    //��Զ���ڴ�
    if(!ReadRemoteMem((LPVOID)lpImportAddr, (PCHAR)m_pdwImport, dwSize))
    {
        CDeBug::OutErrMsg(TEXT("FixImport����ȡԶ���ڴ�ʧ�ܣ�"));
        return FALSE;
    }

    //����IAT
    PDWORD pTmpImport = m_pdwImport;
    PCHAR  pEnd = (PCHAR)m_pdwImport + dwSize;
    
    if(!ResolveIAT(pModLst, pTmpImport, (PDWORD)pEnd))
    {
        CDeBug::OutErrMsg(TEXT("FixImport��ת��IATʧ�ܣ�"));
        return FALSE;
    }

    tcout << TEXT("ѡ���ļ�\r\n�ļ�����") << endl;
    CFileDialog FileDlg(TRUE);
    
    //��ȡ�ļ���
    CString strFilePath = TEXT("");
    if(FileDlg.DoModal() == IDOK)
    {
        strFilePath = FileDlg.GetPathName();
    }
    
    tcout << (LPCTSTR)strFilePath << endl;

    CString strNewFile = strFilePath + TEXT(".bak");

    //�����ļ�
    if(!CopyFile(strFilePath, strNewFile, FALSE))
    {
        CDeBug::OutErrMsg(TEXT("FixImport�������ļ�ʧ��"));
        return FALSE;
    }

    //����ģ�飬ת��IAT �� VA Ϊ RVA
    POSITION pos = pModLst.GetHeadPosition();
    while(pos)
    {
        PMOD_INFO pMod = pModLst.GetNext(pos);

        //�ж�VA ����
        if(lpImportAddr >= pMod->dwModBaseAddr &&
            lpImportAddr < (pMod->dwModBaseAddr + pMod->dwModSize))
        {
            lpImportAddr -= pMod->dwModBaseAddr;
            break;
        }
    }

    //ת��IAT
    if(!ConvertIAT(strFilePath, lpImportAddr))
    {
        //ɾ������
        //DeleteFile(strNewFile);
        CDeBug::OutErrMsg(TEXT("FixImport��ת��IAT���м��ʧ�ܣ�"));
        return FALSE;
    }

    ShowLst(pModLst);
    return TRUE;
}

BOOL CPE::ConvertIAT(CString& strFile, DWORD dwIATBase)
{
    //��ȡ�ļ���С
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

    //���ļ� ���ļ�
    HANDLE hFile = CreateFile(strFile,              //·��
        GENERIC_READ | GENERIC_WRITE,                  //����ʽ��
        FILE_SHARE_READ | FILE_SHARE_WRITE,               //�����
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
    
    //Ԥ���ռ�����д�����
    dwFileSize += ONE_PAGE * 3;

    //�����ļ�ӳ��
    m_hFileMap = CreateFileMapping(hFile,         //�ļ����
        NULL,            //��ȫ����һ��ΪNULL
        PAGE_READWRITE,   //�������ԣ�ֻ��
        0,               //��Ҫ�ڴ��С�ʹ��ļ���Сһ��
        dwFileSize,               //
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
        FILE_MAP_READ | FILE_MAP_WRITE,         //ֻ��ҳ
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
    
    //ת����IAT
    if(!HandleIAT(m_lpData, dwIATBase))
    {
        tcout << TEXT("��������BUG ����ϵ����Ա��") << endl;
        CDeBug::OutErrMsg(TEXT("GetFunLst: ��ȡ�ļ���Ϣ"));
        SafeRet(hFile);
        return FALSE;
    }
    
    //ShowLst(pModLst);
    SafeRet(hFile);
    return TRUE;
}

BOOL CPE::HandleIAT(LPVOID lpBuf, DWORD dwIATBase)
{
    DWORD dwRva = 0;        //RVA ������
    DWORD dwFA = 0;         //FA  ������
    DWORD dwNumOfIAT = 0;   //IAT �±�
    //�ҵ����һ���ڱ�
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
            //�޸�SizeOfImage
            DWORD dwSizeOfImage = m_pOption32->SizeOfImage;
            dwSizeOfImage += ONE_PAGE * 2;
            m_pOption32->SizeOfImage = dwSizeOfImage;

            //��ȡ�ڴ��ļ�������С
            DWORD dwVirtualSize = pTmpSection->Misc.VirtualSize;
            dwVirtualSize = MaxMemCurNum(dwVirtualSize, m_pOption32->SectionAlignment);
            dwVirtualSize += ONE_PAGE * 2;
            pTmpSection->Misc.VirtualSize = dwVirtualSize;


            //��ȡ�ļ�������С
            DWORD dwSectionSize = pTmpSection->SizeOfRawData;
            DWORD dwSize = 0;   //�����Ĵ�С
            
            //���뵽�ļ�β
            dwSize = MaxMemCurNum(dwSectionSize, m_pOption32->FileAlignment);
            
            //�ļ�ĩβ��λ�ã��¿���ҳ����ʼλ��
            DWORD dwFileBase = dwSectionSize + pTmpSection->PointerToRawData;
            
            
            //����������ҳ��Ϊ��������Ϣ
            //��������ʼ
            PCHAR pImport = (PCHAR) lpBuf + dwSize + pTmpSection->PointerToRawData;
            dwSize += ONE_PAGE;

            ZeroMemory(pImport, ONE_PAGE);

            //�ַ�����ʼ
            PCHAR pStr = (PCHAR) lpBuf + dwSize + pTmpSection->PointerToRawData;
            dwSize += ONE_PAGE;
            ZeroMemory(pImport, ONE_PAGE);

            //IAT��ʼ
            dwRva = dwIATBase;  //RVA
            if(!ConvertRvaToFa(m_pSection, m_dwSectionCount, dwRva, dwFA))
            {
                CDeBug::OutErrMsg(TEXT("HandleIAT: ת��FA��RVAʧ��"));
                return FALSE;
            }
            PCHAR pIAT = (PCHAR)lpBuf + dwFA;
            ZeroMemory(pImport, ONE_PAGE);
            //dwSize += ONE_PAGE;

            //��д�ļ���С
            pTmpSection->SizeOfRawData = dwSize;

//             //������
//             SECTION_NATURE& SecNature = *(PSECTION_NATURE)&pTmpSection->Characteristics;
//             SecNature.m_dwRead = TRUE;
//             SecNature.m_dwWrite = TRUE;
//             SecNature.m_dwExecute = TRUE;


            //1��pStrλ����дģ������������
            //2��pIATλ����д��������RVA
            //3��pImport λ�� ��д �����
            
            DWORD dwImportSize = 0;         //������С
            DWORD dwBase = (DWORD)lpBuf;    //�ļ���ַ
            PDWORD pdwTmpIAT = (PDWORD)pIAT;  //IATָ��
            PCHAR  pszTmpStr = (PCHAR)pStr;   //Strָ��
            PIMAGE_IMPORT_DESCRIPTOR pTmpImport = (PIMAGE_IMPORT_DESCRIPTOR)pImport;    //�����ָ��
            
            DWORD dwIAT = 0;
            DWORD dwStrCount = 0;

            //����ģ����Ϣ
            POSITION pos = m_ImportLst.GetHeadPosition();
            while(pos)
            {
                PIMPORT_MOD_INFO pMod = m_ImportLst.GetNext(pos);

                //��ģ�鵼�뺯��С��1������Ϊ����д�����
                if(pMod->dwFunCount < 1)
                    continue;

                //���ÿ���RVA
                dwFA = (DWORD)pszTmpStr - dwBase;
                if(!ConvertFaToRva(m_pSection, m_dwSectionCount, dwFA, dwRva))
                {
                    CDeBug::OutErrMsg(TEXT("HandleIAT: ת��FA��RVAʧ��"));
                    return FALSE;
                }
                pTmpImport->Name = dwRva;       //����
                
                //����IAT��ϵ
                dwFA = (DWORD)pdwTmpIAT - dwBase;
                if(!ConvertFaToRva(m_pSection, m_dwSectionCount, dwFA, dwRva))
                {
                    CDeBug::OutErrMsg(TEXT("HandleIAT: ת��FA��RVAʧ��"));
                    return FALSE;
                }
                pTmpImport->FirstThunk = dwRva;     //IAT

                //��������
                dwStrCount = pMod->strModName.GetLength() + 1;
                memcpy(pszTmpStr, (LPCTSTR)pMod->strModName, dwStrCount);
                pszTmpStr += dwStrCount;

                //ģ2���� ��0
                if(dwStrCount % 2 != 0)
                {
                    pszTmpStr++;
                }
                
                //������ģ���е���ĺ���
                POSITION subPos = pMod->FunLst.GetHeadPosition();
                while(subPos)
                {
                    PIMPORT_FUN_INFO pFun = pMod->FunLst.GetNext(subPos);
                    
                    SYS_IMPORT_FUN_INFO SysFunInfo = {0};
                    
                    if(dwNumOfIAT == pFun->dwNumOfIAT)
                    {
                        //Ϊ���ƣ���λΪ0
                        if(pFun->bIsName)
                        {
                            //����IAT����
                            SysFunInfo.m_dwState = 0;
                            dwFA = (DWORD)pszTmpStr - dwBase;
                            if(!ConvertFaToRva(m_pSection, m_dwSectionCount, dwFA, dwRva))
                            {
                                CDeBug::OutErrMsg(TEXT("HandleIAT: ת��FA��RVAʧ��"));
                                return FALSE;
                            }
                            SysFunInfo.m_dwNum = dwRva;     //IAT INFO
                            
                            //����������
                            dwStrCount = pFun->strFunName.GetLength() + 1;
                            WORD wHint = dwStrCount;
                            
                            //Hint
                            memcpy(pszTmpStr, (PCHAR)&wHint, sizeof(WORD));
                            pszTmpStr += sizeof(WORD);
                            
                            memcpy(pszTmpStr, (LPCTSTR)pFun->strFunName, dwStrCount);
                            pszTmpStr += dwStrCount;
                            
                            //ģ2���� ��0
                            if(dwStrCount % 2 != 0)
                            {
                                pszTmpStr++;
                            }
                            
                            *pdwTmpIAT = *(PDWORD)&SysFunInfo;
                        }
                        //Ϊ��ţ���λΪ1
                        else
                        {
                            //�������
                            SysFunInfo.m_dwState = 1;
                            SysFunInfo.m_dwState = pFun->dwOrdinal;
                            
                            *pdwTmpIAT = *(PDWORD)&SysFunInfo;
                        }
                    }//End if(dwNumOfIAT == pFun->dwNumOfIAT)
                    
                    //��IATָ��
                    pdwTmpIAT++;
                    dwNumOfIAT++;
                }//End while(subPos)

                //IATβ����NULL
                *pdwTmpIAT = NULL;

                //��IATָ��
                pdwTmpIAT++;
                dwNumOfIAT++;

                //�Ƶ����ṹ��
                pTmpImport++;

                //������С�Լ�
                dwImportSize += sizeof(IMAGE_IMPORT_DESCRIPTOR);
            }

            //�޸�����Ŀ¼
            dwFA = (DWORD)pImport - dwBase;
            if(!ConvertFaToRva(m_pSection, m_dwSectionCount, dwFA, dwRva))
            {
                CDeBug::OutErrMsg(TEXT("HandleIAT: ת��FA��RVAʧ��"));
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
    DWORD dwTime = 0;   //����

    //������ʼλ��
    PDWORD pTmpStart = pStart;

    //����λ�����
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
            CDeBug::OutErrMsg(TEXT("ResolveIAT�� ���뺯���ռ�ʧ��"));
            return FALSE;
        }
        PIMPORT_FUN_INFO pDstFun = NULL;
        
        //��ʼ��������
        DWORD dwImportFixSucceed = 0; //�ɹ�����
        DWORD dwImportAllCount   = 0; //�ܸ���

        //����ģ��
        POSITION pos = pModLst.GetHeadPosition();
        while(pos)
        {
            BOOL bIsFunFind = FALSE;    // �Ƿ��ҵ�����
            BOOL bIsModFind = FALSE;    // �Ƿ��ҵ�ģ��

            //�ж�ģ������
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
                        CDeBug::OutErrMsg(TEXT("ResolveIAT�� ���뺯���ռ�ʧ��"));
                        return FALSE;
                    }

                    //����������
                    POSITION subPos = pSrcMod->FunLst.GetHeadPosition();
                    while(subPos)
                    {
                        PMOD_EXPORT_FUN pSrcFun = pSrcMod->FunLst.GetNext(subPos);

                        //�жϺ�������
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
                            
                            //�ҵ�һ����IAT���һ��
                            *pStart = 0;

                            pDstMod->FunLst.AddTail(pDstFun);
                            break;
                        }
                    }// End while(subPos)
                
                    // ��ָ��
                    pStart++;
                    dwNum++;
                    dwImportAllCount++;
                
                    //��ǰģ�����
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

            //�ҵ�
            if(bIsModFind)
            {
                pDstMod->dwModBase = pSrcMod->dwModBaseAddr;    //ģ���ַ
                pDstMod->dwModSize = pSrcMod->dwModSize;        //ģ���С
                pDstMod->strModName = pSrcMod->strModName;      //ģ����
                break;
            }

        }//End while(pos)

        //�����ȡ���������������û��ȡ���ͷ���Դ
        if(pDstMod->dwFunCount < 1)
        {
            delete pDstMod;
            pDstMod = NULL;
        }
        else
        {
            m_ImportLst.AddTail(pDstMod);
        }

        //��ָ��
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