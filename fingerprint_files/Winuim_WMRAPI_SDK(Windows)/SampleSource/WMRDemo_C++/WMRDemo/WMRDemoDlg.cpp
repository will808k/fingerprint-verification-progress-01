
// WMRDemoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "WMRDemo.h"
#include "WMRDemoDlg.h"
#include "WMRAPI.h"
#ifdef _WIN64
#pragma comment(lib,"x64/WMRAPI.lib")
#else
#pragma comment(lib,"x86/WMRAPI.lib")
#endif


//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CWMRDemoDlg 对话框




CWMRDemoDlg::CWMRDemoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWMRDemoDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_DevHandle = NULL;
	m_bOpened = false;
	m_CaptureType = 0;
	m_bFinished = false;
	m_CapNum = 0;
	m_bGetFeature =false;
	m_hThread = NULL;
	for (int i=0;i<3;i++)
	{
       pFeatureArray[i] =(BYTE*)malloc(400*400) ;
	   memset(pFeatureArray[i],0,sizeof(pFeatureArray[i]));
	}
}

void CWMRDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_edit);
	DDX_Control(pDX, IDC_IMG, m_img);
}

BEGIN_MESSAGE_MAP(CWMRDemoDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUT_OPEN, &CWMRDemoDlg::OnBnClickedButOpen)
	ON_BN_CLICKED(IDC_BUT_CLOSE, &CWMRDemoDlg::OnBnClickedButClose)
	ON_BN_CLICKED(IDC_BUT_ENROLL, &CWMRDemoDlg::OnBnClickedButEnroll)
	ON_BN_CLICKED(IDC_BUT_STOPENROLL, &CWMRDemoDlg::OnBnClickedButStopenroll)
	ON_BN_CLICKED(IDC_BUT_VERIFY, &CWMRDemoDlg::OnBnClickedButVerify)
	ON_BN_CLICKED(IDC_BUT_STOPVERIFY, &CWMRDemoDlg::OnBnClickedButStopverify)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUT_BEEP, &CWMRDemoDlg::OnBnClickedButBeep)
	ON_BN_CLICKED(IDC_BUT_BMP2FEA, &CWMRDemoDlg::OnBnClickedButBmp2fea)
	ON_BN_CLICKED(IDC_BUT_SAVEBMP, &CWMRDemoDlg::OnBnClickedButSavebmp)
	ON_BN_CLICKED(IDC_BUT_EXIT, &CWMRDemoDlg::OnBnClickedButExit)
END_MESSAGE_MAP()


// CWMRDemoDlg 消息处理程序

BOOL CWMRDemoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CWMRDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CWMRDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CWMRDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

DWORD WINAPI GetFeatureProc(LPVOID para)
{
	OutputDebugString(_T("GetImgProc begin"));
	int gt = 0;
	CWMRDemoDlg* pWMR = (CWMRDemoDlg*)para;
	if (NULL == pWMR)
	{
		return 0;
	}
	pWMR->GetFeature();
	return 0;
}

void CharStr2HexStr(unsigned char const* pucCharStr, char* pszHexStr, int iSize)
{
	int i = 0;
	unsigned char byte[2];
	pszHexStr[0] = 0;
	for(i=0; i<iSize; i++)
	{
		byte[0] = pucCharStr[i] >> 4;
		byte[1] = pucCharStr[i]%16;
		for(int j=0; j<2; j++)
		{
			if(byte[j] >= (unsigned char)0 && byte[j] <= (unsigned char)9)
				pszHexStr[i*2+j] = '0' + byte[j];
			else
				pszHexStr[i*2+j] = 'A' + byte[j] - 10;
		}
	}
}

void CWMRDemoDlg::ShowMessage(CString str)
{
	m_showMessage += str;
	SetDlgItemText(IDC_EDIT1,m_showMessage);
	::SendMessage(m_edit.GetSafeHwnd(),WM_VSCROLL,SB_BOTTOM,0);
}


void CWMRDemoDlg::OnBnClickedButOpen()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_bOpened)
	{
		m_strtmpstr.Format(_T("设备已打开..\r\n"));
		ShowMessage(m_strtmpstr);
		return;
	}
	int nCount = WM_GetDeviceCount();
	if (nCount <= 0)
	{
		m_strtmpstr.Format(_T("当前电脑中可用的设备有 %d 个：\r\n"),0);
		ShowMessage(m_strtmpstr);
		return;

	}
	m_strtmpstr.Format(_T("当前电脑中可用的设备有 %d 个：\r\n"),nCount);
	ShowMessage(m_strtmpstr);
	int nRet = WM_Init();
	if (WM_OK != nRet)
	{
		m_strtmpstr.Format(_T("初始化设备失败！！\r\n"));
		ShowMessage(m_strtmpstr);
		return;
	}
	m_strtmpstr.Format(_T("初始化设备成功..\r\n"));
	ShowMessage(m_strtmpstr);
	nRet = WM_OpenDevice(0,&m_DevHandle);
	if (WM_OK != nRet)
	{
		m_strtmpstr.Format(_T("打开设备失败！！\r\n"));
		ShowMessage(m_strtmpstr);
		return;
	}

	m_bOpened = true;
	m_strtmpstr.Format(_T("打开设备成功..\r\n"));
	ShowMessage(m_strtmpstr);
	memset(DeviceSN,0,64);
	WM_GetSerialNumber(m_DevHandle,DeviceSN);
	m_strtmpstr.Format(_T("序列号：%s\r\n"),DeviceSN);
	ShowMessage(m_strtmpstr);
    WM_GetImageInfo(&ImageWidth,&ImageHeight);
	//ImageBuf = (unsigned char *)malloc(ImageWidth*ImageHeight);
	//BmpImageBuf = (unsigned char *)malloc(ImageWidth*ImageHeight+1078);

	if (NULL==m_hThread)
	{
		LPVOID  lp = (LPVOID)this;
		hThreadTerm=FALSE;
		m_bGetFeature =false;
		m_hThread=CreateThread(NULL,0,GetFeatureProc,this,0,NULL);
	}

}

void CWMRDemoDlg::OnBnClickedButClose()
{
	// TODO: 在此添加控件通知处理程序代码

	//	OnStopGetFeature();

	hThreadTerm=TRUE;
	if(m_hThread!=NULL)
	{
		WaitForSingleObject(m_hThread,INFINITE);
		CloseHandle(m_hThread);
		m_hThread=NULL;
	}

	if (m_bOpened)
	{
		m_bOpened=FALSE;
		int rt=WM_CloseDevice(m_DevHandle);
		if (WM_OK == rt)
		{
			m_DevHandle =NULL;
			m_strtmpstr.Format(_T("关闭设备成功..\r\n"));
			ShowMessage(m_strtmpstr);
			return;
		}
	}
	m_strtmpstr.Format(_T("已关闭设备..\r\n"));
	ShowMessage(m_strtmpstr);   

}
HBITMAP CWMRDemoDlg::BMPDataBufferToHBITMAP(char * m_pBMPBuffer)
{
	HBITMAP             hBmp;   
	LPSTR               hDIB,lpBuffer = m_pBMPBuffer;   
	LPVOID              lpDIBBits;   
	BITMAPFILEHEADER    bmfHeader;   
	DWORD               bmfHeaderLen;   

	bmfHeaderLen = sizeof(bmfHeader);   
	strncpy((LPSTR)&bmfHeader,(LPSTR)lpBuffer,bmfHeaderLen);   
	//if (bmfHeader.bfType != ((WORD) ('M' << 8) | 'B')) return NULL;    
	if (bmfHeader.bfType != (*(WORD*)"BM")) return NULL;//我copy《Windows程序设计》上的做法。    
	hDIB = lpBuffer + bmfHeaderLen;   
	BITMAPINFOHEADER &bmiHeader = *(LPBITMAPINFOHEADER)hDIB ;   
	BITMAPINFO &bmInfo = *(LPBITMAPINFO)hDIB ;   
	lpDIBBits=(lpBuffer)+((BITMAPFILEHEADER *)lpBuffer)->bfOffBits;   
	CClientDC dc(this);   
	hBmp = CreateDIBitmap(dc.m_hDC,&bmiHeader,CBM_INIT,lpDIBBits,&bmInfo,DIB_RGB_COLORS);
	return hBmp;
}

int CWMRDemoDlg::DisplayBmpImg(unsigned char* BmpImg,int width,int height)
{
	HBITMAP hBitmap = BMPDataBufferToHBITMAP((char *)BmpImg);
	HBITMAP hBitmap2 = m_StretchImage.ShrinkBitmap(hBitmap,width/2,height/2);
	
	m_img.SetBitmap(hBitmap2);
	//SendMessage(WM_MYUPDATE,0,0);
	return 0;
}


int CWMRDemoDlg::GetFeature()
{
    memset(ImageBuf,0,sizeof(ImageBuf));
	memset(BmpImageBuf,0,sizeof(BmpImageBuf));
	memset(pTemplate,0,sizeof(pTemplate));
	memset(pFeature,0,sizeof(pFeature));
	int nRet = 0;
	CString FeatureText=_T("");
	DWORD TimeOut = 0;
	int Size = 0;
	while(!hThreadTerm)
	{
		if(m_bGetFeature)
		{
			if (!m_bFinished)
			{
				nRet = WM_GetImage(m_DevHandle,TimeOut,ImageBuf,&Size);
				if (nRet == WM_OK)
				{
					WM_RawToBMP(ImageBuf,ImageWidth,ImageHeight,BmpImageBuf,&Size);
					DisplayBmpImg(BmpImageBuf,ImageWidth,ImageHeight);
				    //CharStr2HexStr((const unsigned char *)Feature,FeatureHex,Size);*/
					if (m_CaptureType == 0) //注册模式
					{
						//nRet = WM_Extract(ImageBuf,ImageWidth,ImageHeight,pFeatureArray[m_CapNum],&Size);
						memset(pFeatureArray[m_CapNum],0,sizeof(pFeatureArray[m_CapNum]));
						memcpy(pFeatureArray[m_CapNum],BmpImageBuf,ImageWidth*ImageHeight+1078);
						if (nRet == WM_OK)
						{
							m_CapNum++;
							m_strtmpstr.Format(_T("第%d次采集成功..\r\n"),m_CapNum);
							ShowMessage(m_strtmpstr); 
							if (m_CapNum > 2)
							{
								m_bFinished = true;
								nRet = WM_GenTemplateWithImage(pFeatureArray,m_CapNum,ImageWidth,ImageHeight,pTemplate,&Size);
								if (nRet == WM_OK)
								{
									m_strtmpstr.Format(_T("合成模板成功，注册成功！\r\n"));
									ShowMessage(m_strtmpstr);
									char pTemplateHex[4096]={0};
									CharStr2HexStr(pTemplate,pTemplateHex,Size);
									CString strTem(pTemplateHex);
									m_strtmpstr.Format(_T("指纹模板数据：%s\r\n"),strTem);
									ShowMessage(m_strtmpstr); 


								}
								else
								{
									m_strtmpstr.Format(_T("合成模板失败，注册失败！\r\n"));
									ShowMessage(m_strtmpstr); 
									m_CapNum = 0;

									//if (m_CapNum <= 2)
									//{
									//	m_strtmpstr.Format(_T("合成模板失败，请再按压一次！\r\n"));
									//	ShowMessage(m_strtmpstr); 
									//}
									//else
									//{
									//	m_strtmpstr.Format(_T("合成模板失败，注册失败！\r\n"));
									//	ShowMessage(m_strtmpstr); 
									//	m_CapNum = 0;
									//}

								}
							}
							//Sleep(500);
						}
						else
						{
							m_strtmpstr.Format(_T("指纹图像质量不佳，请再按压一次..\r\n"));
							ShowMessage(m_strtmpstr); 
						}

					}
					else  //比对模式
					{
						nRet = WM_Extract(BmpImageBuf,ImageWidth,ImageHeight,pFeature,&Size);
						if (nRet == WM_OK)
						{
							char pFeatureHex[4096]={0};
							CharStr2HexStr(pFeature,pFeatureHex,Size);
							CString strTem(pFeatureHex);
							m_strtmpstr.Format(_T("指纹特征数据：%s\r\n"),strTem);
							ShowMessage(m_strtmpstr); 
							m_bFinished = true;
							int score = 0;
                            nRet=WM_Verify(pTemplate,pFeature,&score);
							if (nRet == WM_OK)
							{
								m_strtmpstr.Format(_T("指纹比对成功，分值：%d..\r\n"),score);
							//	m_strtmpstr.Format(_T("指纹比对成功..\r\n"));
								ShowMessage(m_strtmpstr); 
							}
							else if (nRet == WM_VERIFY_FAIL)
							{
								m_strtmpstr.Format(_T("指纹比对失败，分值：%d..\r\n"),score);
								//m_strtmpstr.Format(_T("指纹比对失败..\r\n"));
								ShowMessage(m_strtmpstr); 
							}
							else
							{
								m_strtmpstr.Format(_T("比对失败，错误码：%d..\r\n"),nRet);
								ShowMessage(m_strtmpstr); 
							}
							
						}
						else
						{
							m_strtmpstr.Format(_T("指纹图像质量不佳，请再按压一次..\r\n"));
							ShowMessage(m_strtmpstr); 
						}


					}

				}
			}
		}

	}
	return S_OK;
}

void CWMRDemoDlg::OnBnClickedButEnroll()
{
	// TODO: 在此添加控件通知处理程序代码
	m_bGetFeature =true;
	m_bFinished = false;
	m_CaptureType = 0;
	m_CapNum = 0;
	m_strtmpstr.Format(_T("开始注册，采集指纹三次，合成模板成功即为注册成功..\r\n"));
	ShowMessage(m_strtmpstr); 
	m_strtmpstr.Format(_T("请按压手指..\r\n"));
	ShowMessage(m_strtmpstr); 
}

void CWMRDemoDlg::OnBnClickedButStopenroll()
{
	// TODO: 在此添加控件通知处理程序代码
	m_strtmpstr.Format(_T("停止注册..\r\n"));
	ShowMessage(m_strtmpstr); 
	m_bFinished = true;
	m_bGetFeature = false;
}

void CWMRDemoDlg::OnBnClickedButVerify()
{
	// TODO: 在此添加控件通知处理程序代码
	m_bGetFeature =true;
	m_bFinished = false;
	m_CaptureType = 1;
	m_strtmpstr.Format(_T("开始比对，请按压手指..\r\n"));
	ShowMessage(m_strtmpstr); 
}

void CWMRDemoDlg::OnBnClickedButStopverify()
{
	// TODO: 在此添加控件通知处理程序代码
	m_strtmpstr.Format(_T("停止比对..\r\n"));
	ShowMessage(m_strtmpstr); 
	m_bFinished = true;
	m_bGetFeature = false;
}

void CWMRDemoDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: 在此处添加消息处理程序代码
	if(m_DevHandle != NULL)
	{
		WM_CloseDevice(m_DevHandle);
		WM_Free();
	}


}

void CWMRDemoDlg::OnBnClickedButBeep()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_DevHandle == NULL || m_bOpened == false)
	{
		m_strtmpstr.Format(_T("请先打开设备..\r\n"));
		ShowMessage(m_strtmpstr);
		return;
	}
    int iOpenBeep=1;   
	int iCloseBeep=0;
	int nRet =WM_Beep(m_DevHandle,iOpenBeep); //开蜂鸣器
	if (nRet == WM_OK)
	{
		m_strtmpstr.Format(_T("蜂鸣器开成功..\r\n"));
		ShowMessage(m_strtmpstr);

	}
	else
	{
		m_strtmpstr.Format(_T("蜂鸣器开失败..\r\n"));
		ShowMessage(m_strtmpstr);
		return;
	}
	Sleep(2000);  //蜂鸣器响2秒
	nRet =WM_Beep(m_DevHandle,iCloseBeep); //开蜂鸣器
	if (nRet == WM_OK)
	{
		m_strtmpstr.Format(_T("蜂鸣器关成功..\r\n"));
		ShowMessage(m_strtmpstr);

	}
	else
	{
		m_strtmpstr.Format(_T("蜂鸣器关失败..\r\n"));
		ShowMessage(m_strtmpstr);
		return;
	}
}

void CWMRDemoDlg::OnBnClickedButBmp2fea()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_DevHandle == NULL || m_bOpened == false)
	{
		m_strtmpstr.Format(_T("请先打开设备..\r\n"));
		ShowMessage(m_strtmpstr);
		return;
	}
	CFileDialog OpenFile(TRUE,_T("打开位图"),NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,_T("位图文件(*.BMP)|*.BMP||"));
	if(OpenFile.DoModal() == IDOK)
	{
		CFile file;
		if (!file.Open(OpenFile.GetPathName(),CFile::modeRead))
			return;
		int nFileLen=file.GetLength();
		BYTE *pFileBuf=new BYTE[nFileLen+1];
		memset(pFileBuf,0,sizeof(nFileLen));
		if (file.Read(pFileBuf,nFileLen)!=nFileLen)
			return;
		file.Close();
		pFileBuf[nFileLen]=0;
		int width=0;
		int height=0;
		memcpy(&width,pFileBuf+18,4);
		memcpy(&height,pFileBuf+22,4);
		unsigned char pCharData[512]={0};
		char pCharDataHex[1024]={0};
		int iSize=0;
		int ret = WM_BmpImage2Feature(pFileBuf,width,height,pTemplate,&iSize);
		if (ret == WM_OK)
		{
			m_strtmpstr.Format(_T("提取特征成功，可开始比对..\r\n"));
			ShowMessage(m_strtmpstr);

		}
		else
		{
			m_strtmpstr.Format(_T("提取特征失败..\r\n"));
			ShowMessage(m_strtmpstr);
		}

		if (pFileBuf)
		{
			delete [] pFileBuf;
			pFileBuf =NULL;
		}

	}
	UpdateData(FALSE);

}

void CWMRDemoDlg::OnBnClickedButSavebmp()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_DevHandle == NULL || m_bOpened == false)
	{
		m_strtmpstr.Format(_T("请先打开设备..\r\n"));
		ShowMessage(m_strtmpstr);
		return;
	}
	CFileDialog OpenFile(FALSE,_T("保存位图"),NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,_T("位图文件(*.BMP)|*.BMP||"));
	if(OpenFile.DoModal() == IDOK)
	{
		CFile cfile;
		BOOL bSave = cfile.Open(OpenFile.GetPathName(),CFile::modeCreate | CFile::modeWrite);
		if (bSave)
		{
			cfile.Write(BmpImageBuf,ImageWidth*ImageHeight+1078);
			cfile.Close();
			m_strtmpstr.Format(_T("保存图像成功..\r\n"));
			ShowMessage(m_strtmpstr);
		}
		else
		{
			m_strtmpstr.Format(_T("保存图像失败..\r\n"));
			ShowMessage(m_strtmpstr);
		}
	}
}

void CWMRDemoDlg::OnBnClickedButExit()
{
	// TODO: 在此添加控件通知处理程序代码
	OnCancel();
}
