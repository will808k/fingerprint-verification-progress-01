#pragma once

class CStretchImage
{
public:
    typedef struct {
        WORD x,y;  // dimensions
        WORD l;    // bytes per scan-line (32-bit allignment)
        BYTE *b;   // bits of bitmap,3 bytes/pixel, BGR
    } tWorkBMP;  // 24-bit working bitmap

    CStretchImage(void);
    virtual ~CStretchImage(void);
    void CreateWorkingBitmap (WORD dx,WORD dy,tWorkBMP *w);
    POINT GetBitmapSize (HBITMAP h);
    void SetBMIHeader (BITMAPINFO *b,short dx,short dy);
    void OpenBitmapForWork (HBITMAP b,tWorkBMP *w);
    HBITMAP CreateEmptyBitmap (WORD dx,WORD dy);
    void SaveWorkingBitmap (tWorkBMP *w,HBITMAP b);
    void ShrinkWorkingBitmap (tWorkBMP *a,tWorkBMP *b,WORD bx,WORD by);
    HBITMAP ShrinkBitmap(HBITMAP a, WORD bx, WORD by);
    BOOL SaveHBITMAPTOBMP(HBITMAP hBitmap, CString FileName);

};
