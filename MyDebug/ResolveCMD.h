// ResolveCMD.h: interface for the CResolveCMD class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RESOLVECMD_H__8BCAFF6D_7D48_480E_964B_1B726B366F70__INCLUDED_)
#define AFX_RESOLVECMD_H__8BCAFF6D_7D48_480E_964B_1B726B366F70__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "TagDeal.h"

class CResolveCMD  
{
public:
	CResolveCMD();
	virtual ~CResolveCMD();
    BOOL Resolve(CMD_INFO& CmdInfo);
};

#endif // !defined(AFX_RESOLVECMD_H__8BCAFF6D_7D48_480E_964B_1B726B366F70__INCLUDED_)
