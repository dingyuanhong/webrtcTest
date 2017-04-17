#pragma once

#include "afxwin.h"
#include <vector>

class SplitItem
	:public CStatic
{
public:
	SplitItem();	// standard constructor
	virtual ~SplitItem();
	virtual BOOL Create(DWORD dwStyle,
		const RECT& rect, CWnd* pParent = NULL);
	void SetID(CString id);

	DECLARE_MESSAGE_MAP()
protected:
	afx_msg void OnSize(UINT, int, int);
	afx_msg void OnDestroty();
	afx_msg void OnShowWindow(BOOL, UINT);
protected:
	CStatic *ctr_id_;
	CStatic *ctr_picture_;
	CFont* id_font_;
	CString id_;
	BOOL show_;
};

class SplitScreen :public CStatic
{
public:
	SplitScreen();	// standard constructor
	~SplitScreen();

	virtual BOOL Create(DWORD dwStyle,
		const RECT& rect, CWnd* pParent = NULL);

	void AddItem(SplitItem *item);
	void RemoveItem(SplitItem *item);
	
	void ResetControls();
	void LockReset();
	void UnlockReset();
private:
	std::vector<SplitItem*> items_;
	bool lockreset_;
	bool needreset_;
};