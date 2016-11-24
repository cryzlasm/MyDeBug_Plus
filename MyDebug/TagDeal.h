#if !defined(TAG_DEAL_H__CCFCD8C7_A313_4B5F_9343_626594F87C8E__INCLUDED_)
#define TAG_DEAL_H__CCFCD8C7_A313_4B5F_9343_626594F87C8E__INCLUDED_


#define TF  0x100           //TF标志位
#define NORMAL_CC     0xcc  //一般断点

//硬件断点编号
#define    DR0    0
#define    DR1    1
#define    DR2    2
#define    DR3    3

//硬件断点属性
#define    HBP_INSTRUCTION_EXECUT    0      //00  执行
#define    HBP_DATAS_WRITES          1      //01  写
#define    HBP_DATAS_READS_WRITES    3      //11  读写  
#define    HBP_SET                   1      //硬件断电已设置
#define    HBP_UNSET                 0      //硬件断点未设置
#define    HBP_HIT                   1      //硬件断点命中

//转换进制
#define    CONVERT_HEX      16

//模块列表
typedef struct _tagModLst
{
    DWORD dwBaseAddr;       //模块基址
    //DWORD dwSize;           //模块大小
    //DWORD dwEntryPoint;     //模块入口点
    //LPTSTR strName;      //模块名称
    CString strPath;      //模块路径
}MODLST, *PMODLST;

typedef enum _enumBreakPoint
{
    BP_SYS,             //系统断点，用于断在入口点
    BP_ONCE,            //一次性断点
    BP_NORMAL,          //一般断点
    BP_HARDWARE,        //硬件断点
    BP_MEM,             //内存断点
    BP_HARD_READ,       //硬件读
    BP_HARD_WRITE,      //硬件写
    BP_HARD_EXEC        //硬件执行
}BPSTATE, *PBPSTATE;

typedef enum _enumCmd
{
    CMD_INVALID,
    CMD_SHOWONCE,            //显示一条信息
    CMD_SHOWFIVE,            //显示五条信息
    CMD_SHOWALLDBG,          //显示当前所有信息
    CMD_STEP,                //单步步入         t     
    CMD_STEPGO,              //单步步过         p     
    CMD_RUN,                 //运行             g     
    CMD_TRACE,               //自动跟踪记录     trace
    CMD_DISPLAY_ASMCODE,     //反汇编           u
    CMD_DISPLAY_DATA,        //显示内存数据     d 
    CMD_REGISTER,            //寄存器           r
    CMD_EDIT_DATA,           //修改内存数据     e
    CMD_BREAK_POINT,         //一般断点         bp
    CMD_BP_LIST,             //一般断点列表     bpl
    CMD_CLEAR_NORMAL,        //删除一般断点     bpc
    CMD_BP_HARD,             //硬件断点         bh
    CMD_BP_HARD_LIST,        //硬件断点列表     bhl
    CMD_CLEAR_BP_HARD,       //删除硬件断点     bhc
    CMD_BP_MEMORY,           //内存断点         bm
    CMD_BP_MEMORY_LIST,      //内存断点列表     bml
    CMD_BP_PAGE_LIST,        //分页断点列表     bmpl
    CMD_CLEAR_BP_MEMORY,     //删除内存断点     bmc
    CMD_LOAD_SCRIPT,         //导入脚本         ls
    CMD_EXPORT_SCRIPT,       //导出脚本         es
    CMD_QUIT,                //退出程序         q
    CMD_MODULE_LIST,         //查看模块         ML
    CMD_MEM_INFO_LIST,       //内存信息列表     mil
    CMD_DUMP,                //内存Dump         dump
    CMD_FIX_IAT              //修复IAT          fixiat
}CMDSTATE, *PCMDSTATE;


//断点信息
typedef struct _tagBreakPoint
{
    BPSTATE     bpState;        //断点状态
    LPVOID      lpAddr;         //断点地址
    
    //一般断点字段
    DWORD       dwOldOrder;     //原指令
    DWORD       dwCurOrder;     //现在的指令

    BOOL        bIsSingleStep;  //是否设置单步

    //硬件断点字段
    BPSTATE     hbpStatus;      //断点属性  R W E  读 写 执行

    //硬件断点与内存断点公用字段
    DWORD       dwLen;          //断点长度

    //内存断点字段
    DWORD       dwOldProtect;   //断点原有保护属性

}MYBREAK_POINT, *PMYBREAK_POINT;

//转换CMD信息时所需要的信息
typedef struct _tagCmdInfo
{
    BOOL            bIsBreakInputLoop;      //是否结束交互
    DWORD           dwPreAddr;              //上一个Addr的位置
    CMDSTATE        dwState;                //CMD命令码
    CString         strCMD;                 //CMD命令操作数
}CMD_INFO, *PCMD_INFO;

//标志寄存器
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

//导入表-函数信息表
typedef struct _tagSysImportFunInfo
{
    DWORD m_dwNum:31;     //序号，或者RVA
    DWORD m_dwState:1;    //是否为序号 1为序号 0为RVA
}SYS_IMPORT_FUN_INFO, *PSYS_IMPORT_FUN_INFO;

//节表属性
typedef struct _tag_SECTION_NATURE
{
    DWORD m_dwUnUse:28;     //无用
    DWORD m_dwShare:1;      //WRES
    DWORD m_dwExecute:1;    //WRES
    DWORD m_dwRead:1;       //WRES
    DWORD m_dwWrite:1;      //WRES
}SECTION_NATURE, *PSECTION_NATURE;

//导入表函数信息
typedef struct _tag_IMPORT_FUN_INFO
{
    DWORD dwNumOfIAT;       //在IAT中的序号
    BOOL bIsName;           //是否为地址
    DWORD dwFunAddrRVA;     //地址RVA
    DWORD dwOrdinal;        //序号
    CString strFunName;     //函数名
}IMPORT_FUN_INFO, *PIMPORT_FUN_INFO;

//导入表模块信息
typedef struct _tag_IMPORT_MOD_INFO
{
    DWORD dwFixSucceed;     //成功个数
    DWORD dwFixFailure;     //失败个数
    DWORD dwModBase;        //模块基址
    DWORD dwModSize;        //模块大小
    DWORD dwFunCount;       //函数个数
    CString strModName;     //模块名
    CList<PIMPORT_FUN_INFO, PIMPORT_FUN_INFO> FunLst;    //函数列表
}IMPORT_MOD_INFO, *PIMPORT_MOD_INFO;

//函数信息表
typedef struct _tag_MODULE_EXPORT_FUN_INFO
{
    DWORD dwFunAddr;    //函数地址
    BOOL bIsName;       //是否为名称
    DWORD dwOrdinal;    //序号
    CString strFunName; //名称
}MOD_EXPORT_FUN, *PMOD_EXPORT_FUN;

//模块列表
typedef struct _tag_MODULE_INFO
{
    DWORD dwModBaseAddr;    // 模块基址
    DWORD dwModSize;        // 模块大小 
    DWORD dwlpEntryPoint;   // 模块入口
    DWORD dwFunCount;       // 模块导出函数个数
    CString strModName;     // 模块名称
    CString strModPath;     // 模块路径
    CList<PMOD_EXPORT_FUN, PMOD_EXPORT_FUN&> FunLst;    //函数列表
}MOD_INFO, *PMOD_INFO;

// typedef struct _tag_IAT_INFO
// {
//     DWORD dwIATAddr;    //IAT地址
//     DWORD dwIATCount;   //导入函数个数
//     BOOL  bIsName;      //是否为名称
//     DWORD dwOrdinal;    //序号
//     CString strDllName; //名称
// 
// }IAT_INFO, *PIAT_INFO;

//InLoadOrderModuleList
typedef struct _tag_MODULE_LDR_INFO
{
    LIST_ENTRY             pForwardLst;            //Offset:: 00,  下一个表
    LIST_ENTRY             pBeforeLst;             //Offset:: 08,  上一个表
    LIST_ENTRY             pUnKnowLst;             //Offset:: 10,  未知表
    
    LPVOID  lpMouduleBase;                          //Offset:: 18,  模块基址
    
    LPVOID  lpMouduleEntryPoint;                    //Offset:: 1C,  模块入口点
    
    DWORD   dwMouduleSize;                          //Offset:: 20,  模块大小
    
    DWORD   dwPathStrLen:  16;                      //Offset:: 24,  路径字符串的长度
    DWORD   dwPathStrSize: 16;                      //Offset:: 24,  路径字符串实际占用空间大小
    LPCWSTR bstrFilePath;                           //Offset:: 28,  路径字符串位置
    
    DWORD   dwNameStrLen:  16;                      //Offset:: 2C,  名称字符串的长度
    DWORD   dwNameStrSize: 16;                      //Offset:: 2C,  名称字符串实际占用空间大小
    LPCWSTR bstrFileName;                           //Offset:: 30,  名称字符串位置
    
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
//     LIST_ENTRY              pForwardLst;                    //Offset:: 00,  下一个表
//     LIST_ENTRY              pBeforeLst;                     //Offset:: 08,  上一个表
//     LIST_ENTRY              pUnKnowLst;                     //Offset:: 10,  未知表
//     LPVOID                  lpDllBase;                      //Offset:: 18,  模块基址
//     LPVOID                  lpEntryPoint;                   //Offset:: 1C,  模块入口点
//     DWORD                   dwSizeOfImage;                  //Offset:: 20,  模块大小
//     DWORD                   dwPathStrLen:  16;              //Offset:: 24,  路径字符串的长度
//     DWORD                   dwPathStrSize: 16;              //Offset:: 24,  路径字符串实际占用空间大小
//     BSTR                    bstrFilePath;                   //Offset:: 28,  路径字符串地址
//     DWORD                   dwNameStrLen:  16;              //Offset:: 2C,  名称字符串的长度
//     DWORD                   dwNameStrSize: 16;              //Offset:: 2C,  名称字符串实际占用空间大小
//     BSTR                    bstrFileName;                   //Offset:: 30,  名称字符串地址
//     DWORD                   dwFlags;                        //Offset:: 34,  
//     DWORD                   dwLoadCount:   16;              //Offset:: 38,  引用计数？？
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
//     LPVOID                  ptagEntryPointActivationContext;//Offset:: 48, _ACTIVATION_CONTEXT 类型的指针
//     LPVOID                  lpPatchInformation;             //Offset:: 4C,
//     _LIST_ENTRY             lstForwarderLinks;              //Offset:: 50
//     _LIST_ENTRY             lstServiceTagLinks;             //Offset:: 58
//     _LIST_ENTRY             lstStaticLinks;                 //Offset:: 60
//     LPVOID                  lpContextInformation;           //Offset:: 68
//     DWORD                   dwOriginalBase;                 //Offset:: 6C
//     _LARGE_INTEGER          tagLoadTime;                    //Offset:: 70
// }MODULE_LDR_INFO_EX, *PMODULE_LDR_INFO_EX;  //Offset:: 78  size 0x78, 120

#endif