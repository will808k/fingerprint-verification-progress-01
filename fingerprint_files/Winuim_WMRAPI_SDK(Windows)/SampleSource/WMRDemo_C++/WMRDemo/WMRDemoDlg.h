
// WMRDemoDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "StretchImage.h"

// CWMRDemoDlg 对话框
class CWMRDemoDlg : public CDialog
{
// 构造
public:
	CWMRDemoDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_WMRDEMO_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButOpen();
	afx_msg void OnBnClickedButClose();
	afx_msg void OnBnClickedButEnroll();
	afx_msg void OnBnClickedButStopenroll();
	afx_msg void OnBnClickedButVerify();
	afx_msg void OnBnClickedButStopverify();
	void ShowMessage(CString str);
	HBITMAP BMPDataBufferToHBITMAP(char * m_pBMPBuffer);
	int DisplayBmpImg(unsigned char* BmpImg,int width,int height);
	CEdit m_edit;
	CString m_showMessage;
	CString m_strtmpstr;
	HANDLE m_DevHandle;
	unsigned char DeviceSN[64];
	bool m_bOpened;
	HANDLE   m_hThread;
	BOOL hThreadTerm;
	unsigned char ImageBuf[160000];
	unsigned char BmpImageBuf[160000];
	int GetFeature();
	int ImageWidth;
	int ImageHeight;
	unsigned char *pFeatureArray[3];
	unsigned char pTemplate[1024];
	unsigned char pFeature[1024];
	int m_CaptureType;
	int m_bFinished;
	int m_CapNum;
	bool m_bGetFeature;

	afx_msg void OnDestroy();
	CStatic m_img;
	afx_msg void OnBnClickedButBeep();
	afx_msg void OnBnClickedButBmp2fea();
	afx_msg void OnBnClickedButSavebmp();
	afx_msg void OnBnClickedButExit();
	CStretchImage m_StretchImage;
};
