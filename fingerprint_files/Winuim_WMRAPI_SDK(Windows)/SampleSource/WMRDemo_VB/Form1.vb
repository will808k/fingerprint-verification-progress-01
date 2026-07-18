Imports System
Imports System.Collections.Generic
Imports System.ComponentModel
Imports System.Data
Imports System.Drawing
Imports System.IO
Imports System.Linq
Imports System.Runtime.InteropServices
Imports System.Text
Imports System.Threading
Imports System.Windows.Forms

Namespace WMPlatformCSharpAppLocalDllDemo
	Partial Public Class Form1
		Inherits Form

#Region "函数引用声明"
        <DllImport("WMRAPI.dll")>
        Public Shared Function WM_Init() As Integer
        End Function

        <DllImport("WMRAPI.dll")>
        Public Shared Function WM_Free() As Integer
        End Function
        <DllImport("WMRAPI.dll")>
        Public Shared Function WM_GetDeviceCount() As Integer
        End Function

        <DllImport("WMRAPI.dll")>
        Public Shared Function WM_OpenDevice(ByVal nDevIndex As Integer, ByRef DevHandle As IntPtr) As Integer
        End Function
        <DllImport("WMRAPI.dll")>
        Public Shared Function WM_CloseDevice(ByVal DevHandle As IntPtr) As Integer
        End Function
        <DllImport("WMRAPI.dll")>
        Public Shared Function WM_GetSerialNumber(ByVal DevHandle As IntPtr, ByVal DeviceSN As StringBuilder) As Integer
        End Function
        <DllImport("WMRAPI.dll")>
        Public Shared Function WM_GetImage(ByVal DevHandle As IntPtr, ByVal nTimeOut As Integer, ByVal ImageBuf() As Byte, <System.Runtime.InteropServices.Out()> ByRef Size As Integer) As Integer
        End Function
        <DllImport("WMRAPI.dll")>
        Public Shared Function WM_Extract(ByVal ImageBuf() As Byte, ByVal ImageWidth As Integer, ByVal ImageHeight As Integer, ByVal Feature() As Byte, <System.Runtime.InteropServices.Out()> ByRef Size As Integer) As Integer
        End Function

        <DllImport("WMRAPI.dll")>
        Public Shared Function WM_GetImageInfo(<System.Runtime.InteropServices.Out()> ByRef ImageWidth As Integer, <System.Runtime.InteropServices.Out()> ByRef ImageHeight As Integer) As Integer
        End Function

        <DllImport("WMRAPI.dll")>
        Public Shared Function WM_RawToBMP(ByVal ImageBuf() As Byte, ByVal ImageWidth As Integer, ByVal ImageHeight As Integer, ByVal BmpImageBuf() As Byte, <System.Runtime.InteropServices.Out()> ByRef Size As Integer) As Integer
        End Function

        <DllImport("WMRAPI.dll")>
        Public Shared Function WM_GenTemplate(ByVal Feature1() As Byte, ByVal Feature2() As Byte, ByVal Feature3() As Byte, ByVal Template() As Byte, <System.Runtime.InteropServices.Out()> ByRef Size As Integer) As Integer
        End Function

        <DllImport("WMRAPI.dll")>
        Public Shared Function WM_GenTemplateWithImage(ByVal Image() As IntPtr, ByVal nCount As Integer, ByVal nWidth As Integer, ByVal nHeight As Integer, ByVal Template() As Byte, <System.Runtime.InteropServices.Out()> ByRef Size As Integer) As Integer
        End Function
        <DllImport("WMRAPI.dll")>
        Public Shared Function WM_Verify(ByVal Template() As Byte, ByVal Feature() As Byte, <System.Runtime.InteropServices.Out()> ByRef Score As Integer) As Integer
        End Function
        '最终注册的特征，将用于demo中做对比，实际环境中则做永久存储
        Private registerTemplate(2047) As Byte
        Private Shared handle_Conflict As New IntPtr(0)
        '设置支持图像宽高度
        Private imageWidth, imageHeight As Integer
#End Region
#Region "初始化窗口部件"
        Public Sub New()
            InitializeComponent()
        End Sub

        Private Sub Form1_Load(ByVal sender As Object, ByVal e As EventArgs) Handles Me.Load
        End Sub
        ' 字节数组转16进制字符串 
        Public Shared Function byteToHexStr(ByVal bytes() As Byte, ByVal len As Integer) As String
            Dim returnStr As String = ""
            If bytes IsNot Nothing Then
                For i As Integer = 0 To len - 1
                    returnStr &= bytes(i).ToString("X2")
                Next i
            End If
            Return returnStr
        End Function
#End Region
#Region "局部变量"
#End Region
        Private Sub button1_Click(ByVal sender As Object, ByVal e As EventArgs) Handles button1.Click
            '打开设备
            Dim result As Integer = 0
            '获取设备数量
            result = WM_GetDeviceCount()
            If result <= 0 Then
                MessageBox.Show("当前环境没有任何指纹设备")
                Return
            End If
            textBox1.Text = textBox1.Text & "当前电脑中可用设备有" & result & "个"
            '初始化设备
            result = WM_Init()
            If result <> 0 Then
                textBox1.Text = textBox1.Text & vbCrLf & "初始化设备失败，错误码：" & result
                Return
            End If
            textBox1.Text = textBox1.Text & vbCrLf & "初始化设备成功.."

            Try
                '打开设备
                result = WM_OpenDevice(0, handle_Conflict)
                If result = 0 Then
                    textBox1.Text = textBox1.Text & vbCrLf & "打开设备成功.."
                    '获取设备图像信息
                    WM_GetImageInfo(imageWidth, imageHeight)
                End If
            Catch e2 As Exception
                textBox1.Text = textBox1.Text & vbCrLf & "打开设备失败," & e2.Message
            End Try
        End Sub

        Private Sub Form1_FormClosing(ByVal sender As Object, ByVal e As FormClosingEventArgs) Handles Me.FormClosing
            '关闭设备
            WM_CloseDevice(handle_Conflict)
            '释放设备
            WM_Free()
        End Sub

        Private Sub button3_Click(ByVal sender As Object, ByVal e As EventArgs) Handles button3.Click
            '开始注册
            Dim mythread As New Thread(AddressOf GetFeature)
            mythread.Start()
        End Sub

        Private Sub GetFeature()
            Dim registerTempFeatures As New List(Of Byte())()
            Dim imgPtr(2) As IntPtr

            Dim result As Integer = 0
            Dim flag As Boolean = False
            Dim imageBuf(299999) As Byte
            Dim bmpBytes(299999) As Byte
            Dim feature(2047) As Byte
            For i As Integer = 1 To 3
                SetText("请第" & i & "次按压手指..")
                'INSTANT VB NOTE: The variable size was renamed since Visual Basic does not handle local variables named the same as class members well:
                Dim size_Conflict As Integer = 0
                flag = False
                Do While Not flag
                    '获取指纹图像
                    result = WM_GetImage(handle_Conflict, 0, imageBuf, size_Conflict)
                    If result = 0 Then
                        '显示图像                    
                        Dim bmpSize As Integer = 0
                        result = WM_RawToBMP(imageBuf, imageWidth, imageHeight, bmpBytes, bmpSize)
                        If result <> 0 Then
                            SetText("图像质量不佳，请再次采集")
                            Continue Do
                        End If
                        imgPtr(i - 1) = Marshal.AllocHGlobal(bmpSize)
                        Marshal.Copy(bmpBytes, 0, imgPtr(i - 1), bmpSize)
                        SetImage(bmpBytes)
                        SetText("按压成功，已获取第" & i & "次图像")
                        Exit Do
                    End If
                    Thread.Sleep(100)
                    'Console.WriteLine("get feature " + i + " result " + result +" size "+ size);
                Loop
            Next i
            'registerTempFeatures中已包含3次图像获取，可合成模板
            Dim templateSize As Integer = 0
            '合成指纹模板
            result = WM_GenTemplateWithImage(imgPtr, 3, imageWidth, imageHeight, registerTemplate, templateSize)
            If result = 0 Then
                SetText("注册指纹模板已生成")
                'string str = Convert.ToBase64String(registerTemplate);
                Dim str As String = byteToHexStr(registerTemplate, templateSize)
                SetText("注册指纹模板数据:" & str)
            Else
                SetText("指纹模板合成失败，请重新点击 开始注册 按钮")
            End If
            '释放资源
            For j As Integer = 0 To 2
                Marshal.FreeHGlobal(imgPtr(j))
            Next j
            '释放设备
            WM_Free()
        End Sub

        Private Delegate Sub SetTextCallback(ByVal text As String)
        '在给textBox1.text赋值的地方调用以下方法即可
        Private Sub SetText(ByVal text_Conflict As String)
            ' InvokeRequired需要比较调用线程ID和创建线程ID
            ' 如果它们不相同则返回true
            If Me.textBox1.InvokeRequired Then
                Dim d As New SetTextCallback(AddressOf SetText)
                Me.Invoke(d, New Object() {text_Conflict})
            Else
                Me.textBox1.Text = textBox1.Text & vbCrLf & text_Conflict
                Me.textBox1.Focus() '获取焦点
                Me.textBox1.Select(Me.textBox1.TextLength, 0) '光标定位到文本最后
                Me.textBox1.ScrollToCaret() '滚动到光标处t
            End If
        End Sub

        Private Delegate Sub SetImageCallback(ByVal bmpBytes() As Byte)
        '在给textBox1.text赋值的地方调用以下方法即可
        Private Sub SetImage(ByVal bmpBytes() As Byte)
            ' InvokeRequired需要比较调用线程ID和创建线程ID
            ' 如果它们不相同则返回true
            If Me.textBox1.InvokeRequired Then
                Dim d As New SetImageCallback(AddressOf SetImage)
                Me.Invoke(d, New Object() {bmpBytes})
            Else
                Dim ms As New MemoryStream(bmpBytes)
                Dim returnImage As Image = Image.FromStream(ms)
                Me.pictureBox1.Image = returnImage
            End If
        End Sub
        Private Sub button5_Click(ByVal sender As Object, ByVal e As EventArgs) Handles button5.Click '开始比对
            Dim mythread As New Thread(AddressOf DoVerify)
            mythread.Start()
        End Sub
        Private Sub DoVerify()
            SetText("请按压手指进行比对..")
            Dim size_Conflict As Integer = 0
            Dim result As Integer = 0
            Dim imageBuf(299999) As Byte
            Dim bmpBytes(299999) As Byte
            Dim feature(2047) As Byte
            Do While size_Conflict = 0
                '获取指纹图像
                result = WM_GetImage(handle_Conflict, 0, imageBuf, size_Conflict)
                If result = 0 Then
                    '显示图像                    
                    Dim bmpSize As Integer = 0
                    result = WM_RawToBMP(imageBuf, imageWidth, imageHeight, bmpBytes, bmpSize)
                    SetImage(bmpBytes)
                End If
                Thread.Sleep(100)
                'Console.WriteLine("get feature " + i + " result " + result +" size "+ size);
            Loop
            '提取指纹特征
            result = WM_Extract(bmpBytes, imageWidth, imageHeight, feature, size_Conflict)
            If result <> 0 Then
                SetText("比对指纹特征提取失败，错误码：" & result)
                Return
            End If
            'string str = Convert.ToBase64String(feature);
            Dim str As String = byteToHexStr(feature, size_Conflict)
            SetText("比对指纹特征数据:" & str)
            Dim score As Integer = 0
            '比对指纹模板和特征
            result = WM_Verify(registerTemplate, feature, score)
            If result = 0 Then
                SetText("比对成功，比对分数：" & score)
            Else
                SetText("比对失败，如需重试请点击 开始比对 按钮")
            End If
            '释放设备
            WM_Free()
        End Sub
    End Class
End Namespace
