#pragma once

#define WM_OK				0       //成功
#define WM_FAIL				-1      //失败
#define WM_INIT_FAIL		-101    //初始化设备失败
#define WM_FREE_FAIL		-102	//释放设备失败
#define WM_OPEN_FAIL		-103	//打开设备失败
#define WM_AUTHEN_FAIL		-104	//设备验证失败
#define WM_CLOSE_FAIL		-105	//关闭设备失败
#define WM_GETIMG_FAIL		-106    //获取图像失败
#define WM_IMG_TIMEOUT		-107    //获取图像超时
#define WM_EXTRACT_FAIL		-110	//提取特征失败
#define WM_GENTEMP_FAIL		-111	//合成模板失败
#define WM_VERIFY_FAIL		-112	//比对失败
#define WM_IMGCONVERT_FAIL  -113	//图像转换失败
#define WM_FEACONVERT_FAIL  -114	//特征转换失败
#define WM_PARAMETER_ERROR	-120	//参数传入错误


#if defined (__ANDROID__) || defined( __linux )
#define  WMAPI 
#ifdef WMR_DYN_LOAD
// 使用dl库进行动态加载
#define WMFUNC extern 
#else
#define WMFUNC extern 
#endif
typedef size_t WMHANDLE;
#endif

#if defined (_WIN32)
#define  WMAPI __stdcall
#define WMFUNC extern 
typedef HANDLE WMHANDLE;
#endif

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((WMHANDLE)-1)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================
功能： 初始化设备。
参数： 无
返回： 0:表示调用成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*/
WMFUNC int WMAPI WM_Init ();

/*==================================================================================
功能： 释放设备。
参数： 无
返回： 0:表示调用成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*/
WMFUNC int WMAPI WM_Free ();

/*==================================================================================
功能： 得到当前型号指纹采集设备个数。
参数： 无
返回： 设备个数 
==================================================================================*/
WMFUNC int WMAPI WM_GetDeviceCount();

/*==================================================================================
功能： 打开当前指定的设备，目前只支持打开一个指纹设备。
参数： [in] nDevIndex: 指纹采集设备索引号，nDevIndex为0表示打开第一个设备。
[out] DevHandle: 设备句柄
返回： 0:表示打开设备成功； 
其它:调用失败，具体参数“错误码定义”。
==================================================================================*/
WMFUNC int WMAPI WM_OpenDevice (int nDevIndex , WMHANDLE* DevHandle);

/*==================================================================================
功能： 关闭某个设备。
参数： [in]DevHandle： 设备句柄。
返回： 0:表示关闭设备成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*/
WMFUNC int WMAPI WM_CloseDevice (WMHANDLE DevHandle);

/*==================================================================================
功能： 获取指纹采集设备序列号。
参数： [in] DevHandle： 设备句柄；
[out] DeviceSN：输出设备序列号，由调用者申请内存。
返回： 0: 表示获取设备序列号成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*/
WMFUNC int WMAPI WM_GetSerialNumber (WMHANDLE DevHandle,unsigned char* DeviceSN);

/*==================================================================================
功能： 获取指纹图像信息。
参数： [out] ImageWidth：图像宽度；
[out] ImageHeight：图像高度。
返回： 0: 表示获取图像信息成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*/
WMFUNC int WMAPI WM_GetImageInfo(int* ImageWidth,int* ImageHeight);

/*==================================================================================
功能： 获取指纹采集图像。
参数： [in] DevHandle： 设备句柄；
[in] nTimeOut：取图像超时时间，单位为毫秒, 具体为：
0：立即返回；
>0：等待时间。 
[out] ImageBuf：获取的图像缓冲区，格式为RAW，由调用者申请内存，大小为：图像宽度* 图像高度；
[out] Size: 返回的图像数据大小。
返回： 0：表示获取指纹图像成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*/
WMFUNC int WMAPI WM_GetImage(WMHANDLE DevHandle,  int nTimeOut, 
					  unsigned char *ImageBuf, int *Size);

/*==================================================================================
功能： 从指纹图像中提取指纹特征。
参数： [in] ImageBuf： 指纹RAW图像；
[in] ImageWidth：传入指纹图像数据的宽度；
[in] ImageHeight：传入指纹图像数据的高度；
[out] Feature：指纹特征缓冲区，由调用者申请内存；
[out] Size: 返回的指纹特征数据大小。
返回： 0：表示提取指纹特征成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*/
WMFUNC int WMAPI WM_Extract ( unsigned char * ImageBuf, 
					  int ImageWidth,
					  int ImageHeight,
					  unsigned char * Feature, 
					  int * Size );

/*==================================================================================
功能： 根据三幅指纹图像合成指纹模板。
参数： [in] Image: 图像数组，默认传3个图像；
       [in] nCount：图像个数，默认为3个；
	   [in] nWidth：传入指纹图像数据的宽度；
	   [in] nHeight：传入指纹图像数据的高度；
[out] Template：获取模板缓冲区，由调用者申请内存；
[out] Size: 返回的指纹模板数据大小。
返回： 0：表示合成指纹模板成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*/
WMFUNC int WMAPI WM_GenTemplateWithImage(unsigned char *Image[10],
	int nCount,
	int nWidth,
	int nHeight,
	unsigned char *Template,
	int * Size);
WMFUNC int WMAPI WM_GenTemplateWithImage3(unsigned char *Image1, unsigned char *Image2, unsigned char *Image3,
	int nWidth,
	int nHeight,
	unsigned char *Template,
	int * Size);
	
/*==================================================================================
功能： 根据三幅指纹图像的特征合成指纹模板。
参数： [in] Feature1，Feature2，Feature3，分别表示3个特征；
[out] Template：获取模板缓冲区，由调用者申请内存；
[out] Size: 返回的指纹模板数据大小。
返回： 0：表示合成指纹模板成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*/
WMFUNC int WMAPI WM_GenTemplate( unsigned char *Feature1, 
						 unsigned char * Feature2,
						 unsigned char * Feature3, 
						 unsigned char *Template, 
						 int * Size );

/*==================================================================================
功能： 获取的特征与传入的模板进行比对。
参数： [in] Template：传入指纹模板数据；
[in] Feature：传入指纹特征数据；
[out] Score：比对相似度。
返回： 0：表示比对成功；
其它:比对失败。
==================================================================================*/
WMFUNC int WMAPI WM_Verify ( unsigned char *Template, 
					 unsigned char * Feature,
					 int *Score);

/*==================================================================================
功能： 将指纹图像从RAW格式转换为BMP格式。
参数： [in] RawImageBuf：传入RAW格式指纹图像数据；
[in] ImageWidth：传入指纹图像数据的宽度；
[in] ImageHeight：传入指纹图像数据的高度；
[out] BmpImageBuf：获取BMP格式图像的缓冲区，由调用者申请内存，大小一般为：图像宽度×图像高度+1078；
[out] Size：返回图像数据的大小。
返回： 0:表示调用成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*/
WMFUNC int WMAPI WM_RawToBMP ( unsigned char *RawImageBuf, 
					   int ImageWidth,
					   int ImageHeight,
					   unsigned char *BmpImageBuf, 
					   int *Size);


/*==================================================================================
功能： 开关蜂鸣器。
参数： [in] DevHandle：设备句柄；
[in] iBeepState：蜂鸣器状态，1为开启，0为关闭；
返回： 0:表示调用成功；
其它：调用失败，具体参数“错误码定义”。
备注：提示音可以根据调用开和关的时长来达到不同的音效
==================================================================================*/
WMFUNC int WMAPI WM_Beep(WMHANDLE DevHandle,int iBeepState);


/*==================================================================================
功能： 从指纹BMP图像中提取指纹特征。
参数： [in] BmpImageBuf： 指纹BMP图像；
[in] ImageWidth：传入指纹图像数据的宽度；
[in] ImageHeight：传入指纹图像数据的高度；
[out] Feature：指纹特征缓冲区，由调用者申请内存；
[out] Size: 返回的指纹特征数据大小。
返回： 0：表示提取指纹特征成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*/
WMFUNC int WMAPI WM_BmpImage2Feature(unsigned char *BmpImageBuf, int ImageWidth,int ImageHeight,unsigned char *Feature, int *Size);

/*==================================================================================
功能： 由居民身份证指纹特征转为纹宁指纹特征。
参数： [in] CIdfeature： 输入的居民身份证指纹特征；
[out] WMfeature：保存纹宁指纹特征缓冲区，由调用者申请内存；
返回：成功则返回纹宁特征的内存大小；
     小于0则调用失败，具体参数“错误码定义”。
==================================================================================*/
WMFUNC int WMAPI WM_ExtractFeatureFromCId ( char* CIdfeature, char* WMfeature);

/********************************************************************************
函数描述 : 由SY指纹特征转为WEVI指纹特征
参数描述 :
[in]  feature1 输入的SY指纹特征的内存数组
[out] feature2 保存WEVI指纹特征的内存
返回值 : 返回特征的内存大小
       小于0则调用失败，具体参数“错误码定义”。
********************************************************************************/
WMFUNC int WMAPI WM_ExtractFeatureFromSY(char* feature1, char* feature2);

#ifdef __cplusplus
}
#endif

#ifdef WMR_DYN_LOAD

typedef int (WMAPI *PFunc_WM_Init) ();
typedef int (WMAPI *PFunc_WM_Free) ();
typedef int (WMAPI *PFunc_WM_GetDeviceCount)();
typedef int (WMAPI *PFunc_WM_OpenDevice) (int nDevIndex , WMHANDLE* DevHandle);
typedef int (WMAPI *PFunc_WM_CloseDevice) (WMHANDLE DevHandle);
typedef int (WMAPI *PFunc_WM_GetSerialNumber) (WMHANDLE DevHandle,unsigned char* DeviceSN);
typedef int (WMAPI *PFunc_WM_GetImageInfo)(int* ImageWidth,int* ImageHeight);
typedef int (WMAPI *PFunc_WM_GetImage)(WMHANDLE DevHandle,  int nTimeOut, 
					  unsigned char *ImageBuf, int *Size);
typedef int (WMAPI *PFunc_WM_Extract) ( unsigned char * ImageBuf, 
					  int ImageWidth,
					  int ImageHeight,
					  unsigned char * Feature, 
					  int * Size );
typedef int (WMAPI *PFunc_WM_GenTemplateWithImage)(unsigned char *Image[10],
	int nCount,
	int nWidth,
	int nHeight,
	unsigned char *Template,
	int * Size);
typedef int (WMAPI *PFunc_WM_GenTemplateWithImage3)(unsigned char *Image1, unsigned char *Image2, unsigned char *Image3,
	int nWidth,
	int nHeight,
	unsigned char *Template,
	int * Size);
typedef int (WMAPI *PFunc_WM_GenTemplate)( unsigned char *Feature1, 
						 unsigned char * Feature2,
						 unsigned char * Feature3, 
						 unsigned char *Template, 
						 int * Size );
typedef int (WMAPI *PFunc_WM_Verify) ( unsigned char *Template, 
					 unsigned char * Feature,
					 int *Score);
typedef int (WMAPI *PFunc_WM_RawToBMP) ( unsigned char *RawImageBuf, 
					   int ImageWidth,
					   int ImageHeight,
					   unsigned char *BmpImageBuf, 
					   int *Size);
typedef int (WMAPI *PFunc_WM_Beep)(HANDLE DevHandle,int iBeepState);
typedef int (WMAPI *PFunc_WM_BmpImage2Feature)(unsigned char *BmpImageBuf, int ImageWidth,int ImageHeight,unsigned char *Feature, int *Size);
typedef int (WMAPI *PFunc_WM_ExtractFeatureFromCId) ( char* CIdfeature, char* WMfeature);
typedef int (WMAPI *PFunc_WM_ExtractFeatureFromSY)(char* feature1, char* feature2);

#ifdef __cplusplus
extern "C" {
#endif

extern PFunc_WM_Init pWM_Init;
extern PFunc_WM_Free pWM_Free;
extern PFunc_WM_GetDeviceCount pWM_GetDeviceCount;
extern PFunc_WM_OpenDevice pWM_OpenDevice;
extern PFunc_WM_CloseDevice pWM_CloseDevice;
extern PFunc_WM_GetSerialNumber pWM_GetSerialNumber;
extern PFunc_WM_GetImageInfo pWM_GetImageInfo;
extern PFunc_WM_GetImage pWM_GetImage;
extern PFunc_WM_Extract pWM_Extract;
extern PFunc_WM_GenTemplateWithImage pWM_GenTemplateWithImage;
extern PFunc_WM_GenTemplate pWM_GenTemplate;
extern PFunc_WM_Verify pWM_Verify;
extern PFunc_WM_RawToBMP pWM_RawToBMP;
extern PFunc_WM_Beep pWM_Beep;
extern PFunc_WM_BmpImage2Feature pWM_BmpImage2Feature;
extern PFunc_WM_ExtractFeatureFromCId pWM_ExtractFeatureFromCId;
extern PFunc_WM_ExtractFeatureFromSY pWM_ExtractFeatureFromSY;

#ifdef __cplusplus
}
#endif

#endif
// WMR_DYN_LOAD
