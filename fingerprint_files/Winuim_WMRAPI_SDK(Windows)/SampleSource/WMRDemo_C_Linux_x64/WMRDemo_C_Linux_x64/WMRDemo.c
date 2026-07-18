
#ifdef _MSC_VER
#pragma warning(once:4996)
#endif

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h> 
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <errno.h>

#include "WMRAPI.h"

int main(void)
{
  int ret = WM_OK;
  int num = 0;
  WMHANDLE hDev= 0;
  int width = 0;
  int height = 0;
  
	unsigned char*	buff[4]={NULL};
	int	lens[4]={0};
	int i;
	
	unsigned char tmp[1024]={0};
	int tmpLen = 0;

	unsigned char feat[1024]={0};
	int featLen = 0;
	int score = 0;
		
  #ifdef WMR_DYN_LOAD
  // 动态加载方式
  char* szPath="./libwmrapi.so";
  size_t hLib = (size_t)dlopen(szPath, RTLD_LAZY);
  if (hLib == 0) {
    printf("dlopen fail, err=%d \r\n", erron);
    return 0;
  }
  pWM_Init = (PFunc_WM_Init)dlsym((void *)hLib, "WM_Init");
  #endif
  
  ret = WM_Init();
	if (ret != WM_OK) {
		printf("初始化sdk失败，code:%d \r\n", ret);
		return 0;
	}
	
  num = WM_GetDeviceCount();
	printf("当前设备数量: %d \r\n", num);

	//打开设备
	ret = WM_OpenDevice(0, &hDev);
	if (ret != 0) {
		printf("打开设备失败，code:%d \r\n", ret);
		return 0;
	}
	
	printf("打开设备成功 hDev=%lu \r\n", hDev);

	ret = WM_GetImageInfo(&width, &height);
	if (ret != 0) {
		printf("获取图形信息失败，code:%d \r\n", ret);
		goto close_dev;
	}
	printf("设备图像宽高信息 width=%d, height=%d \r\n", width, height);

	//获取图像
	printf("开始提取模板流程... \r\n");
	
	for (i = 0; i < 3; i++) {
		buff[i] = malloc(width*height+1078);
		// int WM_GetImage(HANDLE DevHandle, int nTimeOut, unsigned char *ImageBuf, int *Size);
		while (1) {
			ret = WM_GetImage(hDev, 5000, &buff[i][1078], &lens[i]);
			if (ret == 0) {
				printf("获取指纹图像成功, 第%d次，长度：%d \r\n", i+1, lens[i]);
				break;
			} else if (ret == -107) {
				printf("超时，重新请求，请按压指纹... %d \r\n", i+1);
				continue;
			} else {
				printf("获取指纹图像失败，code:%d \r\n", ret);
				goto free_buf;
			}
		}
		
		if (i<2) {
      #ifdef _WIN32
      Sleep(1*1000);
      #else
      usleep(1*1000*1000);
      #endif
		}
	}
	
  printf("正在合成模板... \r\n");
	//提取模板
	ret = WM_GenTemplateWithImage3(buff[0], buff[1], buff[2], width, height, &tmp[0], &tmpLen);
	if (ret != 0) {
		printf("提取模板失败，code:%d \r\n", ret);
		goto free_buf;
	}
	printf("提取模板成功, 长度：%d \r\n", tmpLen);

  #ifdef _WIN32
  Sleep(1*1000);
  #else
  usleep(1*1000*1000);
  #endif
  
	printf("开始提取特征流程... \r\n");
  #ifdef _WIN32
  Sleep(200);
  #else
  usleep(200*1000);
  #endif
	buff[3] = malloc(width*height+1078);
	while (1) {
		ret = WM_GetImage(hDev, 500, buff[3], &lens[3]);
		if (ret == 0) {
			break;
		} else if (ret == -107) {
			printf("超时，重新请求，请按压指纹... \r\n");
			continue;
		} else {
			printf("获取指纹图像失败，code:%d \r\n", ret);
			goto free_buf;
		}
		printf("获取指纹图像成功，长度：%d \r\n", lens[3]);
		break;
	}


	ret = WM_Extract(buff[3], width, height, &feat[0], &featLen);
	if (ret != 0) {
		printf("提取特征失败，code:%d \r\n", ret);
		goto free_buf;
	}
	printf("提取特征成功, 长度：%d \r\n", featLen);


	ret = WM_Verify(&tmp[0], &feat[0], &score);
	printf("比对结果，ret: %d, score: %d \r\n", ret, score);
  
free_buf:
  for(i=0;i++;i<4)
    if (buff[i] != NULL)
      free(buff[i]);
    
close_dev:
		ret = WM_CloseDevice(hDev);
		printf("关闭设备，code:%d \r\n", ret);

		WM_Free();
	
  #ifdef WMR_DYN_LOAD
  dlclose((void*)hLib);
  #endif
}
