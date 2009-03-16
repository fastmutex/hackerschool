// FirewallAppDoc.cpp : implementation of the CFirewallAppDoc class
//

#include "stdafx.h"
#include "FirewallApp.h"

#include "FirewallAppDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFirewallAppDoc

IMPLEMENT_DYNCREATE(CFirewallAppDoc, CDocument)

BEGIN_MESSAGE_MAP(CFirewallAppDoc, CDocument)
	//{{AFX_MSG_MAP(CFirewallAppDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFirewallAppDoc construction/destruction

CFirewallAppDoc::CFirewallAppDoc()
{
	nRules = 0;
}

CFirewallAppDoc::~CFirewallAppDoc()
{
}

BOOL CFirewallAppDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CFirewallAppDoc serialization

void CFirewallAppDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CFirewallAppDoc diagnostics

#ifdef _DEBUG
void CFirewallAppDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CFirewallAppDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFirewallAppDoc commands

int CFirewallAppDoc::AddRule(unsigned long srcIp,
							 unsigned long srcMask,
							 unsigned short srcPort,
							 unsigned long dstIp,
							 unsigned long dstMask,
							 unsigned short dstPort,
							 unsigned int protocol,
							 int action)
{

	if(nRules >= MAX_RULES)
	{
		return -1;
	}

	else
	{
		rules[nRules].sourceIp		  = srcIp;
		rules[nRules].sourceMask	  = srcMask;
		rules[nRules].sourcePort	  = srcPort;
		rules[nRules].destinationIp   = dstIp;
		rules[nRules].destinationMask = dstMask;
		rules[nRules].destinationPort = dstPort;
		rules[nRules].protocol		  = protocol;
		rules[nRules].action		  = action;

		nRules++;
	}

	return 0;
}

void CFirewallAppDoc::ResetRules()
{
	nRules = 0;
}

void CFirewallAppDoc::DeleteRule(unsigned int position)
{
	// Out of range
	if(position >= nRules)
		return;

	// If it is the last rule, decrement nRule
	if(position != nRules - 1)
	{
		unsigned int i;

		for(i = position + 1;i<nRules;i++)
		{
			rules[i - 1].sourceIp		  = rules[i].sourceIp;
			rules[i - 1].sourceMask		  = rules[i].sourceMask;
			rules[i - 1].sourcePort		  = rules[i].sourcePort;
			rules[i - 1].destinationIp    = rules[i].destinationIp;
			rules[i - 1].destinationMask  = rules[i].destinationMask;
			rules[i - 1].destinationPort  = rules[i].destinationPort;
			rules[i - 1].protocol	      = rules[i].protocol;
			rules[i - 1].action		      = rules[i].action;
		}
	}
	nRules--;
}
