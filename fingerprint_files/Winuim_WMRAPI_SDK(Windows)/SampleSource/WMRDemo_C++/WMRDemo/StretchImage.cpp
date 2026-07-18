#include "StdAfx.h"
#include "StretchImage.h"

#define Alloc(p,t) (t *)malloc((p)*sizeof(t))
#define For(i,n) for ((i)=0;(i)<(n);(i)++)
#define iFor(n) For (i,n)
#define jFor(n) For (j,n)

CStretchImage::CStretchImage(void)
{
}

CStretchImage::~CStretchImage(void)
{
}

void CStretchImage::CreateWorkingBitmap (WORD dx,WORD dy,tWorkBMP *w)
{
    w->x=dx;
    w->y=dy;
    w->l=(dx+1)*3&0xfffc;
    w->b=Alloc(w->l*dy,BYTE);
}
POINT CStretchImage::GetBitmapSize (HBITMAP h)
{
    POINT p;
    BITMAP o;
    GetObject (h,sizeof(o),&o);
    p.x=o.bmWidth;
    p.y=o.bmHeight;
    return (p);
}
void CStretchImage::SetBMIHeader (BITMAPINFO *b,short dx,short dy)
{
    b->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    b->bmiHeader.biWidth=dx;
    b->bmiHeader.biHeight=-dy;
    b->bmiHeader.biPlanes=1;
    b->bmiHeader.biBitCount=24;
    b->bmiHeader.biCompression=BI_RGB;
    b->bmiHeader.biSizeImage=0;
    b->bmiHeader.biXPelsPerMeter=1;
    b->bmiHeader.biYPelsPerMeter=1;
    b->bmiHeader.biClrUsed=0;
    b->bmiHeader.biClrImportant=0;
}
void CStretchImage::OpenBitmapForWork (HBITMAP b,tWorkBMP *w)
{
    BITMAPINFO s;
    HDC h=GetDC(NULL);
    POINT v=GetBitmapSize(b);
    CreateWorkingBitmap (v.x,v.y,w);
    SetBMIHeader (&s,w->x,w->y);
    GetDIBits (h,b,0,w->y,w->b,&s,DIB_RGB_COLORS);
    ReleaseDC (NULL,h);
} 
HBITMAP CStretchImage::CreateEmptyBitmap (WORD dx,WORD dy)
{
    HDC h=GetDC(NULL);
    HBITMAP b=CreateCompatibleBitmap(h,dx,dy);
    ReleaseDC (NULL,h);

    return (b);
}



void CStretchImage::SaveWorkingBitmap (tWorkBMP *w,HBITMAP b)
{
    BITMAPINFO s;
    HDC h=GetDC(NULL);
    SetBMIHeader (&s,w->x,w->y);
    SetDIBits (h,b,0,w->y,w->b,&s,DIB_RGB_COLORS);
    ReleaseDC (NULL,h);
}

void CStretchImage::ShrinkWorkingBitmap (tWorkBMP *a,tWorkBMP *b,WORD bx,WORD by)
{
    BYTE *uy=a->b,*ux,i;
    WORD x,y,nx,ny=0;
    DWORD df=3*bx,nf=df*by,j;
    float k,qx[2],qy[2],q[4],*f=Alloc(nf,float);

    CreateWorkingBitmap (bx,by,b);

    jFor (nf) f[j]=0;
    j=0;

    For (y,a->y) {
        ux=uy;
        uy+=a->l;
        nx=0;
        ny+=by;

        if (ny>a->y) {

            qy[0]=1-(qy[1]=(ny-a->y)/(float)by);

            For (x,a->x) {

                nx+=bx;

                if (nx>a->x) {
                    qx[0]=1-(qx[1]=(nx-a->x)/(float)bx);

                    iFor (4) q[i]=qx[i&1]*qy[i>>1];

                    iFor (3) {
                        f[j]+=(*ux)*q[0];
                        f[j+3]+=(*ux)*q[1];
                        f[j+df]+=(*ux)*q[2];
                        f[(j++)+df+3]+=(*(ux++))*q[3];
                    }
                }
                else iFor (3) {
                    f[j+i]+=(*ux)*qy[0];
                    f[j+df+i]+=(*(ux++))*qy[1];
                }
                if (nx>=a->x) nx-=a->x;
                if (!nx) j+=3;
            }
        }
        else {
            For (x,a->x) {

                nx+=bx;

                if (nx>a->x) {
                    qx[0]=1-(qx[1]=(nx-a->x)/(float)bx);
                    iFor (3) {
                        f[j]+=(*ux)*qx[0];
                        f[(j++)+3]+=(*(ux++))*qx[1];
                    }
                }
                else iFor (3) f[j+i]+=*(ux++);

                if (nx>=a->x) nx-=a->x;
                if (!nx) j+=3;
            }
            if (ny<a->y) j-=df;
        }
        if (ny>=a->y) ny-=a->y;
    }

    nf=0;
    k=bx*by/(float)(a->x*a->y);
    uy=b->b;

    For (y,by) {
        jFor (df) uy[j]=f[nf++]*k+.5;
        uy+=b->l;
    }

    free (f);
}

HBITMAP CStretchImage::ShrinkBitmap(HBITMAP a, WORD bx, WORD by)
{
    tWorkBMP in,out;
    HBITMAP b=CreateEmptyBitmap(bx,by);
    OpenBitmapForWork (a,&in);
    ShrinkWorkingBitmap (&in,&out,bx,by);
    free (in.b);
    SaveWorkingBitmap (&out,b);
    free (out.b);
    return (b);
}

BOOL CStretchImage::SaveHBITMAPTOBMP(HBITMAP hBitmap, CString FileName)
{
    HDC     hDC;      
    //当前分辨率下每象素所占字节数      
    int     iBits;      
    //位图中每象素所占字节数      
    WORD     wBitCount;      
    //定义调色板大小，     位图中像素字节大小     ，位图文件大小     ，     写入文件字节数          
    DWORD     dwPaletteSize=0,     dwBmBitsSize=0,     dwDIBSize=0,     dwWritten=0;          
    //位图属性结构          
    BITMAP     Bitmap;              
    //位图文件头结构      
    BITMAPFILEHEADER     bmfHdr;              
    //位图信息头结构          
    BITMAPINFOHEADER     bi;              
    //指向位图信息头结构              
    LPBITMAPINFOHEADER     lpbi;              
    //定义文件，分配内存句柄，调色板句柄          
    HANDLE     fh,     hDib,     hPal,hOldPal=NULL;          

    //计算位图文件每个像素所占字节数          
    hDC     =     CreateDC(_T("DISPLAY"),     NULL,     NULL,     NULL);      
    iBits     =     GetDeviceCaps(hDC,     BITSPIXEL)     *     GetDeviceCaps(hDC,     PLANES);          
    DeleteDC(hDC);          
    if (iBits     <=     1)
        wBitCount     =     1;          
    else if(iBits     <=     4)
        wBitCount     =     4;          
    else if(iBits     <=     8)
        wBitCount     =     8;          
    else
        wBitCount     =     24;          

    GetObject(hBitmap,     sizeof(Bitmap),     (LPSTR)&Bitmap);      
    bi.biSize =     sizeof(BITMAPINFOHEADER);      
    bi.biWidth =     Bitmap.bmWidth;      
    bi.biHeight =     Bitmap.bmHeight;      
    bi.biPlanes  =     1;      
    bi.biBitCount =     wBitCount;      
    bi.biCompression   =     BI_RGB;      
    bi.biSizeImage  =     0;      
    bi.biXPelsPerMeter  =     0;      
    bi.biYPelsPerMeter                         =     0;      
    bi.biClrImportant                         =     0;      
    bi.biClrUsed                                                 =     0;      

    dwBmBitsSize     =     ((Bitmap.bmWidth     *     wBitCount     +     31)     /     32)     *     4     *     Bitmap.bmHeight;      

    //为位图内容分配内存          
    hDib     =     GlobalAlloc(GHND,dwBmBitsSize     +     dwPaletteSize     +     sizeof(BITMAPINFOHEADER));          
    lpbi     =     (LPBITMAPINFOHEADER)GlobalLock(hDib);          
    *lpbi     =     bi;          

    //     处理调色板              
    hPal     =     GetStockObject(DEFAULT_PALETTE);          
    if     (hPal)          
    {          
        hDC     =     ::GetDC(NULL);          
        hOldPal     =     ::SelectPalette(hDC,     (HPALETTE)hPal,     FALSE);          
        RealizePalette(hDC);          
    }      

    //     获取该调色板下新的像素值          
    GetDIBits(hDC,     hBitmap,     0,     (UINT)     Bitmap.bmHeight,     (LPSTR)lpbi     +     sizeof(BITMAPINFOHEADER)          
        +dwPaletteSize,                         (BITMAPINFO     *)lpbi,     DIB_RGB_COLORS);          

    //恢复调色板              
    if     (hOldPal)          
    {          
        ::SelectPalette(hDC,     (HPALETTE)hOldPal,     TRUE);          
        RealizePalette(hDC);          
        ::ReleaseDC(NULL,     hDC);          
    }          

    //创建位图文件              
    fh     =     CreateFile(FileName,     GENERIC_WRITE||GENERIC_READ,FILE_SHARE_READ||FILE_SHARE_WRITE,     NULL,     CREATE_ALWAYS,          
        FILE_ATTRIBUTE_NORMAL||FILE_FLAG_SEQUENTIAL_SCAN||FILE_FLAG_DELETE_ON_CLOSE,     NULL);          

    if     (fh     ==     INVALID_HANDLE_VALUE)         return     FALSE;          

    //     设置位图文件头          
    bmfHdr.bfType     =     0x4D42;     //     "BM"          
    dwDIBSize     =     sizeof(BITMAPFILEHEADER)     +     sizeof(BITMAPINFOHEADER)     +     dwPaletteSize     +     dwBmBitsSize;              
    bmfHdr.bfSize     =     dwDIBSize;          
    bmfHdr.bfReserved1     =     0;          
    bmfHdr.bfReserved2     =     0;          
    bmfHdr.bfOffBits     =     (DWORD)sizeof(BITMAPFILEHEADER)     +     (DWORD)sizeof(BITMAPINFOHEADER)     +     dwPaletteSize;          
    //     写入位图文件头          
    WriteFile(fh,     (LPSTR)&bmfHdr,     sizeof(BITMAPFILEHEADER),     &dwWritten,     NULL);          
    //     写入位图文件其余内容          
    WriteFile(fh,     (LPSTR)lpbi,     dwDIBSize,     &dwWritten,     NULL);          
    //清除              
    GlobalUnlock(hDib);          
    GlobalFree(hDib);          
    CloseHandle(fh);          

    return     TRUE;       
    return 0;
}