#include "StdAfx.h"

#include "PlcMonitorWnd.h"

#include "../Resource.h"
#include "../Global.h"

#include "FXPlc.h"
#include "SerialTask.h"

//#include "./inc/msxml6.h"

#pragma warning (disable: 4996)

//#pragma comment(lib, "./lib/msxml6.lib")

////////////////////////////////////////////////////////////////////////////////
//
//  CPlcMonitorCell
//

CPlcMonitorCell::CPlcMonitorCell()
{
	m_rect = CRect(0, 0, 0, 0);
	m_strDeviceUnitAdd = "";
	m_nDeviceUnitType = DeviceUnitBit;

	m_colorTxtLight = RGB(255, 255, 255);
	m_colorTxtDark = RGB(128, 128, 128);

	m_pImageLight = NULL;
	m_pImageDark = NULL;
	m_pImageBlack = NULL;

	m_bMouseOnCell = false;

	m_nDeviceNum = 1;

	m_accessStatus = StatusIdle;

    m_nCellStatus = CellStatusNormal;

    m_nCellValue = 0;
}

CPlcMonitorCell::~CPlcMonitorCell()
{
}

void CPlcMonitorCell::SetDeviceUnitAdd(string strUnitAdd)
{
}

bool CPlcMonitorCell::LoadCellImage(LPCTSTR szLightFile, LPCTSTR szDarkFile, LPCSTR szBlackFile)
{
    return true;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CPlcMonitorTaskItem
//

CPlcMonitorTaskItem::CPlcMonitorTaskItem()
{
	m_taskAction = PlcTaskRead;
	m_plcDataType = PlcDataBit;
	m_nDataLen = 0;
	m_strResult = ""; //ASCII
}

CPlcMonitorTaskItem::~CPlcMonitorTaskItem()
{
	m_vecCell.clear(); //不删除队列中成员指向的对象
}

////////////////////////////////////////////////////////////////////////////////
//
//  CPlcMonitorWnd
//

CPlcMonitorWnd::CPlcMonitorWnd(void)
{
	InitializeCriticalSection(&m_sectionMonitorWnd);

	m_hPlcMonitorTimer = INVALID_HANDLE_VALUE;

	m_pCurCellUnderMouse = NULL;

	m_nRunningEnqTaskNum = 0;

    //7段数码
    m_nDigitWidth		= 22;
	m_nDigitHeight		= 32;

    m_bWndClosing = false;

	m_paddingX = 100;
	m_paddingY = 100;
	m_contentWidth = 200;
	m_contentHeight = 200;
}

CPlcMonitorWnd::~CPlcMonitorWnd(void)
{
    DeleteCriticalSection(&m_sectionMonitorWnd);
	//DestroyObjects();
}

BOOL CPlcMonitorWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	TRACE(_T("PreCreateWindow\n"));
	return CWnd::PreCreateWindow(cs);
}
BEGIN_MESSAGE_MAP(CPlcMonitorWnd, CWnd)
	ON_COMMAND(ID_EDIT_CLEAR, &CPlcMonitorWnd::OnEditClear)
	ON_COMMAND(ID_EDIT_COPY, &CPlcMonitorWnd::OnEditCopy)
	ON_COMMAND(ID_EDIT_CUT, &CPlcMonitorWnd::OnEditCut)
	//ON_COMMAND(ID_VIEW_OUTPUTWND, &CPlcMonitorWnd::OnViewOutput)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_SETFOCUS()
	ON_WM_RBUTTONDOWN()
	ON_WM_SETCURSOR()
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
    //ON_MESSAGE(WM_SETFONT, OnSetFont)
    //ON_MESSAGE(WM_GETFONT, OnGetFont)
	ON_MESSAGE(WM_SERIAL_TASK_FINISH, &CPlcMonitorWnd::OnSerialTaskFinish)
	ON_MESSAGE(WM_UPDATE_PLCMON_CELL, &CPlcMonitorWnd::OnUpdateMonitorCell)
END_MESSAGE_MAP()

//LRESULT CPlcMonitorWnd::OnSetFont(WPARAM wParam, LPARAM lParam)
//{
//	return 0;
//}
//
//LRESULT CPlcMonitorWnd::OnGetFont(WPARAM wParam, LPARAM lParam)
//{
//	return 0;
//}

void CPlcMonitorWnd::OnEditClear()
{
}

void CPlcMonitorWnd::OnEditCopy()
{
}

void CPlcMonitorWnd::OnEditCut()
{
}

BOOL CPlcMonitorWnd::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	RegisterWindowClass();

    LOGFONT m_lpfont;
    memset(&m_lpfont, 0, sizeof(LOGFONT));

    m_lpfont.lfHeight = 20;
    lstrcpy(m_lpfont.lfFaceName, _T("微软雅黑"));
	//m_lpfont.lfWeight = FW_BOLD;

    m_defaultFont.CreateFontIndirect(&m_lpfont);

	return CWnd::Create(PLC_MONITOR_WND_CLASSNAME, NULL, dwStyle, rect, pParentWnd, nID);
}

BOOL CPlcMonitorWnd::RegisterWindowClass()
{
    WNDCLASS wndcls;
    //HINSTANCE hInst = AfxGetInstanceHandle();
    HINSTANCE hInst = AfxGetResourceHandle();

    if (!(::GetClassInfo(hInst, PLC_MONITOR_WND_CLASSNAME, &wndcls)))
    {
        // otherwise we need to register a new class
        wndcls.style            = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        wndcls.lpfnWndProc      = ::DefWindowProc;
        wndcls.cbClsExtra       = wndcls.cbWndExtra = 0;
        wndcls.hInstance        = hInst;
        wndcls.hIcon            = NULL;
        wndcls.hCursor          = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
        wndcls.hbrBackground    =(HBRUSH) (COLOR_3DFACE + 1);
        wndcls.lpszMenuName     = NULL;
        wndcls.lpszClassName    = PLC_MONITOR_WND_CLASSNAME;

        if (!AfxRegisterClass(&wndcls))
        {
            AfxThrowResourceException();
            return FALSE;
        }
    }

    return TRUE;
}

void CPlcMonitorWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rectClient;
	GetClientRect(&rectClient);

	m_paddingX = (rectClient.Width() - m_contentWidth)/2 - 50;
	m_paddingY = (rectClient.Height() - m_contentHeight) / 2 - 60;

    //Draw static text and iamges
    //m_imageBallRed.Draw(dc, CRect(0,0,48,48));
    //m_imageBallGray.Draw(dc, CRect(50,0,98,48));
    //m_imageBallGreen.Draw(dc, CRect(100,0,148,48));
    //m_imageBallOrange.Draw(dc, CRect(150,0,198,48));

    {
       	int nOldBkMode = dc.SetBkMode(TRANSPARENT);

        CString csSysName((LPCTSTR)ID_SYSTEM_NAME);//_T("磁 粉 探 伤 机 监 控 系 统");

        CString csName((LPCTSTR)ID_COMPANY_NAME);// = _T("盐 城 市 电 子 设 备 厂");

        LOGFONT m_lpfont;
        memset(&m_lpfont, 0, sizeof(LOGFONT));

        m_lpfont.lfHeight = 50;
        //m_lpfont.lfEscapement = 0;
        //m_lpfont.lfOrientation = 0;
        lstrcpy(m_lpfont.lfFaceName, _T("微软雅黑"));

        CFont font;
        font.CreateFontIndirect(&m_lpfont);

        m_lpfont.lfHeight = 55;

        CFont font1;
        font1.CreateFontIndirect(&m_lpfont);

        CFont* 	pOldFont;
        pOldFont = dc.SelectObject(&font);

        CSize sizeStr = dc.GetOutputTextExtent(csName);
        CPoint pointStart(rectClient.left + (rectClient.Width() - sizeStr.cx) / 2, rectClient.Height() - (rectClient.Height() - m_paddingY - m_contentHeight - sizeStr.cy) / 2);

        dc.ExtTextOut( pointStart.x, pointStart.y, ETO_CLIPPED, CRect(pointStart.x, pointStart.y, pointStart.x + sizeStr.cx, pointStart.y + sizeStr.cy), csName, NULL);

        dc.SelectObject(&font1);
        sizeStr = dc.GetOutputTextExtent(csSysName);

        pointStart.x = rectClient.left + (rectClient.Width() - sizeStr.cx) / 2;
        pointStart.y = (m_paddingY - sizeStr.cy) / 2;

        dc.ExtTextOut( pointStart.x, pointStart.y, ETO_CLIPPED, CRect(pointStart.x, pointStart.y, pointStart.x + sizeStr.cx, pointStart.y + sizeStr.cy), csSysName, NULL);

		//
		//CString csGroup1("功 能 选 择");

		//m_lpfont.lfHeight = 20;
  //      CFont font2;
  //      font2.CreateFontIndirect(&m_lpfont);

  //      dc.SelectObject(&font2);
  //      sizeStr = dc.GetOutputTextExtent(csGroup1);

		//pointStart.x = m_paddingX + 710 - sizeStr.cx/2;
		//pointStart.y = m_paddingY + 255 - sizeStr.cy;
  //      dc.ExtTextOut( pointStart.x, pointStart.y, ETO_CLIPPED, CRect(pointStart.x, pointStart.y, pointStart.x + sizeStr.cx, pointStart.y + sizeStr.cy), csGroup1, NULL);

		//dc.MoveTo(m_paddingX + 860, pointStart.y + sizeStr.cy);
		//dc.LineTo(m_paddingX + 560, pointStart.y + sizeStr.cy );
		//dc.LineTo(m_paddingX + 560, pointStart.y + sizeStr.cy + 10 );
		//
		//dc.MoveTo(m_paddingX + 860, pointStart.y + sizeStr.cy);
		//dc.LineTo(m_paddingX + 860, pointStart.y + sizeStr.cy + 10 );

		//dc.MoveTo(m_paddingX + 710, pointStart.y + sizeStr.cy );
		//dc.LineTo(m_paddingX + 710, pointStart.y + sizeStr.cy + 10 );


		//CString csGroup2("电 极 箱");
  //      sizeStr = dc.GetOutputTextExtent(csGroup2);

		//pointStart.x = m_paddingX + 485 - sizeStr.cx/2;
		//pointStart.y = m_paddingY + 455 - sizeStr.cy;
  //      dc.ExtTextOut( pointStart.x, pointStart.y, ETO_CLIPPED, CRect(pointStart.x, pointStart.y, pointStart.x + sizeStr.cx, pointStart.y + sizeStr.cy), csGroup2, NULL);

		//dc.MoveTo(m_paddingX + 410, pointStart.y + sizeStr.cy);
		//dc.LineTo(m_paddingX + 560, pointStart.y + sizeStr.cy );
		//dc.LineTo(m_paddingX + 560, pointStart.y + sizeStr.cy + 10 );
		//
		//dc.MoveTo(m_paddingX + 410, pointStart.y + sizeStr.cy);
		//dc.LineTo(m_paddingX + 410, pointStart.y + sizeStr.cy + 10 );

        dc.SelectObject(pOldFont);
       	dc.SetBkMode(nOldBkMode);

		font.DeleteObject();
		font1.DeleteObject();
    }

	for ( int i = 0; i < (int)m_vecMonitorCells.size(); i ++ )
	{
		CPlcMonitorCell* pCell = m_vecMonitorCells.at(i);

		DrawCell(&dc, pCell);
	}

    //CImage temp;
    //LoadImage(IDB_PNG_BUTTON_RED, _T("PNG"), temp);

    //temp.Draw(dc, CRect(0, 0,48, 48));


	CWnd::ShowScrollBar( SB_BOTH, 0);
}

void CPlcMonitorWnd::DrawButtonCell(CDC* pDC, CRect& rectDst, bool bActive, int nColorId, int nWidth /*= 48*/, int nHeight /* = 48*/)
{
	Gdiplus::Graphics graphics(*pDC);

    bool m_bMouseOver = false;
    bool m_bMouseClicked = false;

    //CRect m_ClientRect(0, 0, rectDst.Width(), rectDst.Height());
    CRect m_ClientRect(0, 0, nWidth, nHeight);
    {
        Gdiplus::Graphics* m_pMemGraphics = NULL;  
        Gdiplus::Bitmap*	pMemBitmap = NULL;	
        //Gdiplus::CachedBitmap* m_pCachedBitmap = NULL; 

        // create off-screen bitmap
        pMemBitmap = new Gdiplus::Bitmap(m_ClientRect.Width(), m_ClientRect.Height());

        // create off-screen graphics
        m_pMemGraphics = Gdiplus::Graphics::FromImage(pMemBitmap);

        // Turn anti-aliasing ON
        m_pMemGraphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

        // Calculate component sizes (based on the client size)
        Gdiplus::Rect rectInner = Gdiplus::Rect(m_ClientRect.left, m_ClientRect.top, m_ClientRect.Width(), m_ClientRect.Height());
        Gdiplus::Rect rectInnerOutline = rectInner;
        Gdiplus::Rect rectOutter = rectInner;
        rectInner.Inflate(-6, -6);
        rectInnerOutline.Inflate(-1, -1);

        Gdiplus::Color colorFaceStart, colorFaceEnd;

        if (nColorId == 0) //Red
        {
            colorFaceStart = Gdiplus::Color(255, 255, 0, 0);
            if (bActive)
                colorFaceEnd = Gdiplus::Color(255, 200, 0, 0);
            else
                colorFaceEnd = Gdiplus::Color(255, 100, 0, 0);
        }
        else if (nColorId == 1) //Green
        {
            colorFaceStart = Gdiplus::Color(255, 0, 255, 0);

            if (bActive)
                colorFaceEnd = Gdiplus::Color(255, 0, 200, 0);
            else
                colorFaceEnd = Gdiplus::Color(255, 0, 100, 0);
        }
        else if (nColorId == 2)//Blue
        {
            colorFaceStart = Gdiplus::Color(255, 0, 0, 255);
            if (bActive)
                colorFaceEnd = Gdiplus::Color(255, 0, 0, 200);
            else
                colorFaceEnd = Gdiplus::Color(255, 0, 0, 100);
        }
        else //Gray
        {
            colorFaceStart = Gdiplus::Color(255, 225, 225, 225);

            if (bActive)
                colorFaceEnd = Gdiplus::Color(255, 200, 200, 200);
            else
                colorFaceEnd = Gdiplus::Color(255, 100, 100, 100);
        }
        // Create appropriate colors and brushes (these are client size dependent)		
        Gdiplus::LinearGradientBrush InnerBrushMouseNoClick( rectInner, colorFaceStart, colorFaceEnd, 0, true);	
        
        //Gdiplus::LinearGradientBrush InnerBrushMouseClicked( rectInner, Gdiplus::Color(255, 100, 149, 255), Gdiplus::Color(255, 100, 149, 255), 0, true);	
        
        Gdiplus::LinearGradientBrush OutterBrushMouseOut( rectOutter, Gdiplus::Color(255, 100, 100, 100), Gdiplus::Color(255, 200, 200, 200), 0, true);
        Gdiplus::LinearGradientBrush OutterBrushMouseIn ( rectOutter, Gdiplus::Color(100, 150, 150, 0), Gdiplus::Color(200, 255, 255, 0), 0, true);

        Gdiplus::Pen grayPen((Gdiplus::ARGB)(Gdiplus::Color::Gray));

        // Draw the background	
        if (!bActive)
            m_pMemGraphics->FillEllipse(&OutterBrushMouseOut, rectOutter); 
        else
            m_pMemGraphics->FillEllipse(&OutterBrushMouseIn, rectOutter); 

        //if (!m_bMouseClicked)
            m_pMemGraphics->FillEllipse(&InnerBrushMouseNoClick, rectInner); 
        //else
        //    m_pMemGraphics->FillEllipse(&InnerBrushMouseClicked, rectInner); 

        m_pMemGraphics->DrawEllipse(&grayPen, rectInner); 
        m_pMemGraphics->DrawEllipse(&grayPen, rectInnerOutline); 	

        // Prepare brush and font for text drawing
        //Gdiplus::SolidBrush  fontBrush(Gdiplus::Color(255, 255, 255, 255));
        ////Gdiplus::REAL rFontSize = static_cast<Gdiplus::REAL>(Gdiplus::min(((Gdiplus::REAL)m_ClientRect.Height()*0.2), ((Gdiplus::REAL)m_ClientRect.Width()*0.2)));
        //Gdiplus::REAL rFontSize = static_cast<Gdiplus::REAL>(m_ClientRect.Height()*0.2);
        //if ( rFontSize > (Gdiplus::REAL)m_ClientRect.Width()*0.2)
        //{
        //    rFontSize = static_cast<Gdiplus::REAL>(m_ClientRect.Width()*0.2);
        //}
        //Gdiplus::Font font(L"宋体", rFontSize, Gdiplus::FontStyleBold, Gdiplus::UnitPixel );		

        //// Calculate text placement (center)
        //Gdiplus::RectF textBoundRect;	
        //Gdiplus::PointF ptTextOrigin; 
        //m_pMemGraphics->MeasureString(L"工作", 2, &font, ptTextOrigin, &textBoundRect);
        //ptTextOrigin	=  Gdiplus::PointF ((m_ClientRect.Width()- textBoundRect.Width)/2+1, (m_ClientRect.Height()-textBoundRect.Height)/2+1);	

        //// Finally draw the text
        //m_pMemGraphics->DrawString(L"工作", -1, &font, ptTextOrigin, &fontBrush);


        /************************************************/
        /*			ACTUAL DRAWING ENDS HERE			*/
        /************************************************/

        // Create a cached bitmap for double buffering
        //if (m_pCachedBitmap) delete m_pCachedBitmap;
        //m_pCachedBitmap = new CachedBitmap(pMemBitmap, &graphics);


     //   if (m_pCachedBitmap) delete m_pCachedBitmap;
     //   m_pCachedBitmap = new Gdiplus::CachedBitmap(pMemBitmap, &graphics);
    	//graphics.DrawCachedBitmap(m_pCachedBitmap, 0, 500);

        int x = m_paddingX + rectDst.left + (rectDst.Width() - nWidth) / 2;
        int y = m_paddingY + rectDst.bottom - nHeight - 1;

        graphics.DrawImage(pMemBitmap, x, y);

        delete pMemBitmap;
        delete m_pMemGraphics;		
        
        //if (m_pCachedBitmap) delete m_pCachedBitmap;
    }
}


void CPlcMonitorWnd::ConstructMap(void)
{
	vector<CPlcMonitorCell*>::iterator itMonitorCell;

	for ( itMonitorCell = m_vecMonitorCells.begin(); itMonitorCell != m_vecMonitorCells.end(); ++ itMonitorCell )
	{
		m_mapAddCells.insert(map<string, CPlcMonitorCell*>::value_type((*itMonitorCell)->m_strDeviceUnitAdd, *itMonitorCell) );

		//To construct the enquire address.
		//
	}
}

void CPlcMonitorWnd::DestroyObjects(void)
{
	EnterCriticalSection(&m_sectionMonitorWnd);
	m_bWndClosing = true;
    LeaveCriticalSection(&m_sectionMonitorWnd);

	DestroyPlcMonitorTimer();

    Sleep(5000);

    //断开PC和PLC连接
    GetGlobal()->m_serialTask->CreateFxPlcNoProtocolForceTask(BREAK_PC_PLC_LINK, false, NULL);
    TRACE(_T(">>==Break the link==<<\n"));
    //
    Sleep(15000);

    ReleaseAllResource();
	
	m_mapAddCells.clear();

	//
	vector<CPlcMonitorTaskItem*>::iterator itPlcMonTaskItem;
	for ( itPlcMonTaskItem = m_vecTaskItem.begin(); itPlcMonTaskItem != m_vecTaskItem.end(); ++itPlcMonTaskItem )
	{
		if ( NULL != (*itPlcMonTaskItem) )
		{
			delete (*itPlcMonTaskItem);
		}
	}
	m_vecTaskItem.clear();

	//
	vector<CPlcMonitorCell*>::iterator itMonitorCell;
	for ( itMonitorCell = m_vecMonitorCells.begin(); itMonitorCell != m_vecMonitorCells.end(); ++itMonitorCell )
	{
		if ( NULL != (*itMonitorCell) )
		{
			delete (*itMonitorCell);
		}
	}
	m_vecMonitorCells.clear();
}

void CPlcMonitorWnd::AddMonitorCell(CPlcMonitorCell* pNewCell)
{
	m_vecMonitorCells.push_back(pNewCell);
}


void CPlcMonitorWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	CPlcMonitorCell* pCell = GetCurrentCell(point);

	if ( pCell != NULL && pCell->m_nCellType != CPlcMonitorCell::CellTypeReadonly)
	{
		//OutputDebugLine(pCell->m_csPrompt);

		if ( pCell->m_nDeviceUnitType == CPlcMonitorCell::DeviceUnitBit)
		{
            if ( pCell->m_nCellStatus != CPlcMonitorCell::CellStatusDisable ) //CPlcMonitorCell::CellStatusNormal )
			{
				//Create force ON/OFF
				if ( pCell->m_nCellValue == 0 )
				{ //Force on
					GetGlobal()->m_serialTask->CreateFxPlcNoProtocolForceTask((char*)pCell->m_strDeviceUnitAdd.c_str(), true, this, 0, pCell);
				}
				else
				{ //Force off
					GetGlobal()->m_serialTask->CreateFxPlcNoProtocolForceTask((char*)pCell->m_strDeviceUnitAdd.c_str(), false, this, 0, pCell);
				}

				pCell->m_nCellStatus = CPlcMonitorCell::CellStatusPushdown;
			}
		}

	}

	SetFocus();

	CWnd::OnLButtonDown(nFlags, point);
}

void CPlcMonitorWnd::AutoConstructCells(void)		//Auto contruct base the PLC
{
	CRect rectTemp(0,0,24,24);

	int i = 0;
	char szTempAdd[20];
	for ( ; i < 20; i ++ )
	{
		CPlcMonitorCell* pNewCell = new CPlcMonitorCell;

		pNewCell->m_rect = rectTemp;

		pNewCell->m_rect.NormalizeRect();

		sprintf(szTempAdd, "X%04o", i);
		pNewCell->m_strDeviceUnitAdd = szTempAdd; 

		pNewCell->m_nAlign = DT_CENTER | DT_VCENTER | DT_SINGLELINE;

		sprintf(szTempAdd, "%d", i);

		//
		int nWstrLen = MultiByteToWideChar(CP_ACP, 0, szTempAdd, -1, NULL, 0);
		TCHAR* szBuf = new TCHAR[nWstrLen];

		MultiByteToWideChar(CP_ACP, 0, szTempAdd, -1,  szBuf, nWstrLen);

		pNewCell->m_csPrompt = szBuf;
	
		delete szBuf;
		//

		pNewCell->m_nCellStatus = CPlcMonitorCell::CellStatusNormal;

		//
		pNewCell->m_nCellType = CPlcMonitorCell::CellTypeReadonly;
		pNewCell->m_nCellValue = 0;

		if ( i % 5 == 0 )
			pNewCell->m_nCellType = CPlcMonitorCell::CellTypeRW;

		if ( i%3 == 0 )
		{
			pNewCell->m_nCellStatus = CPlcMonitorCell::CellStatusNormal;
			pNewCell->m_nCellValue = 1;
		}

		AddMonitorCell(pNewCell);

		rectTemp.OffsetRect( rectTemp.Width(), 0 );

	}
}

CImage* CPlcMonitorWnd::GetImage(UINT imageUid)
{
    switch (imageUid)
    {
    case IDB_PNG_BALL_GRAY:
        return &m_imageBallGray;
    case IDB_PNG_BALL_RED:
        return &m_imageBallRed;
    case IDB_PNG_BALL_GREEN:
        return &m_imageBallGreen;
    case IDB_PNG_BALL_ORANGE:
        return &m_imageBallOrange;
    }
    return NULL;
}


int CPlcMonitorWnd::ConstructCells(void)
{
	int nUiUnitNum = GetGlobal()->m_nControlUiUnitNum;

	if ( nUiUnitNum > 0 )
	{
		int i = 0;
		for (; i < nUiUnitNum; i ++ )
		{
			CPlcMonitorCell* pNewCell = new CPlcMonitorCell;

			CControlUiUnit* pArrUiPara = GetGlobal()->m_arrControlUiPara;

			pNewCell->m_csPrompt = pArrUiPara[i].csCtrlName;

			if ( pArrUiPara[i].csPlcUnit.GetLength() > 0 )
			{
				LPCSTR pMbcs = UnicodetoMBCS((LPWSTR)(LPCTSTR)pArrUiPara[i].csPlcUnit); //Have to convert to ASCII

				pNewCell->m_strDeviceUnitAdd =  pMbcs;

				delete pMbcs;
			}

			pNewCell->m_rect.top = pArrUiPara[i].nTop;
			pNewCell->m_rect.left = pArrUiPara[i].nLeft;
			pNewCell->m_rect.bottom = pArrUiPara[i].nTop + pArrUiPara[i].nHeight;
			pNewCell->m_rect.right = pArrUiPara[i].nLeft + pArrUiPara[i].nWidth;

			TRACE("%d : %d,%d\n",i, pNewCell->m_rect.Width(), pNewCell->m_rect.Height());

			pNewCell->m_nDeviceUnitType = (CPlcMonitorCell::DeviceUnitType)pArrUiPara[i].nCtrlType;

			pNewCell->m_nCellType = (CPlcMonitorCell::CellType)pArrUiPara[i].nAccessMode;

			pNewCell->m_csDigitFormat = pArrUiPara[i].csDigitFormat;

			pNewCell->m_nDeviceNum = pArrUiPara[i].nDeviceNum;

			pNewCell->m_nAlign = DT_CENTER | DT_VCENTER | DT_SINGLELINE;

            //加入资源的引用
            pNewCell->m_pImageLight = GetImage(pArrUiPara[i].uHighlightID);
            pNewCell->m_pImageDark = GetImage(pArrUiPara[i].uLowlightID);
            pNewCell->m_pImageBlack = GetImage(pArrUiPara[i].uDisableID);

			AddMonitorCell(pNewCell);
		}

		{
			CPlcMonitorCell* pFirstCell = m_vecMonitorCells.at(0);
			CRect rectClient(pFirstCell->m_rect);

			for ( int i = 0; i < (int)m_vecMonitorCells.size(); i ++ )
			{
				CPlcMonitorCell* pCell = m_vecMonitorCells.at(i);
				if (rectClient.left > pCell->m_rect.left)
					rectClient.left = pCell->m_rect.left;
				if (rectClient.right < pCell->m_rect.right)
					rectClient.right = pCell->m_rect.right;
				if (rectClient.top > pCell->m_rect.top )
					rectClient.top = pCell->m_rect.top;
				if (rectClient.bottom < pCell->m_rect.bottom)
					rectClient.bottom = pCell->m_rect.bottom;
			}

			m_contentWidth = rectClient.Width();
			m_contentHeight = rectClient.Height();
		}

	}

	return nUiUnitNum;
}

CRect CPlcMonitorWnd::CalculateCellsDimension(void)
{
	CRect rectTemp(0,0,0,0);

	if ( !m_vecMonitorCells.empty())
	{
		vector<CPlcMonitorCell*>::iterator itCells = m_vecMonitorCells.begin();
		rectTemp = (*itCells)->m_rect;

		itCells ++;

		for (; itCells != m_vecMonitorCells.end(); ++ itCells)
		{
			if ( (*itCells)->m_rect.top < rectTemp.top )
				rectTemp.top = (*itCells)->m_rect.top;
			if ( (*itCells)->m_rect.left < rectTemp.left )
				rectTemp.left = (*itCells)->m_rect.left;
			if ( (*itCells)->m_rect.bottom > rectTemp.bottom )
				rectTemp.bottom = (*itCells)->m_rect.bottom;
			if ( (*itCells)->m_rect.right > rectTemp.right )
				rectTemp.right = (*itCells)->m_rect.right;
		}
	}
	return rectTemp;
}

CPlcMonitorCell* CPlcMonitorWnd::GetCurrentCell(CPoint point)
{
	vector<CPlcMonitorCell*>::iterator itCells = m_vecMonitorCells.begin();

	for (; itCells != m_vecMonitorCells.end(); ++ itCells)
	{
		CRect rectOffset = (*itCells)->m_rect;
		rectOffset.OffsetRect(m_paddingX, m_paddingY);
//		if ((*itCells)->m_rect.PtInRect(point))
		if (rectOffset.PtInRect(point))
			return (*itCells);
	}

	return NULL;
}
void CPlcMonitorWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	CWnd::OnMouseMove(nFlags, point);

	//if ( GetActiveWindow() == this )
	//{
	//	TRACE("The Plc Monitor Wnd is active\n");
	//}


	CPlcMonitorCell* pCurCell = GetCurrentCell(point);

	if ( m_pCurCellUnderMouse != pCurCell )
	{
		CDC* pDC = GetDC();

		if ( m_pCurCellUnderMouse != NULL && m_pCurCellUnderMouse->m_nCellType == 1 )
		{
			DrawCell(pDC, m_pCurCellUnderMouse);
		}

		m_pCurCellUnderMouse = pCurCell;

		if ( pCurCell != NULL && pCurCell->m_nCellType == 1)
		{
			DrawCellHot(pDC, pCurCell);

			TRACKMOUSEEVENT		csTME;
			csTME.cbSize = sizeof(csTME);
			csTME.dwFlags = TME_LEAVE;
			csTME.hwndTrack = m_hWnd;
			::_TrackMouseEvent(&csTME);
		}

		ReleaseDC(pDC);
	}
}

void CPlcMonitorWnd::OnViewOutput()
{
	//CDockablePane* pParentBar = DYNAMIC_DOWNCAST(CDockablePane, GetOwner());
	//CMDIFrameWndEx* pMainFrame = DYNAMIC_DOWNCAST(CMDIFrameWndEx, GetTopLevelFrame());

	//if (pMainFrame != NULL && pParentBar != NULL)
	//{
	//	pMainFrame->SetFocus();
	//	pMainFrame->ShowPane(pParentBar, FALSE, FALSE, FALSE);
	//	pMainFrame->RecalcLayout();
	//}
}

void CPlcMonitorWnd::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
}

void CPlcMonitorWnd::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();

	CWnd::OnRButtonDown(nFlags, point);
}

BOOL CPlcMonitorWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if ( nHitTest == HTCLIENT )
	{
		CPoint point;
		::GetCursorPos(&point); 
		ScreenToClient(&point); 

		CPlcMonitorCell* pCell = GetCurrentCell(point);
		if ( pCell != NULL && pCell->m_nCellType != 0)
		{
			HCURSOR hCursor = LoadCursor(NULL, IDC_HAND);
			SetCursor(hCursor);
			return true;
		}
	}

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

CPlcMonitorCell* CPlcMonitorWnd::FindCell(string strUnitAdd)
{
	map<string, CPlcMonitorCell*>::iterator itAddCell = m_mapAddCells.find(strUnitAdd);

	if ( itAddCell != m_mapAddCells.end() )
	{
		return (itAddCell->second);
	}
	else
		return NULL;
}

CPlcMonitorCell* CPlcMonitorWnd::FindCell(char* pUnitAdd)
{
	string strUnitAdd = pUnitAdd;
	map<string, CPlcMonitorCell*>::iterator itAddCell = m_mapAddCells.find(strUnitAdd);
	return FindCell(strUnitAdd);
}

//查找给定地址所对应的UI对象
CPlcMonitorCell* CPlcMonitorWnd::FindOutputBitCell(char* pUnitAdd)
{
	vector<CPlcMonitorCell*>::iterator itPlcMonCell = m_vecMonitorCells.begin();

	for (; itPlcMonCell != m_vecMonitorCells.end(); ++ itPlcMonCell )
	{
		if ( (*itPlcMonCell)->m_nCellType == CPlcMonitorCell::CellTypeRW && (*itPlcMonCell)->m_strDeviceUnitAdd.compare(pUnitAdd) == 0 )
		{
			return (*itPlcMonCell);
		}
	}

	return NULL;
}

void CPlcMonitorCell::FormatValueString()
{
    if (m_csDigitFormat == "0")
    {
        m_csContent.Format(_T("%d"), m_nCellValue);
    }
    else if (m_csDigitFormat == "1")
    {
        m_csContent.Format(_T("%04X"), m_nCellValue);
    }
    else if (m_csDigitFormat == "2")
    {
        m_csContent.Format(_T("%04o"), m_nCellValue);
    }
}

void CPlcMonitorWnd::UpdateCell(CPlcMonitorCell* pCell)
{
	if ( pCell != NULL )
	{
		CDC* pDC = GetDC();

		DrawCell(pDC, pCell);

		ReleaseDC(pDC);
	}
}

void CPlcMonitorWnd::DrawCell(CDC* pDC, CPlcMonitorCell* pCell)
{
	DWORD dwHiLight = GetSysColor(COLOR_3DHIGHLIGHT);
	DWORD dwShadow = GetSysColor(COLOR_3DSHADOW);
	DWORD dwGrayText = GetSysColor(COLOR_GRAYTEXT);

	CString csTxt;

	int nOldBkMode = pDC->SetBkMode(TRANSPARENT);

	TRIVERTEX vertex[2] ;

	GRADIENT_RECT gRect;
	gRect.UpperLeft  = 0;
	gRect.LowerRight = 1;

    COLORREF clrOrg = pDC->GetTextColor();

	if ( pCell->m_nDeviceUnitType == 0 ) //Bit device
	{
        if ((pCell->m_pImageBlack != NULL) && (pCell->m_pImageLight != NULL) && (pCell->m_pImageDark != NULL))
        {
            CPoint pointStart = CPoint(pCell->m_rect.left + m_paddingX, pCell->m_rect.top + m_paddingY);
            //CRect rectText = pCell->m_rect;
			CRect rectText;
			rectText.top =  pCell->m_rect.top + m_paddingY;
			rectText.left =  pCell->m_rect.left + m_paddingX;
			rectText.right =   pCell->m_rect.right +  m_paddingX;
			rectText.bottom = pCell->m_rect.bottom + m_paddingY;

            if (pCell->m_nCellValue == 0)
            {
                pointStart.x += (pCell->m_rect.Width() - pCell->m_pImageDark->GetWidth())/2;
                pointStart.y = pCell->m_rect.bottom + m_paddingY - pCell->m_pImageDark->GetHeight();
                pCell->m_pImageDark->Draw(*pDC, pointStart);
                rectText.bottom -= pCell->m_pImageDark->GetHeight();
            }
            else
            {
                pointStart.x += (pCell->m_rect.Width() - pCell->m_pImageLight->GetWidth())/2;
                pointStart.y =  pCell->m_rect.bottom + m_paddingY - pCell->m_pImageLight->GetHeight();
                pCell->m_pImageLight->Draw(*pDC, pointStart);

                rectText.bottom -= pCell->m_pImageLight->GetHeight();
            }
			CFont* 	pOldFont;
			pOldFont = pDC->SelectObject(&m_defaultFont);

            pDC->DrawText(pCell->m_csPrompt, &rectText, pCell->m_nAlign);

			pDC->SelectObject(pOldFont);
        }
        else
        {
            DrawButtonCell(pDC, pCell->m_rect, 0, pCell->m_nCellValue);

            //
            //vertex[0].x     = pCell->m_rect.left;
            //vertex[0].y     = pCell->m_rect.top;

            //vertex[1].x     = pCell->m_rect.right;
            //vertex[1].y     = pCell->m_rect.bottom; 

            //if ( pCell->m_nCellValue == 0)
            //{
            //    vertex[0].Red   = 0xC000;
            //    vertex[0].Green = 0xC000;
            //    vertex[0].Blue  = 0xC000;
            //    vertex[0].Alpha = 0x0000;

            //    vertex[1].Red   = 0x8000;
            //    vertex[1].Green = 0x8000;
            //    vertex[1].Blue  = 0x8000;
            //    vertex[1].Alpha = 0x0000;

            //    pDC->SetTextColor(RGB(232,232,232));
            //}
            //else
            //{
            //    vertex[0].Red   = 0xE000;
            //    vertex[0].Green = 0x0000;
            //    vertex[0].Blue  = 0x0000;
            //    vertex[0].Alpha = 0x0000;

            //    vertex[1].Red   = 0x6000;
            //    vertex[1].Green = 0x0000;
            //    vertex[1].Blue  = 0x0000;
            //    vertex[1].Alpha = 0x0000;

            //    pDC->SetTextColor(RGB(255,255,255));
            //}

            //pDC->GradientFill(vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);

            //pDC->Draw3dRect(pCell->m_rect, dwHiLight, dwShadow);
            //pDC->FillEllipse(
            //HDC memDC = CreateCompatibleDC(*pDC);
            //Graphics graphDraw(pDC);

            //pDC->Ellipse(&pCell->m_rect);

            //pDC->DrawText(pCell->m_csPrompt, &(pCell->m_rect), pCell->m_nAlign);

            pDC->SetTextColor(RGB(0,0,0));

			//加上padding
			CRect rectText;
			rectText.top =  pCell->m_rect.top + m_paddingY;
			rectText.left =  pCell->m_rect.left + m_paddingX;
			rectText.right =   pCell->m_rect.right +  m_paddingX;
			rectText.bottom = pCell->m_rect.bottom + m_paddingY - 48;

			CFont* 	pOldFont;
			pOldFont = pDC->SelectObject(&m_defaultFont);

	        pDC->DrawText(pCell->m_csPrompt, &rectText, pCell->m_nAlign);

			pDC->SelectObject(pOldFont);
            pDC->SetTextColor(clrOrg);
            
        }
	}
	else //Word Device
	{
        pDC->SetBkMode(OPAQUE);

        //CBrush brushBK;
        //brushBK.CreateSolidBrush(RGB(232,232,232));
        //pDC->FillRect(pCell->m_rect, &brushBK);
        //pDC->FillRect(pCell->m_rect);

		//pDC->Draw3dRect(pCell->m_rect, dwHiLight, dwShadow);
        pDC->SetBkMode(TRANSPARENT);

		//CRect rectTmp = pCell->m_rect;
		//rectTmp.bottom = rectTmp.top + rectTmp.Height() - 48;
			CRect rectTmp;
			rectTmp.top =  pCell->m_rect.top + m_paddingY;
			rectTmp.left =  pCell->m_rect.left + m_paddingX;
			rectTmp.right =   pCell->m_rect.right +  m_paddingX;
			rectTmp.bottom = pCell->m_rect.bottom + m_paddingY - 48;


		CFont* 	pOldFont;
		pOldFont = pDC->SelectObject(&m_defaultFont);

		pDC->DrawText(pCell->m_csPrompt, &rectTmp, pCell->m_nAlign);

		pDC->SelectObject(pOldFont);

        //Draw the content.
        //rectTmp = pCell->m_rect;
        rectTmp.top = rectTmp.bottom + 48;
        //pDC->DrawText(pCell->m_csContent, &rectTmp, pCell->m_nAlign);

        pCell->m_csContent.Format(_T("%d"), pCell->m_nCellValue);

        DrawLEDNumber(pDC, rectTmp, pCell->m_csContent, 5, 0);
	}

    pDC->SetBkMode(nOldBkMode);
}

void CPlcMonitorWnd::DrawCellHot(CDC* pDC, CPlcMonitorCell* pCell)
{
	DWORD dwHiLight = GetSysColor(COLOR_3DHIGHLIGHT);
	DWORD dwShadow = GetSysColor(COLOR_3DSHADOW);
	DWORD dwGrayText = GetSysColor(COLOR_GRAYTEXT);

	CString csTxt;

	pDC->SetBkMode(TRANSPARENT);

	TRIVERTEX vertex[2] ;

	GRADIENT_RECT gRect;
	gRect.UpperLeft  = 0;
	gRect.LowerRight = 1;

    COLORREF clrOrg = pDC->GetTextColor();

	if ( pCell->m_nDeviceUnitType == 0 ) //Bit device
	{
         DrawButtonCell(pDC, pCell->m_rect, 1, pCell->m_nCellValue);
/*
		vertex[0].x     = pCell->m_rect.left;
		vertex[0].y     = pCell->m_rect.top;

		vertex[1].x     = pCell->m_rect.right;
		vertex[1].y     = pCell->m_rect.bottom; 

		if ( pCell->m_nCellValue == 0)
		{
			vertex[0].Red   = 0xF000;
			vertex[0].Green = 0xF000;
			vertex[0].Blue  = 0xF000;
			vertex[0].Alpha = 0x0000;

			vertex[1].Red   = 0x8000;
			vertex[1].Green = 0x8000;
			vertex[1].Blue  = 0x8000;
			vertex[1].Alpha = 0x0000;

			//pDC->SetTextColor(RGB(232,232,232));
			pDC->SetTextColor(RGB(232,0,0));
		}
		else
		{
			vertex[0].Red   = 0xFFF0;
			vertex[0].Green = 0x0000;
			vertex[0].Blue  = 0x0000;
			vertex[0].Alpha = 0x0000;

			vertex[1].Red   = 0xA000;
			vertex[1].Green = 0x0000;
			vertex[1].Blue  = 0x0000;
			vertex[1].Alpha = 0x0000;

			pDC->SetTextColor(RGB(255,255,255));
		}

		pDC->GradientFill(vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);

		pDC->Draw3dRect(pCell->m_rect,dwHiLight, dwShadow);

		pDC->DrawText(pCell->m_csPrompt, &(pCell->m_rect), pCell->m_nAlign);

        pDC->SetTextColor(clrOrg);
*/
            pDC->SetTextColor(RGB(0,0,0));
            CRect rectText;
			//= pCell->m_rect;
            //rectText.bottom -= 48;

			rectText.top =  pCell->m_rect.top + m_paddingY;
			rectText.left =  pCell->m_rect.left + m_paddingX;
			rectText.right =   pCell->m_rect.right +  m_paddingX;
			rectText.bottom = pCell->m_rect.bottom + m_paddingY - 48;

			CFont* 	pOldFont;
			pOldFont = pDC->SelectObject(&m_defaultFont);

			pDC->DrawText(pCell->m_csPrompt, &rectText, pCell->m_nAlign);

			pDC->SelectObject(pOldFont);
	}
	else //Word Device
	{
  //      pDC->SetBkMode(OPAQUE);
		//pDC->Draw3dRect(pCell->m_rect,dwHiLight, dwShadow);
  //      pDC->SetBkMode(TRANSPARENT);

		//CRect rectTmp = pCell->m_rect;
		//rectTmp.bottom = rectTmp.top + rectTmp.Height() / 2;
		//pDC->DrawText(pCell->m_csPrompt, &rectTmp, pCell->m_nAlign);
  //      //Draw the content.
  //      rectTmp = pCell->m_rect;
  //      rectTmp.top = rectTmp.bottom - rectTmp.Height() / 2;
  //      pDC->DrawText(pCell->m_csContent, &rectTmp, pCell->m_nAlign);
	}
}



void CPlcMonitorWnd::OnMouseHover(UINT nFlags, CPoint point)
{
	//TRACE(_T("OnMouseHover\n"));

	CWnd::OnMouseHover(nFlags, point);
}

void CPlcMonitorWnd::OnMouseLeave()
{
	//TRACE(_T("OnMouseLeave\n"));

	if ( m_pCurCellUnderMouse != NULL && m_pCurCellUnderMouse->m_nCellType == 1 )
	{
		CDC* pDC = GetDC();

		DrawCell(pDC, m_pCurCellUnderMouse);

		ReleaseDC(pDC);
	}

	m_pCurCellUnderMouse = NULL;

	CWnd::OnMouseLeave();
}

bool CPlcMonitorWnd::InitPlcMonitorTimer(DWORD dwPeriod)
{
	bool bResult = false;

	//m_mmTimerEvent = timeSetEvent(uDelay, , 0, TIME_PERIODIC | TIME_CALLBACK_FUNCTION );
	bResult = (::CreateTimerQueueTimer(&m_hPlcMonitorTimer, NULL, PlcMonitorTimerProc, this, 0, dwPeriod, WT_EXECUTEINTIMERTHREAD) != 0 );

	return bResult;
}

void CPlcMonitorWnd::DestroyPlcMonitorTimer(void)
{
	//EnterCriticalSection(&m_sectionMonitorWnd);

	if ( ::DeleteTimerQueueTimer(NULL, m_hPlcMonitorTimer, NULL) == 0 )
	{
		DWORD dwErrCode = GetLastError();
		if ( dwErrCode != ERROR_IO_PENDING )
		{
			::DeleteTimerQueueTimer(NULL, m_hPlcMonitorTimer, NULL);
		}
	}
	

	if ( m_hPlcMonitorTimer != INVALID_HANDLE_VALUE )
	{
		//CloseHandle(m_hPlcMonitorTimer);
		m_hPlcMonitorTimer = INVALID_HANDLE_VALUE;
	}

	//LeaveCriticalSection(&m_sectionMonitorWnd);
}

void CPlcMonitorWnd::ConstructTaskVector(void)
{
	int nSize = m_vecMonitorCells.size();

	if ( nSize > 0 )
	{
		vector<CPlcMonitorCell*>::iterator itMonCell;

		for ( itMonCell = m_vecMonitorCells.begin(); itMonCell != m_vecMonitorCells.end(); itMonCell ++ )
		{
			CPlcMonitorCell* pMonCell = *itMonCell;
			//bool bExist = false;

   //         
			////根据每一个CELL的地址，计算出基地址，再在基地址在已经存在的表中查找，偏差在一定范围内的合并。
			//vector<CPlcMonitorTaskItem*>::iterator itMonTaskItem = m_vecTaskItem.begin();
			//CPlcMonitorTaskItem* pMonTaskItem = NULL;

			//for ( ; itMonTaskItem != m_vecTaskItem.end(); itMonTaskItem ++ )
			//{
			//	pMonTaskItem = *itMonTaskItem;
			//	if ( pMonTaskItem->m_strStartAdd == pMonCell->m_strDeviceUnitAdd )
			//	{
			//		bExist = true;
			//		break;
			//	}
			//}
            
			//if (!bExist )
			{
				//
				CPlcMonitorTaskItem* pNewTaskItem = new CPlcMonitorTaskItem;

                char szBuf[50];
                if (CFXPlc::GetDeviceRWStartAddress(pMonCell->m_strDeviceUnitAdd.c_str(), szBuf))
                {
                    pNewTaskItem->m_strStartAdd = szBuf;
                }
                else
                {
                    pNewTaskItem->m_strStartAdd = pMonCell->m_strDeviceUnitAdd;
                }

				pNewTaskItem->m_nDataLen = pMonCell->m_nDeviceNum;

				pNewTaskItem->m_vecCell.push_back(pMonCell);

				m_vecTaskItem.push_back(pNewTaskItem);
			}
			//else
			//{
			//	//Merge into the list
			//	pMonTaskItem->m_vecCell.push_back(pMonCell);

			//}
		}
	}
}

bool CPlcMonitorWnd::ConstructEnqiryTaskList(void)
{
	//if ( GetGlobal()->m_systemPara.bPlcValid && m_nRunningEnqTaskNum <= 0 )
	if ( m_nRunningEnqTaskNum <= 0 )
	{
		int nEnqTaskNum = 	m_vecTaskItem.size();

		if ( nEnqTaskNum > 0 )
		{
			//To decide whether the last batch finished.
			EnterCriticalSection(&m_sectionMonitorWnd);

			TRACE(_T("++++ ConstructEnqiryTaskList ++++\n"));

			vector<CPlcMonitorTaskItem*>::iterator itPlcMonTaskItem; 

			int i = 0;
			for ( itPlcMonTaskItem = m_vecTaskItem.begin(); itPlcMonTaskItem != m_vecTaskItem.end(); ++itPlcMonTaskItem )
			{
				CPlcMonitorTaskItem* pPlcMonTaskItem = (*itPlcMonTaskItem);
				GetGlobal()->m_serialTask->CreateFxPlcNoProtocolReadTask((char*)pPlcMonTaskItem->m_strStartAdd.c_str(), pPlcMonTaskItem->m_nDataLen, this, SERAIL_TASK_MODE_PERIODICAL, (void*)pPlcMonTaskItem);
			}
			
			//GetGlobal()->m_serialTask->CreateFxPlcNoProtocolReadTask("M0000", 32, this);
			//GetGlobal()->m_serialTask->CreateFxPlcNoProtocolReadTask("D0000", 8, this);
			//GetGlobal()->m_serialTask->CreateFxPlcNoProtocolReadTask("Y0000", 8, this);

			m_nRunningEnqTaskNum = nEnqTaskNum;

			LeaveCriticalSection(&m_sectionMonitorWnd);
		}
	}

	return true;
}

void CALLBACK CPlcMonitorWnd::PlcMonitorTimerProc(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
 	CPlcMonitorWnd* pMonitorWnd = (CPlcMonitorWnd*) lpParameter;
	pMonitorWnd->ConstructEnqiryTaskList();
}

LRESULT CPlcMonitorWnd::OnSerialTaskFinish(WPARAM wp, LPARAM lp)
{
    if (m_bWndClosing)
        return 0;

	CSerialTaskItem* pCurTask = (CSerialTaskItem*)lp;
	if ( pCurTask != NULL )
	{
		char chBuf[2048 + 1];
		char* pTemp = pCurTask->GetValue();
		int nLen = strlen(pTemp);
		strcpy( chBuf, pTemp);

		if ( pCurTask->GetTaskMode() == SERAIL_TASK_MODE_PERIODICAL )
		{
			if ( pCurTask->GetTaskType() == 0 ) //Read Task
			{
				char* pAdd = pCurTask->GetAddress();
                TRACE("Periodical Task: The device address is:%s, Device num is %d\n", pAdd, pCurTask->GetDeviceNum());

				pCurTask->GetDeviceNum();

                CPlcMonitorTaskItem* pMonitorItem = reinterpret_cast<CPlcMonitorTaskItem*>(pCurTask->GetOwnerCtrlItem());

                if (pMonitorItem != NULL)
                {
                    pMonitorItem->m_strResult = chBuf;
                }

				//更新界面: 与初始值比较，如不相同则更新值并重画UI
                PostMessage(WM_UPDATE_PLCMON_CELL, (WPARAM)pMonitorItem);
				
				EnterCriticalSection(&m_sectionMonitorWnd);

				m_nRunningEnqTaskNum --;

				TRACE("Now the running enq task num is %d\n", m_nRunningEnqTaskNum);

				LeaveCriticalSection(&m_sectionMonitorWnd);

			}
		}
		else
		{
			//UI 命令:在所有可能OUT的控件查找地址相当的UI控件。
			char* pAdd = pCurTask->GetAddress();
			CPlcMonitorCell* pPlcMonCell = FindOutputBitCell(pAdd);

			if ( pPlcMonCell != NULL )
			{
				//TRACE(_T("****************Run Panel commond finished**************** command address：%s\n"), pAdd);
                TRACE(_T("****************Run Panel commond finished****************\n"));
			}

		}
	}
	return 0;
}

LRESULT CPlcMonitorWnd::OnUpdateMonitorCell(WPARAM wp, LPARAM lp)
{
    if (m_bWndClosing)
        return 0;

    CPlcMonitorTaskItem* pMonitorItem = reinterpret_cast<CPlcMonitorTaskItem*>(wp);

    if (pMonitorItem != NULL && pMonitorItem->m_vecCell.size() > 0)
    {
        //Todo: Now only first one is valid.
        CPlcMonitorCell* pPlcMonCell = pMonitorItem->m_vecCell[0];

        if ( pPlcMonCell != NULL )
        {
            char szBuf[50];

            if (CFXPlc::ParseFxPlcRevResult(pMonitorItem->m_strResult.c_str(),
                pMonitorItem->m_strStartAdd.c_str(),
                pPlcMonCell->m_strDeviceUnitAdd.c_str(),
                pPlcMonCell->m_nDeviceNum,
                szBuf,
                20,
                CFXPlc::m_bCheckSum))
            {
                int nNewValue = 0;

                int nLen = strlen(szBuf);

                for (int i = 0; i < nLen; i ++)
                {
                    if (szBuf[i] > '9')
                    {
                        nNewValue = (nNewValue << 4) + (szBuf[i] - 'A' + 10);
                    }
                    else
                    {
                        nNewValue = (nNewValue << 4) + (szBuf[i] - '0');
                    }
                }

                if (pPlcMonCell->m_nCellValue != nNewValue)
                {
                    pPlcMonCell->m_nCellValue = nNewValue;

                    pPlcMonCell->FormatValueString();

                    CDC* pDC = GetDC();

                    //TRACE(_T("-----------------DrawCell in OnUpdateMonitorCell: the new value is %d\n"), nNewValue);

                    DrawCell(pDC, pPlcMonCell);

                    ReleaseDC(pDC);
                }
            }
        }
    }

	return 0;
}


bool CPlcMonitorWnd::LoadAllResource()
{
    //Load PNG resource
    LoadImage(IDB_PNG_BALL_GRAY, _T("PNG"), m_imageBallGray);
    LoadImage(IDB_PNG_BALL_RED, _T("PNG"),  m_imageBallRed);
    LoadImage(IDB_PNG_BALL_GREEN, _T("PNG"), m_imageBallGreen);
    LoadImage(IDB_PNG_BALL_ORANGE, _T("PNG"), m_imageBallOrange);

    //Load 7 segmented LED source
	CDC* pDC = GetDC();

	if ( m_cdcBufLEDPane.bValid == FALSE )
	{
		m_bmPane.LoadBitmap( IDB_LED_NUMBER_PANE );

		m_cdcBufLEDPane.dcMemory.CreateCompatibleDC( pDC );
		m_cdcBufLEDPane.pOldBmp = m_cdcBufLEDPane.dcMemory.SelectObject( &m_bmPane );
		m_cdcBufLEDPane.bValid = true;
	}

	if ( m_cdcBufNumber.bValid == FALSE )
	{
		m_bmNumber.LoadBitmap( IDB_LED_NUMBER );

		BITMAP bmTemp;
		m_bmNumber.GetBitmap( &bmTemp );
		
		m_nDigitWidth		= (short)(bmTemp.bmWidth / 11);
		m_nDigitHeight		= (short)bmTemp.bmHeight;

		m_cdcBufNumber.dcMemory.CreateCompatibleDC( pDC );
		m_cdcBufNumber.pOldBmp = m_cdcBufNumber.dcMemory.SelectObject( &m_bmNumber );
		m_cdcBufNumber.bValid = true;
	}

	if ( m_cdcBufNumberDot.bValid == FALSE )
	{
		m_bmNumberDot.LoadBitmap( IDB_LED_NUMBER_DOT );
		m_cdcBufNumberDot.dcMemory.CreateCompatibleDC( pDC );
		m_cdcBufNumberDot.pOldBmp = m_cdcBufNumberDot.dcMemory.SelectObject( &m_bmNumberDot );
		m_cdcBufNumberDot.bValid = true;
	}

    m_bmNull.LoadBitmap( IDB_LED_NULL);

    ReleaseDC(pDC);

    return true;
}

void CPlcMonitorWnd::ReleaseAllResource()
{
    m_imageBallGray.Destroy();
    m_imageBallRed.Destroy();
    m_imageBallGreen.Destroy();
    m_imageBallOrange.Destroy();

	if ( m_cdcBufLEDPane.bValid )
	{
		m_cdcBufLEDPane.dcMemory.SelectObject( m_cdcBufLEDPane.pOldBmp );
        m_cdcBufLEDPane.dcMemory.DeleteDC();
	}
	if ( m_cdcBufNumber.bValid )
	{
		m_cdcBufNumber.dcMemory.SelectObject( m_cdcBufNumber.pOldBmp );
        m_cdcBufNumber.dcMemory.DeleteDC();
	}
	if ( m_cdcBufNumberDot.bValid )
	{
		m_cdcBufNumberDot.dcMemory.SelectObject( m_cdcBufNumberDot.pOldBmp );
        m_cdcBufNumberDot.dcMemory.DeleteDC();
	}

    m_bmPane.DeleteObject();
	m_bmPane.Detach();
	m_bmNumber.DeleteObject();
	m_bmNumber.Detach();
	m_bmNumberDot.DeleteObject();
	m_bmNumberDot.Detach();

    m_bmNull.DeleteObject();
    m_bmNull.Detach();

	m_defaultFont.DeleteObject();
}



bool CPlcMonitorWnd::LoadImage(WORD imageID, LPCTSTR lpType, CImage& image)
{
    HRSRC hRsrc = ::FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(imageID), lpType);
    if(hRsrc!=NULL)
    {
        HGLOBAL hImgData = ::LoadResource(AfxGetResourceHandle(),hRsrc);
        if(hImgData == NULL)
        {
            ::FreeResource(hImgData);
            return false;
        }
        else
        {
            LPVOID lpVoid = ::LockResource(hImgData);
            LPSTREAM pStream=NULL;
            DWORD dwSize = ::SizeofResource(AfxGetResourceHandle(), hRsrc);
            HGLOBAL hNew = ::GlobalAlloc(GHND, dwSize);
            LPBYTE lpByte = (LPBYTE)::GlobalLock(hNew);
            ::memcpy(lpByte, lpVoid, dwSize);
            GlobalUnlock(hNew);
            HRESULT ht = CreateStreamOnHGlobal(hNew, TRUE, &pStream);
            if( ht != S_OK )
            {
                GlobalFree(hNew);
                return false;
            }
            else
            {
                image.Load(pStream);
                GlobalFree(hNew);
            }
            ::FreeResource(hImgData);    
        }
    }
    return true;
}

void CPlcMonitorWnd::DrawLEDNumber(CDC* pDC, CPoint& pointStart, CString csNumber, short nDigit /* = 8*/, short nDigitFraction/* = 2*/, bool bValid/* = true*/, short nStyle /* = 0*/, short nXSpace/* = 2*/, short nYSpace/* = 2*/)
{
	CDC cdcTemp;
	//CBitmap nullTemp;
	//nullTemp.LoadBitmap( IDB_LED_NULL );

	cdcTemp.CreateCompatibleDC(pDC);
	CBitmap* pOldBmp = cdcTemp.SelectObject( &m_bmNull );

	int i = 0;

	for ( i = 0; i < nDigit; i ++ )
		cdcTemp.BitBlt( i * m_nDigitWidth, 0, m_nDigitWidth, m_nDigitHeight, &m_cdcBufLEDPane.dcMemory, 0, 0, SRCCOPY );

	if ( bValid )
	{
		int nDotPos = csNumber.Find('.');
		int nDigitBit = 0;
		int nLen = csNumber.GetLength();

		if ( nDotPos != -1 )
		{
			if ( nDotPos > 0 ) 
			{
				if ( nDotPos > 1 )
				{
					for ( i = 0; i < nDotPos - 1; i ++ )
					{
						if ( csNumber[i] == '-' )
						{
							cdcTemp.BitBlt( m_nDigitWidth * (nDigit - nLen + 1 + i ), 0, m_nDigitWidth, m_nDigitHeight, &m_cdcBufNumber.dcMemory, m_nDigitWidth * 10, 0, SRCCOPY );
						}
						else
						{
							nDigitBit = csNumber[i] - 48;

							cdcTemp.BitBlt( m_nDigitWidth * (nDigit - nLen + 1 + i ), 0, m_nDigitWidth, m_nDigitHeight, &m_cdcBufNumber.dcMemory, m_nDigitWidth * nDigitBit, 0, SRCCOPY );
						}
					}

					nDigitBit = csNumber[i] - 48;
					cdcTemp.BitBlt( m_nDigitWidth * (nDigit - nLen + 1 + i ), 0, m_nDigitWidth, m_nDigitHeight, &m_cdcBufNumberDot.dcMemory, m_nDigitWidth * nDigitBit, 0, SRCCOPY );

					for ( i = nDotPos + 1; i < nLen; i ++ )
					{
						nDigitBit = csNumber[i] - 48;
						cdcTemp.BitBlt( m_nDigitWidth * (nDigit - nLen + i ), 0, m_nDigitWidth, m_nDigitHeight, &m_cdcBufNumber.dcMemory, m_nDigitWidth * nDigitBit, 0, SRCCOPY );
					}
				}
				else
				{
					nDigitBit = csNumber[0] - 48;
					cdcTemp.BitBlt( m_nDigitWidth * (nDigit - nLen + 1 ), 0, m_nDigitWidth, m_nDigitHeight, &m_cdcBufNumberDot.dcMemory, m_nDigitWidth * nDigitBit, 0, SRCCOPY );
					for ( i = nDotPos + 1; i < nLen; i ++ )
					{
						nDigitBit = csNumber[i] - 48;
						cdcTemp.BitBlt( m_nDigitWidth * (nDigit - nLen + i ), 0, m_nDigitWidth, m_nDigitHeight, &m_cdcBufNumber.dcMemory, m_nDigitWidth * nDigitBit, 0, SRCCOPY );
					}
				}
			}
			else
			{
				cdcTemp.BitBlt( m_nDigitWidth * (nDigit - nLen ), 0, m_nDigitWidth, m_nDigitHeight, &m_cdcBufNumberDot.dcMemory, 0, 0, SRCCOPY );
				for ( i = 1; i < nLen; i ++ )
				{
					nDigitBit = csNumber[i] - 48;
					cdcTemp.BitBlt( m_nDigitWidth * (nDigit - nLen + i ), 0, m_nDigitWidth, m_nDigitHeight, &m_cdcBufNumber.dcMemory, m_nDigitWidth * nDigitBit, 0, SRCCOPY );
				}
			}
			if ( nDigitFraction > 0 )
			{
			}
			else
			{

			}
		}
		else
		{
			for ( i = 0; i < nLen; i ++ )
			{
				if ( csNumber[i] == '-' )
				{
					cdcTemp.BitBlt( m_nDigitWidth * (nDigit - nLen + i ), 0, m_nDigitWidth, m_nDigitHeight, &m_cdcBufNumber.dcMemory, m_nDigitWidth * 10, 0, SRCCOPY );
				}
				else
				{
					nDigitBit = csNumber[i] - 48;

					cdcTemp.BitBlt( m_nDigitWidth * (nDigit - nLen + i ), 0, m_nDigitWidth, m_nDigitHeight, &m_cdcBufNumber.dcMemory, m_nDigitWidth * nDigitBit, 0, SRCCOPY );
				}
			}
		}
	}
	
	pDC->BitBlt(pointStart.x + nXSpace, pointStart.y + nYSpace, m_nDigitWidth * nDigit, m_nDigitHeight, &cdcTemp, 0,0, SRCCOPY );

	cdcTemp.SelectObject(pOldBmp);

	CRect rectWnd( nXSpace, nYSpace, nXSpace + nDigit * m_nDigitWidth , nYSpace + m_nDigitHeight );

	rectWnd.InflateRect(1,1);

    rectWnd.OffsetRect(pointStart);

	pDC->Draw3dRect( &rectWnd, GetSysColor(COLOR_3DDKSHADOW), GetSysColor(COLOR_3DFACE));

	rectWnd.InflateRect(1,1);

	pDC->Draw3dRect( &rectWnd, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DHILIGHT));

	//ReleaseDC(pDC);
}

void CPlcMonitorWnd::DrawLEDNumber(CDC* pDC, CRect& rectDst, CString csNumber, short nDigit, short nDigitFraction, bool bValid, short nStyle, short nXSpace, short nYSpace)
{
    CPoint pointStart = CPoint(rectDst.left, rectDst.top);
    int cx = m_nDigitWidth * nDigit;
    int cy = m_nDigitHeight;
    pointStart.x += (rectDst.Width() - cx) / 2;
    pointStart.y += (rectDst.Height() - cy) / 2;

    DrawLEDNumber(pDC, pointStart, csNumber, nDigit, nDigitFraction, bValid, nStyle, nXSpace, nYSpace);
}
