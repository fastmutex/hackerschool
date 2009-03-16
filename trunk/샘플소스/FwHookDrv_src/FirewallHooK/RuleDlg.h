#if !defined(AFX_RULEDLG_H__C9161D68_7C56_47A7_8D2C_3A700665F7FE__INCLUDED_)
#define AFX_RULEDLG_H__C9161D68_7C56_47A7_8D2C_3A700665F7FE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RuleDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRuleDlg dialog

class CRuleDlg : public CDialog
{
// Construction
public:
	CRuleDlg(CWnd* pParent = NULL);   // standard constructor
	
	unsigned long srcIp;
	unsigned long srcMask;
	unsigned short srcPort;

	unsigned long dstIp;
	unsigned long dstMask;
	unsigned short dstPort;

	unsigned int protocol;

	int cAction;


// Dialog Data
	//{{AFX_DATA(CRuleDlg)
	enum { IDD = IDD_ADDRULEDLG };
	CString	m_ipsource;
	UINT	m_portsource;
	CString	m_ipdestination;
	UINT	m_portDestination;
	CString	m_action;
	CString	m_protocol;
	CString	m_srcMask;
	CString	m_dstMask;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRuleDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRuleDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RULEDLG_H__C9161D68_7C56_47A7_8D2C_3A700665F7FE__INCLUDED_)
