#if !defined(TAG_DEAL_H__CCFCD8C7_A313_4B5F_9343_626594F87C8E__INCLUDED_)
#define TAG_DEAL_H__CCFCD8C7_A313_4B5F_9343_626594F87C8E__INCLUDED_


#define TF  0x100           //TF��־λ
#define NORMAL_CC     0xcc  //һ��ϵ�

//Ӳ���ϵ���
#define    DR0    0
#define    DR1    1
#define    DR2    2
#define    DR3    3

//Ӳ���ϵ�����
#define    HBP_INSTRUCTION_EXECUT    0      //00  ִ��
#define    HBP_DATAS_WRITES          1      //01  д
#define    HBP_DATAS_READS_WRITES    3      //11  ��д  
#define    HBP_SET                   1      //Ӳ���ϵ�������
#define    HBP_UNSET                 0      //Ӳ���ϵ�δ����
#define    HBP_HIT                   1      //Ӳ���ϵ�����

//ת������
#define    CONVERT_HEX      16

//ģ���б�
typedef struct _tagModLst
{
    DWORD dwBaseAddr;       //ģ���ַ
    //DWORD dwSize;           //ģ���С
    //DWORD dwEntryPoint;     //ģ����ڵ�
    //LPTSTR strName;      //ģ������
    CString strPath;      //ģ��·��
}MODLST, *PMODLST;

typedef enum _enumBreakPoint
{
    BP_SYS,             //ϵͳ�ϵ㣬���ڶ�����ڵ�
    BP_ONCE,            //һ���Զϵ�
    BP_NORMAL,          //һ��ϵ�
    BP_HARDWARE,        //Ӳ���ϵ�
    BP_MEM,             //�ڴ�ϵ�
    BP_HARD_READ,       //Ӳ����
    BP_HARD_WRITE,      //Ӳ��д
    BP_HARD_EXEC        //Ӳ��ִ��
}BPSTATE, *PBPSTATE;

typedef enum _enumCmd
{
    CMD_INVALID,
    CMD_SHOWONCE,            //��ʾһ����Ϣ
    CMD_SHOWFIVE,            //��ʾ������Ϣ
    CMD_SHOWALLDBG,          //��ʾ��ǰ������Ϣ
    CMD_STEP,                //��������         t     
    CMD_STEPGO,              //��������         p     
    CMD_RUN,                 //����             g     
    CMD_TRACE,               //�Զ����ټ�¼     trace
    CMD_DISPLAY_ASMCODE,     //�����           u
    CMD_DISPLAY_DATA,        //��ʾ�ڴ�����     d 
    CMD_REGISTER,            //�Ĵ���           r
    CMD_EDIT_DATA,           //�޸��ڴ�����     e
    CMD_BREAK_POINT,         //һ��ϵ�         bp
    CMD_BP_LIST,             //һ��ϵ��б�     bpl
    CMD_CLEAR_NORMAL,        //ɾ��һ��ϵ�     bpc
    CMD_BP_HARD,             //Ӳ���ϵ�         bh
    CMD_BP_HARD_LIST,        //Ӳ���ϵ��б�     bhl
    CMD_CLEAR_BP_HARD,       //ɾ��Ӳ���ϵ�     bhc
    CMD_BP_MEMORY,           //�ڴ�ϵ�         bm
    CMD_BP_MEMORY_LIST,      //�ڴ�ϵ��б�     bml
    CMD_BP_PAGE_LIST,        //��ҳ�ϵ��б�     bmpl
    CMD_CLEAR_BP_MEMORY,     //ɾ���ڴ�ϵ�     bmc
    CMD_LOAD_SCRIPT,         //����ű�         ls
    CMD_EXPORT_SCRIPT,       //�����ű�         es
    CMD_QUIT,                //�˳�����         q
    CMD_MODULE_LIST,         //�鿴ģ��         ML
    CMD_MEM_INFO_LIST,       //�ڴ���Ϣ�б�     mil
    CMD_DUMP,                //�ڴ�Dump         dump
    CMD_FIX_IAT              //�޸�IAT          fixiat
}CMDSTATE, *PCMDSTATE;


//�ϵ���Ϣ
typedef struct _tagBreakPoint
{
    BPSTATE     bpState;        //�ϵ�״̬
    LPVOID      lpAddr;         //�ϵ��ַ
    
    //һ��ϵ��ֶ�
    DWORD       dwOldOrder;     //ԭָ��
    DWORD       dwCurOrder;     //���ڵ�ָ��

    BOOL        bIsSingleStep;  //�Ƿ����õ���

    //Ӳ���ϵ��ֶ�
    BPSTATE     hbpStatus;      //�ϵ�����  R W E  �� д ִ��

    //Ӳ���ϵ����ڴ�ϵ㹫���ֶ�
    DWORD       dwLen;          //�ϵ㳤��

    //�ڴ�ϵ��ֶ�
    DWORD       dwOldProtect;   //�ϵ�ԭ�б�������

}MYBREAK_POINT, *PMYBREAK_POINT;

//ת��CMD��Ϣʱ����Ҫ����Ϣ
typedef struct _tagCmdInfo
{
    BOOL            bIsBreakInputLoop;      //�Ƿ��������
    DWORD           dwPreAddr;              //��һ��Addr��λ��
    CMDSTATE        dwState;                //CMD������
    CString         strCMD;                 //CMD���������
}CMD_INFO, *PCMD_INFO;

//��־�Ĵ���
typedef struct _tagEFlags
{
    DWORD dwCF:     1;  //32    0
    DWORD UnUse3:   1;  //31    1
    DWORD dwPF:     1;  //30    2
    DWORD UnUse2:   1;  //29    3
    DWORD dwAF:     1;  //28    4
    DWORD UnUse1:   1;  //27    5
    DWORD dwZF:     1;  //26    6
    DWORD dwSF:     1;  //25    7
    DWORD dwTF:     1;  //24    8
    DWORD dwIF:     1;  //23    9
    DWORD dwDF:     1;  //22    10
    DWORD dwOF:     1;  //21    11
    DWORD UnUse:    20; //20    12

}EFLAGS, *PEFLAGS;

typedef struct _tag_DR7 
{
    unsigned int L0:1;  
    unsigned int G0:1;
    unsigned int L1:1;
    unsigned int G1:1;
    unsigned int L2:1;
    unsigned int G2:1;
    unsigned int L3:1;
    unsigned int G3:1;
    unsigned int LE:1;
    unsigned int GE:1;
    unsigned int RESERVED0:3;
    unsigned int GD:1;
    unsigned int RESERVED1:2;
    unsigned int RW0:2;
    unsigned int LEN0:2;
    unsigned int RW1:2;
    unsigned int LEN1:2;
    unsigned int RW2:2;
    unsigned int LEN2:2;
    unsigned int RW3:2;
    unsigned int LEN3:2;
}MYDR7, *PMYDR7;

typedef struct _tag_DR6 
{
    unsigned int B0:1;  
    unsigned int B1:1;  
    unsigned int B2:1;  
    unsigned int B3:1;   
    unsigned int RESERVED0:9;
    unsigned int BD:1;
    unsigned int BS:1;
    unsigned int BT:1;
    unsigned int RESERVED1:16;
}MYDR6, *PMYDR6;

//�����-������Ϣ��
typedef struct _tagSysImportFunInfo
{
    DWORD m_dwNum:31;     //��ţ�����RVA
    DWORD m_dwState:1;    //�Ƿ�Ϊ��� 1Ϊ��� 0ΪRVA
}SYS_IMPORT_FUN_INFO, *PSYS_IMPORT_FUN_INFO;

//�ڱ�����
typedef struct _tag_SECTION_NATURE
{
    DWORD m_dwUnUse:28;     //����
    DWORD m_dwShare:1;      //WRES
    DWORD m_dwExecute:1;    //WRES
    DWORD m_dwRead:1;       //WRES
    DWORD m_dwWrite:1;      //WRES
}SECTION_NATURE, *PSECTION_NATURE;

//���������Ϣ
typedef struct _tag_IMPORT_FUN_INFO
{
    DWORD dwNumOfIAT;       //��IAT�е����
    BOOL bIsName;           //�Ƿ�Ϊ��ַ
    DWORD dwFunAddrRVA;     //��ַRVA
    DWORD dwOrdinal;        //���
    CString strFunName;     //������
}IMPORT_FUN_INFO, *PIMPORT_FUN_INFO;

//�����ģ����Ϣ
typedef struct _tag_IMPORT_MOD_INFO
{
    DWORD dwFixSucceed;     //�ɹ�����
    DWORD dwFixFailure;     //ʧ�ܸ���
    DWORD dwModBase;        //ģ���ַ
    DWORD dwModSize;        //ģ���С
    DWORD dwFunCount;       //��������
    CString strModName;     //ģ����
    CList<PIMPORT_FUN_INFO, PIMPORT_FUN_INFO> FunLst;    //�����б�
}IMPORT_MOD_INFO, *PIMPORT_MOD_INFO;

//������Ϣ��
typedef struct _tag_MODULE_EXPORT_FUN_INFO
{
    DWORD dwFunAddr;    //������ַ
    BOOL bIsName;       //�Ƿ�Ϊ����
    DWORD dwOrdinal;    //���
    CString strFunName; //����
}MOD_EXPORT_FUN, *PMOD_EXPORT_FUN;

//ģ���б�
typedef struct _tag_MODULE_INFO
{
    DWORD dwModBaseAddr;    // ģ���ַ
    DWORD dwModSize;        // ģ���С 
    DWORD dwlpEntryPoint;   // ģ�����
    DWORD dwFunCount;       // ģ�鵼����������
    CString strModName;     // ģ������
    CString strModPath;     // ģ��·��
    CList<PMOD_EXPORT_FUN, PMOD_EXPORT_FUN&> FunLst;    //�����б�
}MOD_INFO, *PMOD_INFO;

// typedef struct _tag_IAT_INFO
// {
//     DWORD dwIATAddr;    //IAT��ַ
//     DWORD dwIATCount;   //���뺯������
//     BOOL  bIsName;      //�Ƿ�Ϊ����
//     DWORD dwOrdinal;    //���
//     CString strDllName; //����
// 
// }IAT_INFO, *PIAT_INFO;

//InLoadOrderModuleList
typedef struct _tag_MODULE_LDR_INFO
{
    LIST_ENTRY             pForwardLst;            //Offset:: 00,  ��һ����
    LIST_ENTRY             pBeforeLst;             //Offset:: 08,  ��һ����
    LIST_ENTRY             pUnKnowLst;             //Offset:: 10,  δ֪��
    
    LPVOID  lpMouduleBase;                          //Offset:: 18,  ģ���ַ
    
    LPVOID  lpMouduleEntryPoint;                    //Offset:: 1C,  ģ����ڵ�
    
    DWORD   dwMouduleSize;                          //Offset:: 20,  ģ���С
    
    DWORD   dwPathStrLen:  16;                      //Offset:: 24,  ·���ַ����ĳ���
    DWORD   dwPathStrSize: 16;                      //Offset:: 24,  ·���ַ���ʵ��ռ�ÿռ��С
    LPCWSTR bstrFilePath;                           //Offset:: 28,  ·���ַ���λ��
    
    DWORD   dwNameStrLen:  16;                      //Offset:: 2C,  �����ַ����ĳ���
    DWORD   dwNameStrSize: 16;                      //Offset:: 2C,  �����ַ���ʵ��ռ�ÿռ��С
    LPCWSTR bstrFileName;                           //Offset:: 30,  �����ַ���λ��
    
}MODULE_LDR_INFO, *PMODULE_LDR_INFO;    //Offset:: 34, Size 0x34, 52

//InLoadOrderModuleListEx
// typedef struct _tag_MODULE_LDR_INFO_EX
// {
//     // +0x034 Flags            : Uint4B
//     // +0x038 LoadCount        : Uint2B
//     // +0x03a TlsIndex         : Uint2B
//     // +0x03c HashLinks        : _LIST_ENTRY
//     // +0x03c SectionPointer   : Ptr32 Void
//     // +0x040 CheckSum         : Uint4B
//     // +0x044 TimeDateStamp    : Uint4B
//     // +0x044 LoadedImports    : Ptr32 Void
//     
//     LIST_ENTRY              pForwardLst;                    //Offset:: 00,  ��һ����
//     LIST_ENTRY              pBeforeLst;                     //Offset:: 08,  ��һ����
//     LIST_ENTRY              pUnKnowLst;                     //Offset:: 10,  δ֪��
//     LPVOID                  lpDllBase;                      //Offset:: 18,  ģ���ַ
//     LPVOID                  lpEntryPoint;                   //Offset:: 1C,  ģ����ڵ�
//     DWORD                   dwSizeOfImage;                  //Offset:: 20,  ģ���С
//     DWORD                   dwPathStrLen:  16;              //Offset:: 24,  ·���ַ����ĳ���
//     DWORD                   dwPathStrSize: 16;              //Offset:: 24,  ·���ַ���ʵ��ռ�ÿռ��С
//     BSTR                    bstrFilePath;                   //Offset:: 28,  ·���ַ�����ַ
//     DWORD                   dwNameStrLen:  16;              //Offset:: 2C,  �����ַ����ĳ���
//     DWORD                   dwNameStrSize: 16;              //Offset:: 2C,  �����ַ���ʵ��ռ�ÿռ��С
//     BSTR                    bstrFileName;                   //Offset:: 30,  �����ַ�����ַ
//     DWORD                   dwFlags;                        //Offset:: 34,  
//     DWORD                   dwLoadCount:   16;              //Offset:: 38,  ���ü�������
//     DWORD                   dwTlsIndex:    16;              //Offset:: 38,  
//     union
//     {
//         _LIST_ENTRY         lstHashLinks;                   //Offset:: 3C,
//         struct  
//         {
//             DWORD               dwSectionPointer;           //Offset:: 3C,
//             DWORD               dwCheckSum;                 //Offset:: 40,
//         };
//     }UnKnow1;
//     
//     union
//     {
//         DWORD               dwTimeDateStamp;                //Offset:: 44,
//         DWORD               dwLoadedImports;                //Offset:: 44,
//     }UnKnow2;
//     
//     // +0x048 EntryPointActivationContext : Ptr32 _ACTIVATION_CONTEXT
//     // +0x04c PatchInformation : Ptr32 Void
//     // +0x050 ForwarderLinks   : _LIST_ENTRY
//     // +0x058 ServiceTagLinks  : _LIST_ENTRY
//     // +0x060 StaticLinks      : _LIST_ENTRY
//     // +0x068 ContextInformation : Ptr32 Void
//     // +0x06c OriginalBase     : Uint4B
//     // +0x070 LoadTime         : _LARGE_INTEGER
//     LPVOID                  ptagEntryPointActivationContext;//Offset:: 48, _ACTIVATION_CONTEXT ���͵�ָ��
//     LPVOID                  lpPatchInformation;             //Offset:: 4C,
//     _LIST_ENTRY             lstForwarderLinks;              //Offset:: 50
//     _LIST_ENTRY             lstServiceTagLinks;             //Offset:: 58
//     _LIST_ENTRY             lstStaticLinks;                 //Offset:: 60
//     LPVOID                  lpContextInformation;           //Offset:: 68
//     DWORD                   dwOriginalBase;                 //Offset:: 6C
//     _LARGE_INTEGER          tagLoadTime;                    //Offset:: 70
// }MODULE_LDR_INFO_EX, *PMODULE_LDR_INFO_EX;  //Offset:: 78  size 0x78, 120

#endif