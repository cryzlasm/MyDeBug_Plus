命令                格式                 作用   

 Q               Q [地址]            退出调试器
 D               D [地址]            查看内存
 E               E [地址]            修改内存
 U               U [地址]            显示内存
 R               R [寄存器]          查看修改寄存器
 T               T                   单步不如
 P               p                   单步步过
 G               G [地址]            执行地址
 GR              GR                  执行到返回
 GU              GU                  执行到用户
 SBC             SBC                 设置断点指令
 LM              LM                  查看模块
 LT              LT                  查看线程表
 LV              LV                  查看分页表
 LN              LN                  查看当前模块名称
 LAN             LAN                 查看所有名称
 ES              ES                  导出脚本
 LS              LS                  加载脚本
 LSEH            LSEH                查看SEH链
 LSTACK          LSTACK              查看函数调用栈
 RT              RT                  流程记录
 BP              BP [地址]            普通断点
 BL              BL                  查看普通断点
 BC              BC [断点ID]          删除普通断点
 BPM             BPM [地址] [长度] [类型r.w]        设置内存断点
 BLM             BLM                               查看内存断点
 BCM             BCM [断点ID]                      删除内存断点
 BPH             BPH [地址] [长度] [类型r.w.e]      设置硬件断点
 BLH             BLH                               查看硬件断点
 BCH             BCH [断点ID]                      删除硬件断点
 BPQ             BPQ [地址] [长度] [类型r.w.e]      设置条件断点
 BLQ             BLQ                               查看条件断点
 BCQ             BCQ[断点ID]   