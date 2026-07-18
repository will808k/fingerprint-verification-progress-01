unit Unit1;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, Menus;

type
  TForm1 = class(TForm)
    Image1: TImage;
    Panel1: TPanel;
    Button1: TButton;
    Button2: TButton;
    Button3: TButton;
    Button4: TButton;
    Memo1: TMemo;
    Label1: TLabel;
    PopupMenu1: TPopupMenu;
    SaveImage1: TMenuItem;
    SaveDialog1: TSaveDialog;
    procedure FormCreate(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure Button1Click(Sender: TObject);
    procedure Button3Click(Sender: TObject);
    procedure Button2Click(Sender: TObject);
    procedure SaveImage1Click(Sender: TObject);
    procedure Button4Click(Sender: TObject);
  private
    { Private declarations }
    FDevHandle: THANDLE;
    FImgWidth, FImgHeight, FImgBmpSize{, FImgDPI}: Integer;
    FImageBuffer: PByte;
    FImageBufferBMP: PByte;// BMP Image
    ///////////////////
    FTemplate: PByte;
    FFeature: PByte;
    function DoGetImageBuffer(): Boolean;
  public
    { Public declarations }
  end;

var
  Form1: TForm1;

implementation

{$R *.dfm}

uses
  uWMRAPI;

procedure BinToHex(BinValue, HexValue: PChar; BinBufSize: Integer);
Const
  HexDigits='0123456789ABCDEF';
var
  i : longint;
begin
  for i:=0 to binbufsize-1 do
    begin
    HexValue[0]:=hexdigits[1+((ord(binvalue^) shr 4))];
    HexValue[1]:=hexdigits[1+((ord(binvalue^) and 15))];
    inc(hexvalue,2);
    inc(binvalue);
    end;
end;

function FormatBinToHex(pData:PByte; iLen: Cardinal):string;
var
  psResult: PChar;
begin
  Result := '';
  if (pData=nil) or (iLen=0) then Exit;
  psResult := AllocMem(2*iLen+1);
  try
    try
      BinToHex(PChar(pData), psResult, iLen);
    except
    end;
    Result := string(psResult);
  finally
    FreeMem(psResult);
  end;
end;

type

  { TGetImageThread }

  TGetImageThread = class(TThread)
  private
    FOwner: TForm1;
    //FFlipVertical: Boolean;
    FSuccess: Boolean;
    {$IFDEF VER150}
    Finished: Boolean;
    {$ENDIF}
    //////////////
    FMessage: string;
    procedure DoSetUIMessage();
  protected
    procedure Execute(); override;
    procedure DoTerminate; override;
  public
    procedure SetUIMessage(sMessage: string);
  end;

{ TGetImageThread }

procedure TGetImageThread.DoSetUIMessage;
begin
  FOwner.Label1.Caption:= FMessage;
end;

procedure TGetImageThread.DoTerminate;
begin
  inherited;
  {$IFDEF VER150}
  Finished := True;
  {$ENDIF}
end;

procedure TGetImageThread.Execute;
var
  dwFirst: DWORD;
  //nFlipVertical,
  nLen, nRet: Integer;
begin
  FSuccess:=False;
  dwFirst := GetTickCount();
  while not Terminated do
  begin
    if FOwner.FImageBuffer=nil then
    begin
      SetUIMessage('Image Info invalid, FImageBuffer=nil');
      Exit;
    end;
    nRet := WM_GetImage(FOwner.FDevHandle, 0, FOwner.FImageBuffer, nLen);
    if nRet=WM_OK then
    begin
      SetUIMessage('GetImage Success');
      FSuccess:=True;
      Break;
    end
    else
    begin
      SetUIMessage('GetImage Fail, err='+IntToStr(nRet));
    end;

    if (GetTickCount()-dwFirst)>10*1000 then
    begin
      SetUIMessage('ERROR_TIMEOUT 10 SEC');
      Break;
    end;
  end;
end;

procedure TGetImageThread.SetUIMessage(sMessage: string);
begin
  FMessage := sMessage;
  Synchronize(DoSetUIMessage);
end;

procedure TForm1.FormCreate(Sender: TObject);
begin
  FImageBuffer := nil;
  FImageBufferBMP := nil;
  FTemplate:=nil;
  FFeature:=nil;
  fDevHandle := INVALID_HANDLE_VALUE;
  
  if not uWMRAPI.InitAPI() then
  begin
    ShowMessage('Load WMRAPI.dll Fail!');
    PostMessage(Handle, WM_CLOSE, 0, 0);
    Exit;
  end;
end;

procedure TForm1.FormDestroy(Sender: TObject);
begin
  uWMRAPI.FinalAPI();


  if FImageBuffer<>nil then FreeMem(FImageBuffer);
  FImageBuffer := nil;
  if FImageBufferBMP<>nil then FreeMem(FImageBufferBMP);
  FImageBufferBMP := nil;

  if FTemplate<>nil then FreeMem(FTemplate);
  FTemplate := nil;
  if FFeature<>nil then FreeMem(FFeature);
  FFeature := nil;
end;

procedure TForm1.Button1Click(Sender: TObject);
var
  nRet, nCnt: Integer;
  szSN: PByte;
begin
  if fDevHandle <> INVALID_HANDLE_VALUE then
  begin
    Memo1.Lines.Add('Please CloseDevice First');
    Exit;
  end;

  nRet := WM_Init();
  if nRet<>WM_OK then
  begin
    Memo1.Lines.Add('WM_Init() Fail! err='+IntToStr(nRet));
    Exit;
  end;

  nCnt := WM_GetDeviceCount();
  if nCnt<0 then
  begin
    Memo1.Lines.Add('WM_GetDeviceCount Fail, err='+IntToStr(nCnt));
    Exit;
  end
  else if nCnt=0 then
  begin
    Memo1.Lines.Add('WM_GetDeviceCount, Count='+IntToStr(nCnt)+', Device Not Found!');
    Exit;
  end;
  Memo1.Lines.Add('WM_GetDeviceCount OK, Count='+IntToStr(nCnt));

  nRet := WM_OpenDevice(0, fDevHandle);
  if nRet<>WM_OK then
  begin
    fDevHandle := INVALID_HANDLE_VALUE;
    Memo1.Lines.Add('WM_OpenDevice Fail, nRet='+IntToStr(nRet));
    Exit;
  end;
  Memo1.Lines.Add('WM_OpenDevice OK, fDevHandle='+IntToStr(FDevHandle));

  szSN := AllocMem(128);
  try
    nRet := WM_GetSerialNumber(FDevHandle, szSN);
    if nRet<>WM_OK then
    begin
      Memo1.Lines.Add('WM_GetSerialNumber Fail, nRet='+IntToStr(nRet));
    end
    else
    begin
      Memo1.Lines.Add('WM_GetSerialNumber OK, SN='+string(AnsiString(PAnsiChar(szSN))));
    end;
  finally
    FreeMem(szSN);
  end;

  if FImageBuffer<>nil then
    FreeMem(FImageBuffer);
  FImageBuffer := nil;

  nRet := WM_GetImageInfo(FImgWidth, FImgHeight);
  if nRet<>WM_OK then
  begin
    Memo1.Lines.Add('WM_GetImageInfo Fail, nRet='+IntToStr(nRet));
  end
  else
  begin
    Memo1.Lines.Add('WM_GetImageInfo OK, FImgWidth='+IntToStr(FImgWidth)+' FImgHeight='+IntToStr(FImgHeight));
    FImageBuffer := AllocMem(FImgWidth*FImgHeight);
  end;   
end;

procedure TForm1.Button2Click(Sender: TObject);
var
  nRet: Integer;
begin
  if fDevHandle = INVALID_HANDLE_VALUE then
  begin
    Memo1.Lines.Add('Please OpenDevice First');
    Exit;
  end;

  nRet := WM_CloseDevice(fDevHandle);
  if nRet<>WM_OK then
  begin
    Memo1.Lines.Add('WM_CloseDevice Fail, nRet='+IntToStr(nRet));
    Exit;
  end;
  fDevHandle := INVALID_HANDLE_VALUE;
  Memo1.Lines.Add('WM_CloseDevice OK');

  WM_Free();
end;

procedure TForm1.Button3Click(Sender: TObject);
{$IFDEF VER150}
type
  UIntPtr = Cardinal;
{$ENDIF}
var
  //aImageArr:PPByte;
  aImageArr: array[0..10-1] of PByte;
var
  I, nRet, nLen: Integer;
begin
  if fDevHandle = INVALID_HANDLE_VALUE then
  begin
    Memo1.Lines.Add('Please OpenDevice First');
    Exit;
  end;
  if (FImgHeight*FImgWidth<=0) then
  begin
    Memo1.Lines.Add('fingerprint image size info error');
    Exit;
  end;

  //aImageArr := AllocMem(10*SizeOf(PByte));
  //for I:=0 to 10-1 do
  //  PPByte(UIntPtr(Pointer(aImageArr))+I*SizeOf(PByte))^ := nil;
  
  for I:=Low(aImageArr) to High(aImageArr) do
   aImageArr[I] := nil;

  Memo1.Lines.Add('enroll fingerprint BEGIN');


  for I:=0 to 6-1 do
  begin
    Memo1.Lines.Add('press fingerprint '+IntToStr(I+1));

    if not DoGetImageBuffer() then
      Continue;
    Memo1.Lines.Add('capture fingerprint '+IntToStr(I+1)+' OK');

    // Ö»Ìî³äÇ°3¸ö
    //if PPByte(UIntPtr(Pointer(aImageArr))+(I mod 3)*SizeOf(PByte))^ <>nil then
    //  FreeMem(PPByte(UIntPtr(Pointer(aImageArr))+(I mod 3)*SizeOf(PByte))^);
    //PPByte(UIntPtr(Pointer(aImageArr))+(I mod 3)*SizeOf(PByte))^ := AllocMem(FImgHeight*FImgWidth);
    //Move(FImageBuffer^, PPByte(UIntPtr(Pointer(aImageArr))+(I mod 3)*SizeOf(PByte))^^, FImgHeight*FImgWidth);
    if aImageArr[I mod 3] <>nil then
      FreeMem(aImageArr[I mod 3]);
    aImageArr[I mod 3] := AllocMem(FImgBmpSize);
    Move(FImageBufferBMP^, aImageArr[I mod 3]^, FImgBmpSize);

    if i>=2 then
    begin
      nLen := 4096;
      if FTemplate<>nil then FreeMem(FTemplate);
      FTemplate := AllocMem(nLen);

      Memo1.Lines.Add('try Generalize Template...');
      nRet := WM_GenTemplateWithImage(@aImageArr[0],//aImageArr,
                                      3, FImgWidth, FImgHeight, FTemplate, nLen);
      if nRet=WM_OK then
      begin
        Label1.Caption:= 'Enroll OK';
        Memo1.Lines.Add('Generalize Template OK, nLen='+IntToStr(nLen));
        Memo1.Lines.Add('Template: ' + sLineBreak+ FormatBinToHex(FTemplate, nLen));
        Break;
      end
      else
      begin
        Label1.Caption:= 'Enroll FAIL';
        Memo1.Lines.Add('Generalize Template FAIL..., err='+IntToStr(nRet));
      end;
    end;
  end;
  //if PPByte(UIntPtr(Pointer(aImageArr))+(0)*SizeOf(PByte))^<>nil then FreeMem(PPByte(UIntPtr(Pointer(aImageArr))+(0)*SizeOf(PByte))^);
  //if PPByte(UIntPtr(Pointer(aImageArr))+(1)*SizeOf(PByte))^<>nil then FreeMem(PPByte(UIntPtr(Pointer(aImageArr))+(1)*SizeOf(PByte))^);
  //if PPByte(UIntPtr(Pointer(aImageArr))+(2)*SizeOf(PByte))^<>nil then FreeMem(PPByte(UIntPtr(Pointer(aImageArr))+(2)*SizeOf(PByte))^);
  if aImageArr[0]<>nil then FreeMem(aImageArr[0]);
  if aImageArr[1]<>nil then FreeMem(aImageArr[1]);
  if aImageArr[2]<>nil then FreeMem(aImageArr[2]);
  Memo1.Lines.Add('enroll fingerprint END');
end;

function TForm1.DoGetImageBuffer: Boolean;
var
  aGetImageThread: TGetImageThread;
  BmpImage: PByte;
  nBmpImageBufferLength, nBmpImageBufferLengthRet: Integer;
  nRet: Integer;
  //bFlipVertical: Integer;
  aBitmap: TBitmap;
  aStream: TMemoryStream;
begin
  Result := False;
  aGetImageThread:=TGetImageThread.Create(True);
  try
    aGetImageThread.FOwner := Self;
    //aGetImageThread.FFlipVertical := CheckBox1.Checked;
    {$IFDEF VER150}  ///Delphi7
    aGetImageThread.Resume;
    {$ELSE}
    aGetImageThread.Start;
    {$ENDIF}

    while not aGetImageThread.Finished do
    begin
      Sleep(10);
      Application.ProcessMessages;;
    end;

    Result := aGetImageThread.FSuccess;
    if Result then
    begin
      nBmpImageBufferLength :=(FImgWidth * FImgHeight + 1078) *2;
      if nBmpImageBufferLength>0 then
      begin
        BmpImage := AllocMem(nBmpImageBufferLength);

        nBmpImageBufferLengthRet := nBmpImageBufferLength;
        nRet := WM_RawToBMP(FImageBuffer, FImgWidth, FImgHeight, BmpImage, nBmpImageBufferLengthRet);
        if (nRet<>WM_OK) and (nBmpImageBufferLengthRet>nBmpImageBufferLength) then
        begin
          FreeMem(BmpImage);
          BmpImage := AllocMem(nBmpImageBufferLengthRet);
          nRet := WM_RawToBMP(FImageBuffer, FImgWidth, FImgHeight, BmpImage, nBmpImageBufferLengthRet);
        end;
        
        if (nRet=WM_OK) then
        begin
          FImgBmpSize := nBmpImageBufferLengthRet;

          if FImageBufferBMP<>nil then FreeMem(FImageBufferBMP);
          FImageBufferBMP := AllocMem(FImgBmpSize);
          Move(BmpImage^, FImageBufferBMP^, FImgBmpSize);


          aStream:=TMemoryStream.Create;
          aBitmap:=TBitmap.Create;
          try
            aStream.Write(BmpImage^, nBmpImageBufferLengthRet);
            aStream.Position := 0;
            aBitmap.LoadFromStream(aStream);
            Image1.Picture.Assign(aBitmap);
          finally
            aBitmap.Free;
            aStream.Free;
          end;
        end;
        FreeMem(BmpImage);
      end;
    end;
  finally
    aGetImageThread.Free;
  end;
end;

procedure TForm1.SaveImage1Click(Sender: TObject);
begin
  SaveDialog1.FileName:='FingerprintImage_'+IntToStr(GetTickCount())+'.bmp';
  if not SaveDialog1.Execute() then
    Exit;
  Image1.Picture.SaveToFile(SaveDialog1.FileName);
end;

procedure TForm1.Button4Click(Sender: TObject);
var
  nScore, nLen: Integer;
  nRet: Integer;
begin
  if fDevHandle = INVALID_HANDLE_VALUE then
  begin
    Memo1.Lines.Add('Please OpenDevice First');
    Exit;
  end;

  if (FImgHeight*FImgWidth<=0) then
  begin
    Memo1.Lines.Add('fingerprint image size info error');
    Exit;
  end;

  if FTemplate=nil then
  begin
    Memo1.Lines.Add('please enroll fingerprint first');
    Exit;
  end;

  Memo1.Lines.Add('verify fingerprint BEGIN');

  if DoGetImageBuffer() then
  begin
    nLen := 4096;
    if FFeature<>nil then FreeMem(FFeature);
    FFeature := AllocMem(nLen);
    nRet := WM_Extract(FImageBufferBMP, FImgWidth, FImgHeight, FFeature, nLen);
    if nRet=WM_OK then
    begin
      Memo1.Lines.Add('Extract Feature OK nLen='+IntToStr(nLen));
      Memo1.Lines.Add('Feature: ' + sLineBreak+ FormatBinToHex(FFeature, nLen));

      nRet := WM_Verify(FTemplate, FFeature, nScore);
      if nRet=WM_OK then
      begin
        Memo1.Lines.Add('Verify OK nScore='+IntToStr(nScore));
        Label1.Caption:= 'Verify OK nScore='+IntToStr(nScore);
      end
      else
      begin
        Memo1.Lines.Add('Verify FAIL, err='+IntToStr(nRet));
        Label1.Caption:= 'Verify FAIL';
      end;
    end
    else
      Memo1.Lines.Add('Extract Feature FAIL, err='+IntToStr(nRet));
  end
  else
    Memo1.Lines.Add('Get Image FAIL');

  Memo1.Lines.Add('verify fingerprint END');

end;

end.
