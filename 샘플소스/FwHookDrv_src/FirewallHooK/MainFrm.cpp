// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "FirewallApp.h"

#include "MainFrm.h"
#include "RuleDlg.h"
#include "FirewallAppDoc.h"
#include "FirewallAppView.h"
#include "SockUtil.h"
#include "rules.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_BUTTONSTART, OnButtonstart)
	ON_COMMAND(ID_BUTTONADD, OnButtonadd)
	ON_COMMAND(ID_BUTTONDEL, OnButtondel)
	ON_COMMAND(ID_BUTTONDESINSTALL, OnButtondesinstall)
	ON_COMMAND(ID_BUTTONINSTALL, OnButtoninstall)
	ON_COMMAND(ID_BUTTONSTOP, OnButtonstop)
	ON_UPDATE_COMMAND_UI(ID_BUTTONSTART, OnUpdateButtonstart)
	ON_UPDATE_COMMAND_UI(ID_BUTTONSTOP, OnUpdateButtonstop)
	ON_COMMAND(IDMENU_ADDRULE, OnMenuAddrule)
	ON_COMMAND(IDMENU_DELRULE, OnMenuDelrule)
	ON_COMMAND(IDMENU_INSTALLRULES, OnMenuInstallrules)
	ON_COMMAND(IDMENU_UNINSTALLRULES, OnMenuUninstallrules)
	ON_COMMAND(ID_MENUSTART, OnMenustart)
	ON_UPDATE_COMMAND_UI(ID_MENUSTART, OnUpdateMenustart)
	ON_COMMAND(ID_MENUSTOP, OnMenustop)
	ON_UPDATE_COMMAND_UI(ID_MENUSTOP, OnUpdateMenustop)
	ON_COMMAND(ID_APP_EXIT, OnAppExit)
	ON_COMMAND(IDMENU_LOADRULES, OnLoadRules)
	ON_COMMAND(IDMENU_SAVERULES, OnSaveRules)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	started = FALSE;

	// Load Filter driver
	if(filterDriver.LoadDriver("FwHookDrv", NULL, NULL, TRUE) != DRV_SUCCESS)
	{
		AfxMessageBox("Error loading filter driver.");

		exit(-1);
	}
	
}

CMainFrame::~CMainFrame()
{
	
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
//	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
//	EnableDocking(CBRS_ALIGN_ANY);
//	DockControlBar(&m_wndToolBar);

	this->SetWindowText("Firewall FWHook");



	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.style &= ~ FWS_ADDTOTITLE;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


void CMainFrame::OnButtonstart() 
{
	if(filterDriver.WriteIo(START_IP_HOOK, NULL, 0) != DRV_ERROR_IO)
	{
		started = TRUE;
	}
	
}

void CMainFrame::OnButtonstop() 
{
	if(filterDriver.WriteIo(STOP_IP_HOOK, NULL, 0) != DRV_ERROR_IO)
	{
		started = FALSE;
	}
}

void CMainFrame::OnButtonadd() 
{
	CFirewallAppDoc *doc = (CFirewallAppDoc *)GetActiveDocument();
	CRuleDlg dlg;

	// Check rule limit
	if(doc->nRules < MAX_RULES )
	{
		if(dlg.DoModal() == IDOK)
		{
			// Add rule to rule list
			if(doc->AddRule(dlg.srcIp, dlg.srcMask, dlg.srcPort, dlg.dstIp, dlg.dstMask, dlg.dstPort, dlg.protocol, dlg.cAction) != 0)
				AfxMessageBox("Error adding the rule");

			else
			{
				// Update screen
				CFirewallAppView *view = (CFirewallAppView *)GetActiveView();

				view->UpdateList();
			}

		}
	}

	else
		AfxMessageBox("No more rules can't be added.");
}

void CMainFrame::OnButtondel() 
{
	CFirewallAppView *view = (CFirewallAppView *)GetActiveView();


	// Get rule position
	POSITION pos = view->m_rules.GetFirstSelectedItemPosition();
	if (pos == NULL)
	{
		AfxMessageBox("No rule is selected.");

		return;
	}

	int position;
	position = view->m_rules.GetNextSelectedItem(pos);

	CFirewallAppDoc *doc = (CFirewallAppDoc *)GetActiveDocument();
	doc->DeleteRule(position);


	// Update view.
	view->UpdateList();
	
}


void CMainFrame::OnButtoninstall() 
{
	CFirewallAppDoc *doc = (CFirewallAppDoc *)GetActiveDocument();

	unsigned int i;
	DWORD result;

	// Send rules to the driver
	for(i=0;i<doc->nRules;i++)
	{
		result = AddFilterToFw(doc->rules[i].sourceIp, 
							   doc->rules[i].sourceMask,
							   doc->rules[i].sourcePort,
							   doc->rules[i].destinationIp,
							   doc->rules[i].destinationMask,
							   doc->rules[i].destinationPort,
							   doc->rules[i].protocol, 
							   doc->rules[i].action);


		if (!result) 
			break;
	}
	
}


void CMainFrame::OnButtondesinstall() 
{
	if(filterDriver.WriteIo(CLEAR_FILTER, NULL, 0) == DRV_ERROR_IO)
	{
		AfxMessageBox("Error uninstalling rules.");
	}
	
}


void CMainFrame::OnUpdateButtonstart(CCmdUI* pCmdUI) 
{
	if(started)
		pCmdUI->Enable(FALSE);

	else
		pCmdUI->Enable(TRUE);	
}

void CMainFrame::OnUpdateButtonstop(CCmdUI* pCmdUI) 
{
	if(started)
		pCmdUI->Enable(TRUE);

	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnMenuAddrule() 
{
	OnButtonadd();	
}

void CMainFrame::OnMenuDelrule() 
{
	OnButtondel();	
}

void CMainFrame::OnMenuInstallrules() 
{
	OnButtoninstall();
}

void CMainFrame::OnMenuUninstallrules() 
{
	OnButtondesinstall();
	
}

void CMainFrame::OnMenustart() 
{
	OnButtonstart();
}

void CMainFrame::OnUpdateMenustart(CCmdUI* pCmdUI) 
{
	if(started)
		pCmdUI->Enable(FALSE);

	else
		pCmdUI->Enable(TRUE);	
	
}

void CMainFrame::OnMenustop() 
{
	OnButtonstop();
}

void CMainFrame::OnUpdateMenustop(CCmdUI* pCmdUI) 
{
	if(started)
		pCmdUI->Enable(TRUE);

	else
		pCmdUI->Enable(FALSE);
	
}

void CMainFrame::OnAppExit() 
{

}

BOOL CMainFrame::AddFilterToFw(unsigned long srcIp, 
							   unsigned long srcMask,
							   unsigned short srcPort, 
							   unsigned long dstIp,
							   unsigned long dstMask,
							   unsigned short dstPort,
							   unsigned int protocol,
							   int action)
{
	IPFilter pf;


	pf.protocol = protocol;

	pf.destinationIp = dstIp;			
	pf.sourceIp		 = srcIp;

	pf.destinationMask = dstMask;	
	pf.sourceMask	   = srcMask;		

	pf.destinationPort = htons(dstPort);						
	pf.sourcePort	   = htons(srcPort);				

	pf.drop = action;		
	
	// Send rule to the driver
	DWORD result = filterDriver.WriteIo(ADD_FILTER, &pf, sizeof(pf));

	if (result != DRV_SUCCESS) 
	{
		AfxMessageBox("Error adding a rule.");

		return FALSE;
	}

	else
		return TRUE;
}

void CMainFrame::OnLoadRules() 
{
	CFile file;
	CFileException e;
	DWORD nRead;

	CFirewallAppDoc *doc = (CFirewallAppDoc *)GetActiveDocument();

	CFileDialog dg(TRUE,NULL, NULL, OFN_HIDEREADONLY | OFN_CREATEPROMPT,"Rule Files(*.rul)|*.rul|all(*.*)|*.*||", NULL);
	if(dg.DoModal()==IDCANCEL)
		return;
	
	CString nf=dg.GetPathName();

	if(nf.GetLength() == 0)
	{
		AfxMessageBox("Invalid file.");

		return;
	}

	if( !file.Open(nf, CFile::modeRead, &e ) )
	{
		AfxMessageBox("Error openning the file.");

		return;
	}

	doc->ResetRules();
	RuleInfo rule;
	
    do
    {
 		nRead = file.Read(&rule, sizeof(RuleInfo));
		
		if(nRead == 0)
			break;

		if(doc->AddRule(rule.sourceIp,
				        rule.sourceMask,
						rule.sourcePort,
						rule.destinationIp,
						rule.destinationMask,
						rule.destinationPort,
						rule.protocol,				   
						rule.action)  != 0)
		{

			AfxMessageBox("Error adding the rule");

			break;
		}

    }while (1);


		
	// Update view
	CFirewallAppView *view = (CFirewallAppView *)GetActiveView();

	view->UpdateList();
}

void CMainFrame::OnSaveRules() 
{
	CFirewallAppDoc *doc = (CFirewallAppDoc *)GetActiveDocument();

	if(doc->nRules == 0)
	{
		AfxMessageBox("No rules to save.");

		return;
	}


	CFileDialog dg(FALSE, NULL, NULL, OFN_HIDEREADONLY | OFN_CREATEPROMPT,"Rule Files(*.rul)|*.rul|all(*.*)|*.*||", NULL);
	if(dg.DoModal()==IDCANCEL)
		return;
	
	CString nf=dg.GetPathName();

	if(nf.GetLength() == 0)
	{
		AfxMessageBox("Invalid file.");

		return;
	}

	CFile file;
	CFileException e;
	
	if( !file.Open( nf, CFile::modeCreate | CFile::modeWrite, &e ) )
	{
		AfxMessageBox("Error openning the file.");

		return;
	}


	unsigned int i;

	// Send rules to the driver
	for(i=0;i<doc->nRules;i++)
	{
		file.Write(&doc->rules[i], sizeof(RuleInfo));
	}
	
	file.Close();
}
