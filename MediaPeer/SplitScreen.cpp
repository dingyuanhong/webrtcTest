#include "stdafx.h"
#include "splitscreen.h"

BEGIN_MESSAGE_MAP(SplitItem, CStatic)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

static LONG ID_HEIGHT = 20;

SplitItem::SplitItem()	// standard constructor
	:ctr_id_(NULL), ctr_picture_(NULL)
	,show_(false)
{
}

SplitItem::~SplitItem()
{
	if (ctr_id_ != NULL) {
		ctr_id_->DestroyWindow();
		delete ctr_id_; ctr_id_ = NULL;
	}
	if (ctr_picture_ != NULL) {
		ctr_picture_->DestroyWindow();
		delete ctr_picture_; ctr_picture_ = NULL;
	}
}

BOOL SplitItem::Create(DWORD dwStyle,
	const RECT& rect, CWnd* pParent)
{
	BOOL ret = CStatic::Create(_T(""),dwStyle,rect,pParent,0);
	if (ret == FALSE) return ret; 

	LONG width = rect.right - rect.left;
	LONG height = rect.bottom - rect.top;

	CRect rc(0,0,width, ID_HEIGHT);
	ctr_id_ = new CStatic();
	ctr_id_->Create(_T(""), WS_CHILDWINDOW, rc,this);

	rc.top = ID_HEIGHT;
	rc.bottom = height;
	if (rc.bottom < rc.top) {
		rc.bottom = rc.top;
	}
	ctr_picture_ = new CStatic();
	ctr_picture_->Create(_T(""),WS_CHILDWINDOW,rc,this);

	ctr_id_->ShowWindow(SW_SHOW);
	ctr_picture_->ShowWindow(SW_SHOW);
	return ret;
}

void SplitItem::OnDestroty()
{
	CStatic::OnDestroy();
	if (ctr_id_ != NULL) {
		ctr_id_->ShowWindow(SW_HIDE);
		ctr_id_->DestroyWindow();
		delete ctr_id_; ctr_id_ = NULL;
	}
	if (ctr_picture_ != NULL) {
		ctr_picture_->ShowWindow(SW_HIDE);
		ctr_picture_->DestroyWindow();
		delete ctr_picture_; ctr_picture_ = NULL;
	}
}

void SplitItem::OnSize(UINT nType, int cx, int cy)
{
	CStatic::OnSize(nType,cx,cy);

	LONG width = cx;
	LONG height = cy;

	CRect rc( 0,0,width, ID_HEIGHT);
	if (ctr_id_ != NULL) {
		ctr_id_->MoveWindow(&rc);
	}

	CRect rc_picture( 0, ID_HEIGHT,width,0 );
	rc_picture.bottom = height;
	if (rc_picture.bottom < rc_picture.top) {
		rc_picture.bottom = rc_picture.top;
	}
	if (ctr_picture_ != NULL) {
		ctr_picture_->MoveWindow(&rc_picture);
	}
}

void SplitItem::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CWnd::OnShowWindow(bShow, nStatus);
	if (ctr_id_ != NULL) {
		ctr_id_->ShowWindow(bShow);
	}
	if (ctr_picture_ != NULL) {
		ctr_picture_->ShowWindow(bShow);
	}
	show_ = bShow;
}

void SplitItem::SetID(CString id)
{
	id_ = id;
}


SplitScreen::SplitScreen()
	:lockreset_(false), needreset_(false)
{
}

SplitScreen::~SplitScreen()
{
}

BOOL SplitScreen::Create(DWORD dwStyle,
	const RECT& rect, CWnd* pParent)
{
	return CStatic::Create(_T(""), dwStyle, rect, pParent);
}

void SplitScreen::AddItem(SplitItem *item)
{
	std::vector<SplitItem*>::iterator it =  std::find(items_.begin(), items_.end(), item);
	if (it == items_.end()) {
		items_.push_back(item);
	}
	if (lockreset_) {
		needreset_ = true;
	}
	else
	{
		ResetControls();
		needreset_ = false;
	}
}

void SplitScreen::RemoveItem(SplitItem *item)
{
	std::vector<SplitItem*>::iterator it = std::find(items_.begin(), items_.end(), item);
	if (it != items_.end()) {
		items_.erase(it);
	}

	if (lockreset_) {
		needreset_ = true;
	}
	else
	{
		ResetControls();
		needreset_ = false;
	}
}

void SplitScreen::LockReset()
{
	lockreset_ = true;
}

void SplitScreen::UnlockReset()
{
	if (needreset_) {
		ResetControls();
		needreset_ = false;
	}
	lockreset_ = false;
}

void SplitScreen::ResetControls()
{
	ASSERT(IsWindow(GetSafeHwnd()));
	CRect rect;
	GetWindowRect(&rect);
	LONG width = rect.right - rect.left;
	LONG height = rect.bottom - rect.top;
	int row = 1;

	if (items_.size() == 1) {
		row = 1;
	}
	else if (items_.size() <= 4) {
		row = 2;
	}
	else {
		row = 4;
		
	}

	int col = items_.size() / row + items_.size() % row;
	if (col == 0) col = 1;

	int rowStep = width / row;
	int colStep = height / col;

	for (size_t i = 0; i < items_.size(); i++)
	{
		int x = i % row;
		int y = i / row;
		rect = CRect( x * rowStep,y*colStep,(x + 1)*rowStep, (y + 1)* colStep );
		items_[i]->MoveWindow(&rect);
	}
}