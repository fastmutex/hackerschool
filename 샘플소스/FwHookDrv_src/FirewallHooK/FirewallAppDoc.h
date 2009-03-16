// FirewallAppDoc.h : interface of the CFirewallAppDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_FIREWALLAPPDOC_H__615C5245_0FBE_434A_B124_2EAEB8BBD20B__INCLUDED_)
#define AFX_FIREWALLAPPDOC_H__615C5245_0FBE_434A_B124_2EAEB8BBD20B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Rules.h"

#define MAX_RULES 15

class CFirewallAppDoc : public CDocument
{
protected: // create from serialization only
	CFirewallAppDoc();
	DECLARE_DYNCREATE(CFirewallAppDoc)

// Attributes
public:
	unsigned int nRules;
	RuleInfo rules[MAX_RULES];

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFirewallAppDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	void DeleteRule(unsigned int position);
	void ResetRules();
	int AddRule(unsigned long srcIp, 
				unsigned long srcMask,
				unsigned short srcPort, 
				unsigned long dstIp,
				unsigned long dstMask,
				unsigned short dstPort,
				unsigned int protocol,
				int action);

	virtual ~CFirewallAppDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:


// Generated message map functions
protected:
	//{{AFX_MSG(CFirewallAppDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FIREWALLAPPDOC_H__615C5245_0FBE_434A_B124_2EAEB8BBD20B__INCLUDED_)
