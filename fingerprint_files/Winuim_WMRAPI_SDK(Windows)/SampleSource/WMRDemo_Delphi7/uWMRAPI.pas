unit uWMRAPI;

interface
uses
  Windows, SysUtils;

{$DEFINE WEVI2}

const
  WM_OK				= 0;       //成功
  WM_FAIL				= -1;      //失败
  WM_INIT_FAIL		= -101;    //初始化设备失败
  WM_FREE_FAIL		= -102;	//释放设备失败
  WM_OPEN_FAIL		= -103;	//打开设备失败
  WM_AUTHEN_FAIL		= -104;	//设备验证失败
  WM_CLOSE_FAIL		= -105;	//关闭设备失败
  WM_GETIMG_FAIL		= -106;    //获取图像失败
  WM_IMG_TIMEOUT		= -107;    //获取图像超时
  WM_EXTRACT_FAIL		= -110;	//提取特征失败
  WM_GENTEMP_FAIL		= -111;	//合成模板失败
  WM_VERIFY_FAIL		= -112;	//比对失败
  WM_IMGCONVERT_FAIL  = -113;	//图像转换失败
  WM_FEACONVERT_FAIL  = -114;	//特征转换失败
  WM_PARAMETER_ERROR	= -120;	//参数传入错误


var
(*==================================================================================
功能： 初始化设备。
参数： 无
返回： 0:表示调用成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*)
  WM_Init: function (): Integer; stdcall;

(*==================================================================================
功能： 释放设备。
参数： 无
返回： 0:表示调用成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*)
  WM_Free: function (): Integer; stdcall;

(*==================================================================================
功能： 得到当前型号指纹采集设备个数。
参数： 无
返回： 设备个数
==================================================================================*)
  WM_GetDeviceCount: function (): Integer; stdcall;


(*==================================================================================
功能： 打开当前指定的设备，目前只支持打开一个指纹设备。
参数： [in] nDevIndex: 指纹采集设备索引号，nDevIndex为0表示打开第一个设备。
[out] DevHandle: 设备句柄
返回： 0:表示打开设备成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*)
  WM_OpenDevice: function (nDevIndex: Integer; var DevHandle: THANDLE): Integer; stdcall;

(*==================================================================================
功能： 关闭某个设备。
参数： [in]DevHandle： 设备句柄。
返回： 0:表示关闭设备成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*)
  WM_CloseDevice: function (DevHandle: THANDLE): Integer; stdcall;

(*==================================================================================
功能： 获取指纹采集设备序列号。
参数： [in] DevHandle： 设备句柄；
[out] DeviceSN：输出设备序列号，由调用者申请内存。
返回： 0: 表示获取设备序列号成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*)
  WM_GetSerialNumber: function (DevHandle: THANDLE; DeviceSN: PByte): Integer; stdcall;


(*==================================================================================
功能： 获取指纹图像信息。
参数： [out] ImageWidth：图像宽度；
[out] ImageHeight：图像高度。
返回： 0: 表示获取图像信息成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*)
  WM_GetImageInfo: function (var ImageWidth: Integer; var ImageHeight: Integer): Integer; stdcall;


(*==================================================================================
功能： 获取指纹采集图像。
参数： [in] DevHandle： 设备句柄；
[in] nTimeOut：取图像超时时间，单位为毫秒, 具体为：
0：立即返回；
>0：等待时间。
[out] ImageBuf：获取的图像缓冲区，格式为RAW，由调用者申请内存，大小为：图像宽度* 图像高度；
[out] Size: 返回的图像数据大小。
返回： 0：表示获取指纹图像成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*)
  WM_GetImage: function (DevHandle: THANDLE; nTimeOut: Integer;
    ImageBuf: PByte; var Size: Integer): Integer; stdcall;

(*==================================================================================
功能： 从指纹图像中提取指纹特征。
参数： [in] ImageBuf： 指纹RAW图像；
[in] ImageWidth：传入指纹图像数据的宽度；
[in] ImageHeight：传入指纹图像数据的高度；
[out] Feature：指纹特征缓冲区，由调用者申请内存；
[out] Size: 返回的指纹特征数据大小。
返回： 0：表示提取指纹特征成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*)
  WM_Extract: function (ImageBuf: PByte; ImageWidth: Integer; ImageHeight: Integer;
    Feature: PByte{$IFDEF WEVI2}; var nSize: Integer{$ENDIF}): Integer; stdcall;



{$IFDEF WEVI2}
(*==================================================================================
功能： 根据三幅指纹图像合成指纹模板。
参数： [in] Image: 图像数组，默认传3个图像；
       [in] nCount：图像个数，默认为3个；
	   [in] nWidth：传入指纹图像数据的宽度；
	   [in] nHeight：传入指纹图像数据的高度；
[out] Template：获取模板缓冲区，由调用者申请内存；
[out] Size: 返回的指纹模板数据大小。
返回： 0：表示合成指纹模板成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*)
type
  PPByte=^PByte;
var
  WM_GenTemplateWithImage: function (ImageArr10: PPByte;
	  nCount: Integer;
    nWidth: Integer;
    nHeight: Integer;
	  Template: PByte;
	  var nSize: Integer): Integer; stdcall;
{$ELSE}
(*==================================================================================
功能： 根据三幅指纹图像的特征合成指纹模板。
参数： [in] Feature1，Feature2，Feature3，分别表示3个特征；
[out] Template：获取模板缓冲区，由调用者申请内存；
[out] Size: 返回的指纹模板数据大小。
返回： 0：表示合成指纹模板成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*)
  WM_GenTemplate: function (
    Feature1: PByte;
    Feature2: PByte;
    Feature3: PByte;
    Template: PByte; Size: Integer): Integer; stdcall;
{$ENDIF}


(*==================================================================================
功能： 获取的特征与传入的模板进行比对。
参数： [in] Template：传入指纹模板数据；
[in] Feature：传入指纹特征数据；
[out] Score：比对相似度。
返回： 0：表示比对成功；
其它:比对失败。
==================================================================================*)
  WM_Verify: function (
    Template: PByte;
    Feature: PByte;
    var Score: Integer): Integer; stdcall;


(*==================================================================================
功能： 将指纹图像从RAW格式转换为BMP格式。
参数： [in] RawImageBuf：传入RAW格式指纹图像数据；
[in] ImageWidth：传入指纹图像数据的宽度；
[in] ImageHeight：传入指纹图像数据的高度；
[out] BmpImageBuf：获取BMP格式图像的缓冲区，由调用者申请内存，大小一般为：图像宽度×图像高度+1078；
[out] Size：返回图像数据的大小。
返回： 0:表示调用成功；
其它:调用失败，具体参数“错误码定义”。
==================================================================================*)
  WM_RawToBMP: function (
    RawImageBuf: PByte;
    ImageWidth: Integer;
    ImageHeight: Integer;
    BmpImageBuf: PByte;
    var Size: Integer): Integer; stdcall;


const
  FUNC_LOAD_SDK_CALL_INIT = FALSE;

function InitAPI(sLibPath: string=''):Boolean;
function FinalAPI():Boolean;

implementation

var
  //DLL加载引用计数. 0时才释放
  iRefCount: Integer = 0;
  hLib: HModule=0;
const
  DLLMODULE0 = 'WMRAPI.dll';

function GetModuleName(): string;
var
  DllPath: array [0 .. MAX_PATH] of CHAR;
begin
  ZeroMemory(@DllPath[0], MAX_PATH);
  Windows.GetModuleFileName(HInstance, DllPath, MAX_PATH);
  Result := string(DllPath);
end;

function InitAPI(sLibPath: string):Boolean;
var
  sLibDLL,
  sOldDir, sRootPath: string;
begin
  Result := False;


  if FileExists(sLibPath) then
  begin
    sLibDLL := sLibPath;
  end
  else if DirectoryExists(sLibPath) then
  begin
    sLibDLL := SysUtils.IncludeTrailingPathDelimiter(sLibPath)+DLLMODULE0;
  end
  else
  begin
    if System.IsLibrary then
      sRootPath := ExtractFilePath(GetModuleName())
    else
      sRootPath := ExtractFilePath(ParamStr(0));

    sLibDLL := sRootPath+DLLMODULE0;
  end;


  if hLib = 0 then
  begin
    if FileExists(sLibDLL) then
    begin
      sOldDir := GetCurrentDir();
      try
        SetCurrentDir(ExtractFilePath(sLibDLL));
        hLib := Windows.LoadLibrary(PChar(sLibDLL));
      finally
        SetCurrentDir(sOldDir);
      end;
    end;
  end;

  {$IFDEF MSWINDOWS}
  if hLib>32 then
  {$ELSE}
  if hLib<>0 then
  {$ENDIF}
  begin
    Result := True;
    if iRefCount=0 then
    begin
      WM_Init          := Windows.GetProcAddress(hLib, 'WM_Init');
      WM_Free := Windows.GetProcAddress(hLib, 'WM_Free');
      WM_GetDeviceCount := Windows.GetProcAddress(hLib, 'WM_GetDeviceCount');
      WM_OpenDevice := Windows.GetProcAddress(hLib, 'WM_OpenDevice');
      WM_CloseDevice := Windows.GetProcAddress(hLib, 'WM_CloseDevice');
      WM_GetSerialNumber      := Windows.GetProcAddress(hLib, 'WM_GetSerialNumber');
      WM_GetImageInfo     := Windows.GetProcAddress(hLib, 'WM_GetImageInfo');
      WM_GetImage    := Windows.GetProcAddress(hLib, 'WM_GetImage');
      WM_Extract    := Windows.GetProcAddress(hLib, 'WM_Extract');
      {$IFDEF WEVI2}
      WM_GenTemplateWithImage := Windows.GetProcAddress(hLib, 'WM_GenTemplateWithImage');
      {$ELSE}
      WM_GenTemplate       := Windows.GetProcAddress(hLib, 'WM_GenTemplate');
      {$ENDIF}
      WM_Verify       := Windows.GetProcAddress(hLib, 'WM_Verify');
      WM_RawToBMP       := Windows.GetProcAddress(hLib, 'WM_RawToBMP');
      {$IF FUNC_LOAD_SDK_CALL_INIT}
      if Assigned(WM_Init) then
      begin
        WM_Init();
      end;
      {$IFEND}
    end;
    Windows.InterlockedIncrement(iRefCount);

    //Windows.OutputDebugString(PChar(
    //  'WMRAPI_WMR06.InitAPI '+sRootPath+DLLMODULE0+' OK '));
  end
  else
  begin
    //Windows.OutputDebugString(PChar(
    //  'WMRAPI_WMR06.InitAPI '+sRootPath+DLLMODULE0+' FAIL '+
    //  'LastError='+IntToStr(Windows.GetLastError())));
  end;
end;

function FinalAPI():Boolean;
begin
  Result := True;
  {$IFDEF MSWINDOWS}
  if hLib>32 then
  {$ELSE}
  if hLib<>0 then
  {$ENDIF}
  begin
    if InterlockedDecrement(iRefCount)=0 then
    begin
      {$IF FUNC_LOAD_SDK_CALL_INIT}
      if Assigned(WM_Free) then
      begin
        WM_Free();
      end;
      {$IFEND}

      FreeLibrary(hLib);
      hLib := 0;

      WM_Init := nil;
      WM_Free := nil;
      WM_GetDeviceCount := nil;
      WM_OpenDevice := nil;
      WM_CloseDevice := nil;
      WM_GetSerialNumber := nil;
      WM_GetImageInfo := nil;
      WM_GetImage := nil;
      WM_Extract := nil;
      {$IFDEF WEVI2}
      WM_GenTemplateWithImage := nil;
      {$ELSE}
      WM_GenTemplate := nil;
      {$ENDIF}
      WM_Verify := nil;
      WM_RawToBMP := nil;
    end;
  end;
end;

initialization
  //InitAPI;

finalization
  //FinalAPI;

end.
