// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__D4F52F5F_ED8A_4DB8_86C7_E5E0A88D0924__INCLUDED_)
#define AFX_MAINFRM_H__D4F52F5F_ED8A_4DB8_86C7_E5E0A88D0924__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "winioctl.h"
#include "TDriver.h"
#include "..\Driver\FwHookDrv.h"

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;

	BOOL started;
	TDriver filterDriver;

// Generated message map functions
protected:
	BOOL AddFilterToFw(unsigned long srcIp, 
					   unsigned long srcMask,
					   unsigned short srcPort, 
					   unsigned long dstIp,
					   unsigned long dstMask,
					   unsigned short dstPort,
					   unsigned int protocol,
					   int action);

	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnButtonstart();
	afx_msg void OnButtonadd();
	afx_msg void OnButtondel();
	afx_msg void OnButtondesinstall();
	afx_msg void OnButtoninstall();
	afx_msg void OnButtonstop();
	afx_msg void OnUpdateButtonstart(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonstop(CCmdUI* pCmdUI);
	afx_msg void OnMenuAddrule();
	afx_msg void OnMenuDelrule();
	afx_msg void OnMenuInstallrules();
	afx_msg void OnMenuUninstallrules();
	afx_msg void OnMenustart();
	afx_msg void OnUpdateMenustart(CCmdUI* pCmdUI);
	afx_msg void OnMenustop();
	afx_msg void OnUpdateMenustop(CCmdUI* pCmdUI);
	afx_msg void OnAppExit();
	afx_msg void OnLoadRules();
	afx_msg void OnSaveRules();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__D4F52F5F_ED8A_4DB8_86C7_E5E0A88D0924__INCLUDED_)
