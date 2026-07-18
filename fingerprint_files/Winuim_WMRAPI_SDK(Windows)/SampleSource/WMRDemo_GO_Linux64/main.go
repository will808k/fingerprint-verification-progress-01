package main

/*
#cgo CFLAGS: -I . -O2 -DWMR_LIB
#cgo LDFLAGS: -L . -lwmrapi -lm

#include "libwmrapi.h"
*/
import (
	"C"
)
import (
	"fmt"
	"io/ioutil"
	"log"
	"time"
	"unsafe"
)

// 编译如果找不到动态库执行
// export LIBRARY_PATH=.
// 运行如果找不到动态库执行
// export LD_LIBRARY_PATH=.
func main2() {
	ret := C.WM_Init()
	if ret != 0 {
		log.Println("初始化sdk失败，code:", ret)
		return
	}

	// num := C.WM_GetDeviceCount()
	// log.Println("当前设备数量:", num)

	// //打开设备
	// var handle int64
	// phandle := &handle
	// ret = C.WM_OpenDevice(0, (*C.uint)(unsafe.Pointer(&phandle)))
	// if ret != 0 {
	// 	log.Println("打开设备失败，code:", ret)
	// 	return
	// }
	// defer func() {
	// 	ret := C.WM_CloseDevice((*C.uint)(unsafe.Pointer(phandle)))
	// 	log.Println("关闭设备，code:", ret)

	// 	C.WM_Free()
	// }()
	// log.Println("打开设备成功", phandle)
	// var width, height int
	// ret = C.WM_GetImageInfo((*C.int)(unsafe.Pointer(&width)), (*C.int)(unsafe.Pointer(&height)))
	// if ret != 0 {
	// 	log.Println("获取图形信息失败，code:", ret)
	// 	return
	// }
	// log.Println("设备图像宽高信息", width, height)

	var (
		buff [4][]byte
		// lens [4]int
		err error
	)
	log.Println("开始提取模板流程")
	for i := 0; i < 3; i++ {
		buff[i], err = ioutil.ReadFile(fmt.Sprintf("./%v.bmp", i+1))
		if err != nil {
			log.Println(i, err)
			return
		}
	}

	//提取模板
	tmp := make([]byte, 1024)
	var tmpLen int
	ret = C.WM_GenTemplateWithImage3((*C.uchar)(unsafe.Pointer(&buff[0][0])), (*C.uchar)(unsafe.Pointer(&buff[1][0])), (*C.uchar)(unsafe.Pointer(&buff[2][0])), C.int(256), C.int(288), (*C.uchar)(unsafe.Pointer(&tmp[0])), (*C.int)(unsafe.Pointer(&tmpLen)))
	if ret != 0 {
		log.Println("提取模板失败，code:", ret)
		return
	}
	log.Printf("提取模板成功, 长度：%v\n", tmpLen)
}

func main() {
	ret := C.WM_Init()
	if ret != 0 {
		log.Println("初始化sdk失败，code:", ret)
		return
	}

	num := C.WM_GetDeviceCount()
	log.Println("当前设备数量:", num)

	//打开设备
	var handle int64
	//phandle := &handle
	ret = C.WM_OpenDevice(0, (*C.ulonglong)(unsafe.Pointer(&handle)))
	if ret != 0 {
		log.Println("打开设备失败，code:", ret)
		return
	}
	defer func() {
		ret := C.WM_CloseDevice(C.ulonglong(handle))
		log.Println("关闭设备，code:", ret)

		C.WM_Free()
	}()
	log.Println("打开设备成功", handle)
	var width, height int
	ret = C.WM_GetImageInfo((*C.int)(unsafe.Pointer(&width)), (*C.int)(unsafe.Pointer(&height)))
	if ret != 0 {
		log.Println("获取图形信息失败，code:", ret)
		return
	}
	log.Println("设备图像宽高信息", width, height)

	//获取图像
	var (
		buff [4][]byte
		lens [4]int
	)
	log.Println("开始提取模板流程")
	for i := 0; i < 3; i++ {
		buff[i] = make([]byte, width*height+1078)
		// int WM_GetImage(HANDLE DevHandle, int nTimeOut, unsigned char *ImageBuf, int *Size);
		for {
			ret = C.WM_GetImage(C.ulonglong(handle), C.int(5000), (*C.uchar)(unsafe.Pointer(&buff[i][1078])), (*C.int)(unsafe.Pointer(&lens[i])))
			if ret == 0 {
				log.Printf("获取指纹图像成功, 第%v次，长度：%v\n", i, lens[i])
				break
			} else if ret == -107 {
				log.Println("超时，重新请求", i)
				continue
			} else {
				log.Println("获取指纹图像失败，code:", ret)
				return
			}
		}
		time.Sleep(time.Second)
	}

	//提取模板
	var tmp [1024]byte
	var tmpLen int
	ret = C.WM_GenTemplateWithImage3((*C.uchar)(unsafe.Pointer(&buff[0][0])), (*C.uchar)(unsafe.Pointer(&buff[1][0])), (*C.uchar)(unsafe.Pointer(&buff[2][0])), C.int(width), C.int(height), (*C.uchar)(unsafe.Pointer(&tmp[0])), (*C.int)(unsafe.Pointer(&tmpLen)))
	if ret != 0 {
		log.Println("提取模板失败，code:", ret)
		return
	}
	log.Printf("提取模板成功, 长度：%v\n", tmpLen)

	log.Println("开始提取特征流程")
	buff[3] = make([]byte, width*height+1078)
	for {
		ret = C.WM_GetImage(C.ulonglong(handle), 500, (*C.uchar)(unsafe.Pointer(&buff[3][0])), (*C.int)(unsafe.Pointer(&lens[3])))
		if ret == 0 {
			break
		} else if ret == -107 {
			log.Println("超时，重新请求")
			continue
		} else {
			log.Println("获取指纹图像失败，code:", ret)
			return
		}
		log.Printf("获取指纹图像成功，长度：%v\n", lens[3])
		break
	}

	var feat [1024]byte
	var featLen int
	ret = C.WM_Extract((*C.uchar)(unsafe.Pointer(&buff[3][0])), C.int(width), C.int(height), (*C.uchar)(unsafe.Pointer(&feat[0])), (*C.int)(unsafe.Pointer(&featLen)))
	if ret != 0 {
		log.Println("提取特征失败，code:", ret)
		return
	}
	log.Printf("提取特征成功, 长度：%v\n", featLen)

	var score int
	ret = C.WM_Verify((*C.uchar)(unsafe.Pointer(&tmp[0])), (*C.uchar)(unsafe.Pointer(&feat[0])), (*C.int)(unsafe.Pointer(&score)))
	log.Printf("比对结果，ret: %v, score: %v\n", ret, score)

}
