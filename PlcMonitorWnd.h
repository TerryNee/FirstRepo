#pragma once
#include "afxwin.h"

#include "atlimage.h"

#define PLC_MONITOR_WND_CLASSNAME _T("PlcMinitorWnd")

#include <vector>
#include <string>
#include <map>
using namespace std;

#define DEVICE_UNIT_ADD_LEN  (5+1)
#define PROMPT_STRING_LEN	 (30)

class CPlcMonitorCell
{
public:
	CPlcMonitorCell();
	virtual ~CPlcMonitorCell();

	void SetDeviceUnitAdd(string strUnitAdd);

	bool LoadCellImage(LPCTSTR szLightFile, LPCTSTR szDarkFile, LPCSTR szBlackFile);

public:
	CRect m_rect;						//Size and the position of the Cell

	string m_strDeviceUnitAdd;			//The address of the device

	enum DeviceUnitType
	{
		DeviceUnitBit = 0,
		DeviceUnitWord
	} m_nDeviceUnitType;				//The type of the device unit: Bit or Word

	enum CellType
	{
		CellTypeReadonly = 0,
		CellTypeRW
	} m_nCellType;	    				//0 - Readonly, read/write

	enum CellStatus
	{
		CellStatusNormal = 0,
		CellStatusPushdown,
		CellStatusPushup,
		CellStatusDisable
	} m_nCellStatus;					//0 - normal, 1 - push off, 2 - push up, 3 - disable

	int m_nCellValue;					//0 - 1

	enum AccessStatus
	{
		StatusIdle = 0,
		StatusReading,
		StatusWriting,
		StatusReadingFin,
		StatusWritingFin,
		StatusError
	} m_accessStatus;

	CString m_csPrompt;					//The default is Address
	int m_nAlign;						//Same with DT_xxxx
	CString m_csDigitFormat;			//0 - decimal, 1 - hex, 2 - octal, 3 - binary
	int m_nDeviceNum;					//数据位数

	bool m_bMouseOnCell;				//

	COLORREF m_colorTxtLight;			//The color of the Prompt
	COLORREF m_colorTxtDark;

	CImage* m_pImageLight;
	CImage* m_pImageDark;
	CImage* m_pImageBlack;

public:
    CString m_csContent;                   //缓冲区：存放格式化值的字符串
    void FormatValueString();           //将结果值按照m_csDigitFormat写成字符串
};

class CPlcMonitorTaskItem
{
public:
	CPlcMonitorTaskItem();
	~CPlcMonitorTaskItem();

public:
	enum PlcTaskAction
	{
		PlcTaskRead = 0,
		PlcTaskWrite
	}m_taskAction;

	enum PlcDataType
	{
		PlcDataBit = 0,
		PlcDataWord
	} m_plcDataType;

	string m_strStartAdd;
	int m_nDataLen;

	string m_strResult;

	vector<CPlcMonitorCell*> m_vecCell;
};

class CPlcMonitorWnd : public CWnd
{
public:
	CPlcMonitorWnd(void);
	~CPlcMonitorWnd(void);

	void DestroyObjects(void);

public:
	CRITICAL_SECTION m_sectionMonitorWnd;

    bool m_bWndClosing;

	HANDLE m_hPlcMonitorTimer; //Queue Timers

	CPlcMonitorCell* m_pCurCellUnderMouse;

	vector<CPlcMonitorCell*> m_vecMonitorCells;
	map<string, CPlcMonitorCell*> m_mapAddCells;

	vector<CPlcMonitorTaskItem*> m_vecTaskItem;
	int m_nRunningEnqTaskNum;      //查询子程序信号量

	void AutoConstructCells(void);		//Auto contruct base the PLC

	int ConstructCells(void);         //Construct Cell list from the Config file.

	void ConstructMap(void);
	void ConstructTaskVector(void);   //Construct Task vector m_vecTaskItem base-on all cells (m_vecMonitorCells)

	void AddMonitorCell(CPlcMonitorCell* pNewCell);

	CRect CalculateCellsDimension(void);

	CPlcMonitorCell* GetCurrentCell(CPoint point);
	CPlcMonitorCell* FindCell(string strUnitAdd);
	CPlcMonitorCell* FindCell(char* pUnitAdd);
	CPlcMonitorCell* FindOutputBitCell(char* pUnitAdd);

	void UpdateCell(CPlcMonitorCell* pCell);
	void DrawCell(CDC* pDC, CPlcMonitorCell* pCell);
	void DrawCellHot(CDC* pDC, CPlcMonitorCell* pCell);

	bool InitPlcMonitorTimer(DWORD dwPeriod);
	void DestroyPlcMonitorTimer(void);

	bool ConstructEnqiryTaskList(void);
	static void CALLBACK PlcMonitorTimerProc(PVOID lpParameter, BOOLEAN TimerOrWaitFired);

public:
    bool LoadAllResource();
    void ReleaseAllResource();

    bool LoadImage(WORD imageID, LPCTSTR lpType, CImage& image);

    CImage m_imageBallGray;
    CImage m_imageBallRed;
    CImage m_imageBallGreen;
    CImage m_imageBallOrange;

public:
    CImage* GetImage(UINT imageUid);

    //7段数字资源
    short m_nDigitWidth;
    short m_nDigitHeight;

  	typedef struct _STRUCT_CDCBUFFER
	{
		BOOL		bValid;
		CDC			dcMemory;
		CBitmap*	pOldBmp;

		_STRUCT_CDCBUFFER::_STRUCT_CDCBUFFER()
		{bValid = FALSE;}
	} STRUCT_CDCBUFFER;

	STRUCT_CDCBUFFER m_cdcBufLEDPane;
	STRUCT_CDCBUFFER m_cdcBufNumber;
	STRUCT_CDCBUFFER m_cdcBufNumberDot;

    CBitmap     m_bmPane;
	CBitmap		m_bmNumber;
	CBitmap		m_bmNumberDot;
    CBitmap     m_bmNull;

    void DrawLEDNumber(CDC* pDC, CPoint& pointStart, CString csNumber, short nDigit = 8, short nDigitFraction = 2, bool bValid = true, short nStyle = 0, short nXSpace = 2, short nYSpace = 2);
    void DrawLEDNumber(CDC* pDC, CRect& rectDst, CString csNumber, short nDigit = 8, short nDigitFraction = 2, bool bValid = true, short nStyle = 0, short nXSpace = 2, short nYSpace = 2);

    //使用GDI+画按键
    void DrawButtonCell(CDC* pDC, CRect& rectDst, bool bActive, int nColorId, int nWidth = 48, int nHeight = 48);

	CFont m_defaultFont;
	int m_paddingX;
	int m_paddingY;
	int m_contentWidth;
	int m_contentHeight;

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    BOOL RegisterWindowClass();
public:
	virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnEditClear();
	afx_msg void OnEditCopy();
	afx_msg void OnEditCut();
	afx_msg void OnViewOutput();

	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
    //afx_msg LRESULT OnSetFont(WPARAM, LPARAM);
    //afx_msg LRESULT OnGetFont(WPARAM, LPARAM);

	afx_msg LRESULT OnSerialTaskFinish(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnUpdateMonitorCell(WPARAM wp, LPARAM lp);
};

