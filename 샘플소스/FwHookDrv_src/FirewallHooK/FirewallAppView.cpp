// FirewallAppView.cpp : implementation of the CFirewallAppView class
//

#include "stdafx.h"
#include "FirewallApp.h"

#include "FirewallAppDoc.h"
#include "FirewallAppView.h"
#include "SockUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFirewallAppView

IMPLEMENT_DYNCREATE(CFirewallAppView, CFormView)

BEGIN_MESSAGE_MAP(CFirewallAppView, CFormView)
	//{{AFX_MSG_MAP(CFirewallAppView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CFormView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFirewallAppView construction/destruction

CFirewallAppView::CFirewallAppView()
	: CFormView(CFirewallAppView::IDD)
{
	//{{AFX_DATA_INIT(CFirewallAppView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// TODO: add construction code here

}

CFirewallAppView::~CFirewallAppView()
{
}

void CFirewallAppView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFirewallAppView)
	DDX_Control(pDX, IDC_LIST1, m_rules);
	//}}AFX_DATA_MAP
}

BOOL CFirewallAppView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFormView::PreCreateWindow(cs);
}

void CFirewallAppView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();
	GetParentFrame()->RecalcLayout();
	ResizeParentToFit();

	RECT rc;
	m_rules.GetClientRect(&rc);

	int width=rc.right-rc.left-110;
	m_rules.InsertColumn(0, "Source IP",LVCFMT_LEFT , width/6, 0);
	m_rules.InsertColumn(1, "Source Mask",LVCFMT_LEFT , width/6, 1);
	m_rules.InsertColumn(2, "Source Port",LVCFMT_LEFT ,width/6, 2);
	m_rules.InsertColumn(3, "Destination IP",LVCFMT_LEFT , width/6, 3);
	m_rules.InsertColumn(4, "Destination Mask",LVCFMT_LEFT , width/6, 4);
	m_rules.InsertColumn(5, "Destination Port",LVCFMT_LEFT , width/6, 5);
	m_rules.InsertColumn(6, "Protocol",LVCFMT_LEFT ,60, 6);
	m_rules.InsertColumn(7, "Action",LVCFMT_LEFT , 50, 7);

	m_rules.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);


}

/////////////////////////////////////////////////////////////////////////////
// CFirewallAppView printing

BOOL CFirewallAppView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CFirewallAppView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CFirewallAppView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CFirewallAppView::OnPrint(CDC* pDC, CPrintInfo* /*pInfo*/)
{
	// TODO: add customized printing code here
}

/////////////////////////////////////////////////////////////////////////////
// CFirewallAppView diagnostics

#ifdef _DEBUG
void CFirewallAppView::AssertValid() const
{
	CFormView::AssertValid();
}

void CFirewallAppView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CFirewallAppDoc* CFirewallAppView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFirewallAppDoc)));
	return (CFirewallAppDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFirewallAppView message handlers

void CFirewallAppView::UpdateList()
{
	CFirewallAppDoc *doc = GetDocument();

	// Update rules list
	m_rules.DeleteAllItems();

	unsigned int i;
	for(i=0;i<doc->nRules;i++)
	{
		AddRuleToList(doc->rules[i].sourceIp,
					  doc->rules[i].sourceMask,
					  doc->rules[i].sourcePort,
					  doc->rules[i].destinationIp,
					  doc->rules[i].destinationMask,
					  doc->rules[i].destinationPort,
					  doc->rules[i].protocol,
					  doc->rules[i].action);
	}
}

void CFirewallAppView::AddRuleToList(unsigned long srcIp, 
									 unsigned long srcMask,
									 unsigned short srcPort, 
									 unsigned long dstIp, 
									 unsigned long dstMask,
									 unsigned short dstPort, 
									 unsigned int protocol, 
									 int action)
{
	char ip[16];
	char port[6];
	LVITEM it;
	int pos;

	
	it.mask		= LVIF_TEXT;
	it.iItem	= m_rules.GetItemCount();
	it.iSubItem	= 0;
	it.pszText	= (srcIp == 0) ? "All" : IpToString(ip, srcIp);
	pos			= m_rules.InsertItem(&it);

	it.iItem	= pos;
	it.iSubItem	= 1;
	it.pszText	= IpToString(ip, srcMask);
	m_rules.SetItem(&it);

	it.iItem	= pos;
	it.iSubItem	= 2;
	it.pszText	= (srcPort == 0) ? "All" : itoa(srcPort, port, 10);
	m_rules.SetItem(&it);
	
	it.iItem	= pos;
	it.iSubItem	= 3;
	it.pszText	= (dstIp == 0) ? "All" : IpToString(ip, dstIp);
	m_rules.SetItem(&it);

	it.iItem	= pos;
	it.iSubItem	= 4;
	it.pszText	= IpToString(ip, dstMask);
	m_rules.SetItem(&it);

	it.iItem	= pos;
	it.iSubItem = 5;
	it.pszText	= (dstPort == 0) ? "All" : itoa(dstPort, port, 10);
	m_rules.SetItem(&it);


	it.iItem	= pos;
	it.iSubItem	= 6;

	if(protocol == 1)
		it.pszText = "ICMP";

	else if(protocol == 6)
		it.pszText = "TCP";

	else if(protocol == 17)
		it.pszText = "UDP";

	else
		it.pszText = "Todos";

	m_rules.SetItem(&it);


	it.iItem	= pos;
	it.iSubItem	= 7;
	it.pszText = action ? "Drop" : "Allow";
	m_rules.SetItem(&it);
}
