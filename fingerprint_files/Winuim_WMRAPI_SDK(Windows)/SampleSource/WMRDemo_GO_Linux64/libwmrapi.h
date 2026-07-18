#ifndef WMR_H
#define WMR_H

#ifdef WMR_LIB
#define WMR_API extern
#elif JNI_LIB
#define WMR_API extern
#elif WMR_EXPORTS
#define WMR_API __declspec(dllexport)
#else
#define WMR_API __declspec(dllimport)
#endif

//typedef unsigned int HANDLE;
//typedef void* POINTER;

//#if (sizeof(POINTER)==4)
//typedef unsigned int HANDLE;
//#else
typedef unsigned long long HANDLE;
//#endif

#ifdef __cplusplus
extern "C"
{
#endif
    WMR_API int WM_Init();
    WMR_API int WM_Free();
    WMR_API int WM_GetDeviceCount();
    WMR_API int WM_OpenDevice(int nDevIndex , HANDLE* DevHandle);
    WMR_API int WM_CloseDevice(HANDLE DevHandle);
    WMR_API int WM_GetSerialNumber (HANDLE DevHandle, unsigned char* DeviceSN);
    WMR_API int WM_GetImageInfo (int * ImageWidth, int* ImageHeight);
    WMR_API int WM_GetImage(HANDLE DevHandle, int nTimeOut, unsigned char *ImageBuf, int *Size);
    WMR_API int WM_Extract ( unsigned char * ImageBuf, int ImageWidth, int ImageHeight, unsigned char * Feature, int * Size );
    WMR_API int WM_GenTemplateWithImage ( unsigned char *Image[3], int nCount, int nWidth, int nHeight, unsigned char *Template, int * Size );
    WMR_API int WM_GenTemplateWithImage3 ( unsigned char *Image1, unsigned char *Image2, unsigned char *Image3, int nWidth, int nHeight, unsigned char *Template, int * Size );
    WMR_API int WM_Verify ( unsigned char *Template, unsigned char * Feature, int *Score);
    WMR_API int WM_VerifyEx ( unsigned char *Template, unsigned char * Feature, int *Score);
#ifdef __cplusplus
}
#endif

#endif //WMR_H
