import sqlite3
import threading
import tkinter
from tkinter import *
import tkinter.scrolledtext
from tkinter import ttk, simpledialog
from PIL import Image, ImageTk
from ctypes import *
from legacy import xrange
from io import BytesIO

import tkinter.messagebox as msgbox
# 导入dll
add_path="C:\\Users\\Public\\Documents\\Winuim\\Chrome\\"
WMdll = WinDLL(add_path + "\\WMRAPI.dll")  ##调用dll文件

# 定义全局变量
global root, img, nr, isOpen, isRunning, ImgPtr, HANDLE
global imageWidth, imageHeight,feature, id,mode,gets

# 初始化全局变量
isOpen = False  # 指纹仪是否打开
isRunning = False  # 是否正在运行线程
root = Tk()  # 定义一个tkinter图形化界面
nr = ""  # 信息框内容
img = None  # 显示的图片
ImgPtr = []  # 指纹模板的图片列表
HANDLE = c_void_p(0)  # 句柄初始化
imageWidth = c_int()  # 指纹图片宽度
imageHeight = c_int()  # 指纹图片高度
feature = create_string_buffer(2048)  # 创建2048字节长的指纹特征，且初始化为0
id = 1
mode=0 #0插入,1更新
gets=None


# 图形化界面
def UI():
    global root  # 使用全局变量
    root.title('WMRAPI')  # 窗口名称
    root.geometry('750x660')  # 窗口大小，这里的乘号不是 * ，而是小写英文字母 x
    root.resizable(0, 0)  # 固定窗口大小，不能随意伸缩
    ButtonGroup(root)  # 调用按钮组
    messageText(root)  # 调用信息框
    showImage(root)  # 调用图片框
    FingerprintLibrary()
    root.mainloop()  # 显示图形化界面

# 按钮组
def ButtonGroup(root):
    mean=means()
    # command为按钮的点击事件，不能加括号，如command=openButton_Click而不是command=openButton_Click()
    state = tkinter.DISABLED
    font=(None, 12)
    openButton = Button(root, text='打开设备', width=12, height=1, font=font, command=openButton_Click)
    featureButton = Button(root, text='注册',state = state, width=12, height=1, font=font, command=featureButton_Click)
    onCompareFingerButton = Button(root, text='1:1比对',state = state, width=12, height=1, font=font, command=onCompareFingerButton_Click)
    onCompareIdentifyButton = Button(root, text='1:N识别',state = state, width=12, height=1, font=font, command=onCompareIdentifyButton_Click)
    onCompareIdentifyNButton = Button(root, text='1:N查重', state=state, width=12, height=1, font=font,command=onCompareIdentifyNButton_Click)


    closeButton = Button(root, text='关闭设备',state = state, width=12, height=1, font=font, command=mean.Close)
    stopButton = Button(root, text='停止',state = state, width=12, height=1, font=font, command=mean.Stop)
    SingleDeleteButton = Button(root, text='单一删除',state = state, width=12, height=1, font=font, command=mean.SingleDelete)
    AllDeleteButton = Button(root, text='清空指纹库', state=state, width=12, height=1, font=font,command=mean.AllDelete)
    exitButton = Button(root, text='退出', width=12, height=1, font=font, command=mean.Exit)

    # 按钮的位置，范围0-1
    openButton.place(relx=0.025, rely=0.03)
    featureButton.place(relx=0.025, rely=0.11)
    onCompareFingerButton.place(relx=0.025, rely=0.19)
    onCompareIdentifyButton.place(relx=0.025, rely=0.27)
    onCompareIdentifyNButton.place(relx=0.025, rely=0.35)


    closeButton.place(relx=0.25, rely=0.03)
    stopButton.place(relx=0.25, rely=0.11)
    SingleDeleteButton.place(relx=0.25, rely=0.19)
    AllDeleteButton.place(relx=0.25, rely=0.27)
    exitButton.place(relx=0.25, rely=0.35)
    return [featureButton,onCompareFingerButton,onCompareIdentifyButton,onCompareIdentifyNButton,closeButton,stopButton,SingleDeleteButton,AllDeleteButton]

def openButton_Click():
    mean=means()
    open_thread = threading.Thread(target=mean.Begin)
    open_thread.start()

def featureButton_Click():
    global id,mode
    mean = means()
    entry_int=Input_id()
    if entry_int!=None:
        db=Sqlite3()
        sql = "select id from Fingers where id = "+str(entry_int)+";"
        FingerID=db.Select(sql)
        #当前用户已存在
        if FingerID==None:
            mode = 0
            id = entry_int
            feature_thread = threading.Thread(target=mean.Feature)
            feature_thread.start()
        else:
            warning = msgbox.askokcancel('确认操作', '该用户已存在，是否覆盖？')  # 返回值true/false
            if warning == True:
                mode = 1
                id = entry_int
                feature_thread = threading.Thread(target=mean.Feature)
                feature_thread.start()

def onCompareFingerButton_Click():
    mean=means()
    verify_thread = threading.Thread(target=mean.onCompareFinger)
    verify_thread.start()

def onCompareIdentifyButton_Click():
    mean=means()
    verify_thread = threading.Thread(target=mean.onCompareIdentify)
    verify_thread.start()

def onCompareIdentifyNButton_Click():
    mean=means()
    verify_thread = threading.Thread(target=mean.onCompareIdentifyN)
    verify_thread.start()

#设置按钮是否可用
def anConfig(root,state):
    button=ButtonGroup(root)
    for i in button:
        i.config(state=state)

# 信息框
def messageText(root):
    text = tkinter.scrolledtext.ScrolledText(root, width=45, height=20)  # 设置信息框样式
    text.place(relx=0.48, rely=0.55)
    text.see(END)  # 信息框处于滚动条最下面信息的位置
    return text

# 插入信息
def message(text, result):
    text.insert(END, result)  # 将信息result插入信息框最末尾
    text.see(END)  # 将滚动条置于末尾
    text.update()  # 更新信息框

# 图片框
def showImage(root):
    labFrame = LabelFrame(root, text="图像显示", relief=GROOVE, width=320, height=330)  # 设置图片样式
    labFrame.place(relx=0.48, rely=0.02)
    lableShowImage = Label(labFrame)  # 包含图片的标签
    lableShowImage.place(relx=0, rely=0)
    return lableShowImage

class FingerprintLibrary:
    # 指纹库表格
    def __init__(self):
        self.Finger = ttk.Treeview(root, columns=("FingerID", "Template"), show="headings",height=16)  # #创建表格对象
        self.Finger.heading("FingerID", text="FingerID")  # 显示表头
        self.Finger.heading("Template", text="Template")
        self.Finger.column("FingerID", width=100)  # #设置列
        self.Finger.column("Template", width=200)
        self.Finger.place(relx=0.025, rely=0.425)
        self.Finger.bind('<ButtonRelease-1>', self.treeviewClick)  # 绑定单击离开事件===========

    # 获取当前点击行的值
    def treeviewClick(self,event):  # 单击
        global gets,getData
        for item in  self.Finger.selection():
            item_text = self.Finger.item(item, "values")
            gets=item_text[0]
            print(item_text[0])

    def Select(self):
        global gets
        db=Sqlite3()
        data = db.SelectAll()
        for i in data:
            self.Finger.insert("", i[0], values=(i[0], i[1].hex().upper()))
        gets=None

class Sqlite3:
    def __init__(self):
        self.Start()
        self.Create()
        self.Close()

    # 连接WMRAPI数据库，如果没有则会创建该数据库
    def Start(self, path='FPdata.db'):
        self.conn = sqlite3.connect(path)
        self.cursor = self.conn.cursor()

    # 创建数据表
    def Create(self):
        sql = '''create table if not exists Fingers(id int primary key not null,data text not null);'''  # 如果数据表Fingers不存在则创建数据表
        self.cursor.execute(sql)

    def Close(self):
        self.cursor.close()
        self.conn.close()

    # 插入数据表
    def Insert(self, id, data):
        try:
            self.Start()
            sql = '''insert into Fingers(id,data) values(?,?);'''  # 如果数据表Fingers不存在则创建数据表
            self.cursor.execute(sql, (id, data))
            self.conn.commit()
            self.Close()
            return 1
        except Exception as e:
            print('>> Insert Error:', e)
            return 0


    # 查询数据
    def Select(self, sql):
        self.Start()
        self.cursor.execute(sql)
        # 获取结果
        result = self.cursor.fetchone()
        self.Close()
        return result

    # 显示数据表
    def SelectAll(self):
        self.Start()
        sql = '''select * from Fingers;'''  # 如果数据表Fingers不存在则创建数据表
        self.cursor.execute(sql)
        # 获取结果集
        data_all = self.cursor.fetchall()
        self.Close()
        return data_all

    # 更新数据
    def Update(self,id,data):
        self.Start()
        sql = '''UPDATE Fingers SET data = ? WHERE id = ?;'''  # 如果数据表Fingers不存在则创建数据表
        self.cursor.execute(sql, (data, id))
        self.conn.commit()
        self.Close()
        return 1

    # 删除数据
    def Delete(self, id):
        self.Start()
        sql = '''DELETE FROM Fingers WHERE id = ?;'''  # 如果数据表Fingers不存在则创建数据表
        self.cursor.execute(sql, (id,))
        self.conn.commit()
        self.Close()
        return 1

    # 删除数据
    def DeleteAll(self):
        self.Start()
        sql = '''DELETE FROM Fingers;'''  # 如果数据表Fingers不存在则创建数据表
        self.cursor.execute(sql)
        self.conn.commit()
        self.Close()
        return 1

#注册FingerID弹出框
def Input_id():
    db=Sqlite3()
    sql = '''SELECT * FROM Fingers ORDER BY id DESC LIMIT 1;'''  # 获取id最大的数据
    if db.Select(sql) != None:
        FingerID = db.Select(sql)[0]+1
    else:
        FingerID=1
    # 输入整数
    entry_int = simpledialog.askinteger(title='设置FingerID', prompt='FingerID',initialvalue=FingerID)
    return entry_int

class means:
    # 打开设备
    def Begin(self):
        global nr, isOpen, isRunning
        # 在信息框中显示之前已有的信息
        text = messageText(root)
        text.insert(END, nr)
        if isOpen == False:
            result = WMdll.WM_GetDeviceCount()
            if result == 0:
                re = "当前环境没有任何指纹设备\r\n"
            else:
                re = "当前电脑中可用设备有" + str(result) + "个\r\n"
                result = WMdll.WM_Init()
                if result != 0:
                    re = re + "初始化设备失败，错误码：" + str(result) + "\r\n"
                else:
                    re = re + "初始化设备成功..\r\n"
            try:
                # 打开设备
                WMRAPI=WMRAPI_Dll()
                Open = WMRAPI.OpenDevice()
                if (Open[0] == 0):
                    re = re + "打开设备成功..\r\n"
                    anConfig(root, tkinter.NORMAL)  #启用按钮
                    Fingerprint = FingerprintLibrary()
                    Fingerprint.Select()  #显示指纹库数据
                    isOpen = True
                    WMRAPI.GetSerialNumber()
                    re = re + "获取设备序列号:" + str(WMRAPI.GetSerialNumber()[2]) + "\r\n"
                    # 获取图像信息
                    WMRAPI.GetImageInfo()

            except Exception as e:
                print(e)
                re = re + "打开设备失败,\r\n"
        else:
            re = "设备已打开\r\n"
        nr = nr + re  # 更新保存的信息整体内容
        message(text, re)  # 将新信息追加到信息框

    # 注册
    def Feature(self):
        global nr, img, isOpen, isRunning
        isRunning = False
        text = messageText(root)  # 函数需要使用信息框
        text.insert(END, nr)
        if isOpen == True:
            re = self.Collect()  # 采集指纹
            if re != 0:  # 采集指纹中途不曾停止，指纹图片数量足够
                self.GetFeature()  # 合成模板
            isRunning = False  # 线程停止
        else:
            result = "指纹设备未打开，无法注册..\r\n"
            nr = nr + result
            message(text, result)

    # 1:1比对
    def onCompareFinger(self):
        global nr, img, isOpen, isRunning,gets
        text = messageText(root)
        text.insert(END, nr)
        images = showImage(root)
        isRunning = False
        if gets==None:
            result = "请选择要比对的指纹模板..\r\n"
            nr = nr + result
            message(text, result)  # 更新信息框
            images.config(image=img)  # 显示最新一张图片在图片框
        else:
            result = "请按压手指进行比对..\r\n"
            nr = nr + result
            WMRAPI = WMRAPI_Dll()
            images.config(image=img)  # 显示最新一张图片在图片框
            message(text, result)  # 更新信息框
            result = self.meansDoVerify()  # 获取比对信息
            if result[1] != 0:  # 获取到指纹特征图片
                nr = nr + result[0]
                db=Sqlite3()
                sql = "select data from Fingers where id = " + gets + ";"
                template = db.Select(sql)
                print(template[0])
                # 进行比对
                Ver = WMRAPI.Verify(template[0])
                if Ver[0] == 0:
                    re= result[0]+ "比对成功，比对分数：" + str(Ver[1].value) + "\r\n"
                    nr = nr +re
                else:
                    re= result[0]+ "比对失败，如需重试请点击 开始比对按钮，比对分数：" + str(Ver[1].value) + "\r\n"
                    nr = nr + re
                img = ImageTk.PhotoImage(result[1])  # 更新图片
                images.config(image=img)  # 显示最新一张图片
                message(text, re)  # 更新信息

    #1:N识别
    def onCompareIdentify(self):
        global nr, img, isOpen, isRunning
        text = messageText(root)
        text.insert(END, nr)
        images = showImage(root)
        isRunning = False

        result = "请按压手指进行识别..\r\n"
        nr = nr + result
        WMRAPI = WMRAPI_Dll()
        images.config(image=img)  # 显示最新一张图片在图片框
        message(text, result)  # 更新信息框
        result = self.meansDoVerify()  # 获取比对信息
        if result[1] != 0:  # 获取到指纹特征图片
            nr = nr + result[0]
            db = Sqlite3()
            templates = db.SelectAll()
            Score=0
            id=0
            for i in templates:
                # 进行比对
                print(i[1])
                Ver = WMRAPI.Verify(i[1])
                if Ver[0] == 0:
                    if Ver[1].value>Score:
                        Score=Ver[1].value
                        id=i[0]
            if Score==0:
                re = result[0] + "识别失败。。。\r\n"
                nr = nr + re
            else:
                re = result[0] + "识别成功，FingerID=" + str(id) + "，得分：" + str(Score) + "\r\n"
                nr = nr + re
            img = ImageTk.PhotoImage(result[1])  # 更新图片
            images.config(image=img)  # 显示最新一张图片
            message(text, re)  # 更新信息

    #1:N查重
    def onCompareIdentifyN(self):
        global nr, img, isOpen, isRunning
        text = messageText(root)
        text.insert(END, nr)
        images = showImage(root)
        isRunning = False

        result = "请按压手指进行查重..\r\n"
        nr = nr + result
        WMRAPI = WMRAPI_Dll()
        images.config(image=img)  # 显示最新一张图片在图片框
        message(text, result)  # 更新信息框
        result = self.meansDoVerify()  # 获取比对信息
        if result[1] != 0:  # 获取到指纹特征图片
            nr = nr + result[0]
            db = Sqlite3()
            templates = db.SelectAll()
            Score= {}
            for i in templates:
                # 进行比对
                Ver = WMRAPI.Verify(i[1])
                if Ver[0] == 0:
                    Score[i[0]]=Ver[1].value
                    Score = dict(sorted(Score.items(), key=lambda x: x[1], reverse=True))  # 排序
                if len(Score)>10:  #字典中超过十个元素时
                    Score.popitem()  # 将最后面的键对值弹出，即分数最小的弹出
                re = result[0] + "指纹识别成功！识别到以下"+str(len(Score))+"个用户:\r\n"
            if Score== {}: #空字典
                re = result[0] + "未识别到用户！\r\n"
                nr = nr + re
            else:
                for key in Score:
                    re = re+"FingerID="+str(key)+"，得分：" + str(Score[key]) + "\r\n"
                nr = nr + re
            img = ImageTk.PhotoImage(result[1])  # 更新图片
            images.config(image=img)  # 显示最新一张图片
            message(text, re)  # 更新信息


    # 关闭设备
    def Close(self):
        global nr, isOpen, isRunning
        text = messageText(root)  # 使用信息框
        if isOpen == True:
            result = WMdll.WM_CloseDevice(HANDLE)  # 关闭设备
            WMdll.WM_Free()  # 释放设备
            if result == 0:
                isOpen = False
                isRunning = False
                nr = nr + "关闭成功\r\n"
                anConfig(root, tkinter.DISABLED)  # 禁用按钮
                FingerprintLibrary()
            else:
                nr = nr + "关闭失败\r\n"
        else:
            nr = nr + "指纹设备未打开\r\n"
        message(text, nr)

    # 停止注册或比对
    def Stop(self):
        global nr, isRunning
        text = messageText(root)  # 使用信息框
        if isOpen == True:
            isRunning = False
            nr = nr + "已停止..\r\n"
        else:
            nr = nr + "指纹设备未打开\r\n"
        message(text, nr)  # 更新信息

    # 退出程序
    def Exit(self):
        global root,isRunning
        isRunning = False
        root.destroy()  # 销毁图形化界面

    def SingleDelete(self):
        global gets,isRunning
        db=Sqlite3()
        db.Delete(gets)
        Fingerprint=FingerprintLibrary()
        Fingerprint.Select()
        isRunning=False

    def AllDelete(self):
        global isRunning
        db=Sqlite3()
        db.DeleteAll()
        Fingerprint=FingerprintLibrary()
        Fingerprint.Select()
        isRunning = False
    # 收集指纹
    def Collect(self):
        global ImgPtr, isRunning, img, nr
        ImgPtr = []  # 每次点击注册都要初始化一次指纹列表
        text = messageText(root)  # 使用信息框
        images = showImage(root)  # 使用图片框
        text.insert(END, nr)  #
        flag = True
        isRunning = True
        WMRAPI=WMRAPI_Dll()
        try:
            for i in range(3):
                if img != None:
                    images.config(image=img)
                re = "请第" + str(i + 1) + "次按压手指..\r\n"
                nr = nr + re
                message(text, re)

                while flag:
                    result = WMRAPI.GetImage()
                    flag = isRunning
                    if result[0] == 0:
                        # 显示图像
                        BMP = WMRAPI.RawToBMP(result[2])
                        print("RawToBMP:" + str(BMP))
                        if BMP[0] != 0:
                            re = "图像质量不佳，请再次采集\r\n"
                            continue
                        ImgPtr.append(BMP[1].raw)  # 将图片的字节数组存入Imgptr
                        byte_stream = BytesIO(ImgPtr[i])  # 将二进制转为字节流
                        roiImg = Image.open(byte_stream)
                        re = "按压成功，已获取第" + str(i + 1) + "次图像\r\n"
                        break
                    if flag == False:
                        return 0
                nr = nr + re
                img = ImageTk.PhotoImage(roiImg)
                images.config(image=img)
                message(text, re)
                images.update()
        except Exception as e:
            print(e)
        return nr

    # 合成模板
    def GetFeature(self):
        global nr
        text = messageText(root)
        text.insert(END, nr)  # 显示前面保存的信息在信息框中
        Fingerprint = FingerprintLibrary()
        WMRAPI = WMRAPI_Dll()
        try:
            # 合成指纹模板
            result = WMRAPI.GenTemplateWithImage(ImgPtr)
            if result[0] == 0:
                re = "注册指纹模板已生成\r\n"
                str1 = result[1]
                re = re + "注册指纹模板数据:\r\n" + str1 + "\r\n"
                re=re+"用户添加成功，FingerID="+str(id)+ "\r\n"
                db = Sqlite3()
                if mode==0:
                    db.Insert(id, result[3])
                else:
                    db.Update(id,result[3])
                Fingerprint.Select()
            else:
                re = "指纹模板合成失败，请重新点击 开始注册按钮\r\n"
            nr = nr + re
            message(text, re)
        except Exception as e:
            print(e)

    # 指纹特征
    def meansDoVerify(self):
        global isRunning
        size = 0
        isRunning = True
        WMRAPI = WMRAPI_Dll()
        while size == 0:
            # 获取指纹图像
            result = WMRAPI.GetImage()
            size = result[3].value
            flag = isRunning
            if flag == False:
                return [None, 0]
            if result[0] == 0:
                # 显示图像
                bmp = WMRAPI.RawToBMP(result[2])
                print(bmp)
                byte_stream = BytesIO(bmp[1].raw)  # 将二进制转为字节流
                roiImg = Image.open(byte_stream)  # 转成图片

        # 提取指纹特征
        result = WMRAPI.Extract(bmp[1])
        if result[0] != 0:
            nr = "比对指纹特征提取失败，错误码：" + str(result[0]) + "\r\n"
            return nr
        str1 = result[1]
        nr = "比对指纹特征数据:\r\n" + str1 + "\r\n"
        return [nr, roiImg]


    def closeVerify(self):
        global isRunning
        if isOpen == True:
            isRunning = False
            re = "停止比对..\r\n"
            return [re, isRunning]
        else:
            re = "指纹设备未打开\r\n"
            return [re, isRunning]

class WMRAPI_Dll:
    # 打开设备
    def OpenDevice(self):
        # 打开设备
        global HANDLE
        print(HANDLE)
        WM_OpenDevice = WMdll.WM_OpenDevice  # 要调用的dll函数
        WM_OpenDevice.restype = c_int  # 设置函数的返回值类型
        WM_OpenDevice.argtypes = (c_int, POINTER(c_void_p))  # 设置函数的参数类型
        result = WM_OpenDevice(0, HANDLE)  # 调用dll函数
        return [result, HANDLE]

    # 获取图像信息
    def GetImageInfo(self):
        global imageWidth, imageHeight  # 使用全局变量
        WM_GetImageInfo = WMdll.WM_GetImageInfo
        WM_GetImageInfo.restype = c_int  # 设置函数的返回值类型
        WM_GetImageInfo.argtypes = (POINTER(c_int), POINTER(c_int))  # 设置函数的参数类型
        result = WM_GetImageInfo(imageWidth, imageHeight)
        return [result, imageWidth.value, imageHeight.value]

    #  获取设备序列号
    def GetSerialNumber(self):
        try:
            DeviceSN = create_string_buffer(40)  # 创建40字节长的DeviceSN，且初始化为0
            result = WMdll.WM_GetSerialNumber(HANDLE, DeviceSN)
            SN = DeviceSN.raw.decode().strip(b'\x00'.decode())  # 将DeviceSN转换成字符串且去掉后面多余的\x00
            return [result, HANDLE, SN]
        except Exception as e:
            print(e)

    # 获取指纹图像
    def GetImage(self):
        try:
            imageBuf = create_string_buffer(300000)  # 创建300000字节长的buf，且初始化为0
            size = c_int()
            Size = pointer(size)  # 指针
            result = WMdll.WM_GetImage(HANDLE, 0, imageBuf, Size)
            print("WM_GetImage:" + str(result), HANDLE, imageBuf, str(size))
            return [result, HANDLE, imageBuf, size]
        except Exception as e:
            print(e)

    # 图像格式转换( RAW 转 BMP )
    def RawToBMP(self,imageBuf):
        try:
            BmpImageBuf = create_string_buffer(300000)  # 创建300000字节长的buf，且初始化为0
            size = c_int()  # int类型
            Size = pointer(size)  # 指针
            result = WMdll.WM_RawToBMP(imageBuf, imageWidth, imageHeight, BmpImageBuf, Size)
            return [result, BmpImageBuf, size]
        except Exception as e:
            print(e)

    # 合成指纹模板
    def GenTemplateWithImage(self,ImgPtr):
        try:
            Image = (POINTER(c_char) * 3)()  # 定义指针数组
            template = create_string_buffer(2048)  # 创建2048字节长的指纹模板，且初始化为0
            Image[:] = [create_string_buffer(ImgPtr[i], 300000) for i in xrange(3)]  # 初始化指针数组，ImgPtr[i]内容为300000字节长
            size = c_int()
            Size = pointer(size)
            result = WMdll.WM_GenTemplateWithImage(Image, 3, imageWidth, imageHeight, template, Size)
            print("WM_GenTemplateWithImage:" + str(result), template, "Size:" + str(size))
            Template = template.raw[:size.value].hex()  # 将获取的模板从字节转为16进制字符串
            Template = Template.upper()  # 字母全大小
            return [result, Template, size, template]
        except Exception as e:
            print(e)

    # 提取指纹特征
    def Extract(self,imageBuf):
        try:
            global feature
            size = c_int()
            Size = pointer(size)
            result = WMdll.WM_Extract(imageBuf, imageWidth, imageHeight, feature, Size)
            print("WM_Extract:" + str(result), feature, str(size))
            Feature = feature.raw[:size.value].hex()  # 将字节转为16进制字符串
            Feature = Feature.upper()  # 字母全大小
            return [result, Feature, size]
        except Exception as e:
            print(e)

    # 指纹比对
    def Verify(self,templates):
        try:
            score = c_int()
            Score = pointer(score)
            template = create_string_buffer(templates)  # 创建2048字节长的指纹模板，且初始化为0
            result = WMdll.WM_Verify(template, feature, Score)
            print("WM_Verify:" + str(result), str(score))
            return [result, score]
        except Exception as e:
            print(e)

if __name__ == '__main__':
    UI()
