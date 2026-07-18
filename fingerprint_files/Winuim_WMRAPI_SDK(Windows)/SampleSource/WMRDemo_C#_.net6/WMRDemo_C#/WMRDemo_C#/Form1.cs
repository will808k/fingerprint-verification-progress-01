using System.Reflection.Metadata;
using System.Runtime.InteropServices;
using System.Text;

namespace WMRDemo
{
    public partial class Form1 : Form
    {
        #region 函数引用声明
        [DllImport("WMRAPI.dll")]
        public static extern int WM_Init();

        [DllImport("WMRAPI.dll")]
        public static extern int WM_Free();

        [DllImport("WMRAPI.dll")]
        public static extern int WM_GetDeviceCount();

        [DllImport("WMRAPI.dll")]
        public static extern int WM_OpenDevice(int nDevIndex, ref IntPtr DevHandle);

        [DllImport("WMRAPI.dll")]
        public static extern int WM_CloseDevice(IntPtr DevHandle);

        [DllImport("WMRAPI.dll")]
        public static extern int WM_GetSerialNumber(IntPtr DevHandle, StringBuilder DeviceSN);


        [DllImport("WMRAPI.dll")]
        public static extern int WM_GetImage(IntPtr DevHandle, int nTimeOut, byte[] ImageBuf, out int Size);


        [DllImport("WMRAPI.dll")]
        public static extern int WM_Extract(byte[] ImageBuf, int ImageWidth, int ImageHeight, byte[] Feature, out int Size);

        [DllImport("WMRAPI.dll")]
        public static extern int WM_GetImageInfo(out int ImageWidth, out int ImageHeight);

        [DllImport("WMRAPI.dll")]
        public static extern int WM_RawToBMP(byte[] ImageBuf, int ImageWidth, int ImageHeight, byte[] BmpImageBuf, out int Size);

        [DllImport("WMRAPI.dll")]
        public static extern int WM_GenTemplate(byte[] Feature1, byte[] Feature2, byte[] Feature3, byte[] Template, out int Size);

        [DllImport("WMRAPI.dll")]
        public static extern int WM_GenTemplateWithImage(IntPtr[] Image, int nCount, int nWidth, int nHeight, byte[] Template, out int Size);


        [DllImport("WMRAPI.dll")]
        public static extern int WM_Verify(byte[] Template, byte[] Feature, out int Score);
        #endregion
        public Form1()
        {
            InitializeComponent();
        }

        #region 局部变量

        //最终注册的特征，将用于demo中做对比，实际环境中则做永久存储
        private byte[] registerTemplate = new byte[2048];
        //设备句柄
        private static IntPtr handle = (IntPtr)0;
        //设置支持图像宽高度
        int imageWidth, imageHeight;
        #endregion

        /// <summary> 
        /// 字节数组转16进制字符串 
        /// </summary> 
        /// <param name="bytes"></param> 
        /// <returns></returns> 
        public static string byteToHexStr(byte[] bytes, int len)
        {
            string returnStr = "";
            if (bytes != null)
            {
                for (int i = 0; i < len; i++)
                {
                    returnStr += bytes[i].ToString("X2");
                }
            }
            return returnStr;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            int result = 0;
            //获取设备数量
            result = WM_GetDeviceCount();
            if (result <= 0)
            {
                MessageBox.Show("当前环境没有任何指纹设备");
                return;
            }
            textBox1.Text = textBox1.Text + "当前电脑中可用设备有" + result + "个";
            //初始化设备
            result = WM_Init();
            if (result != 0)
            {
                textBox1.Text = textBox1.Text + "\r\n" + "初始化设备失败，错误码：" + result;
                return;
            }
            textBox1.Text = textBox1.Text + "\r\n" + "初始化设备成功..";

            try
            {
                //打开设备
                result = WM_OpenDevice(0, ref handle);
                if (result == 0)
                {
                    textBox1.Text = textBox1.Text + "\r\n" + "打开设备成功..";
                    //获取设备图像信息
                    WM_GetImageInfo(out imageWidth, out imageHeight);
                }
            }
            catch (Exception e2)
            {
                textBox1.Text = textBox1.Text + "\r\n" + "打开设备失败," + e2.Message;
            }
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            //关闭设备
            WM_CloseDevice(handle);
            //释放设备
            WM_Free();
        }

        private void GetFeature()
        {
            List<byte[]> registerTempFeatures = new List<byte[]>();
            IntPtr[] imgPtr = new IntPtr[3];

            int result = 0;
            bool flag = false;
            byte[] imageBuf = new byte[300000];
            byte[] bmpBytes = new byte[300000];
            byte[] feature = new byte[2048];
            for (int i = 1; i <= 3; i++)
            {
                SetText("请第" + i + "次按压手指..");
                int size = 0;
                flag = false;
                while (!flag)
                {
                    //获取指纹图像
                    result = WM_GetImage(handle, 0, imageBuf, out size);
                    if (result == 0)
                    {
                        //显示图像                    
                        int bmpSize = 0;
                        result = WM_RawToBMP(imageBuf, imageWidth, imageHeight, bmpBytes, out bmpSize);
                        if (result != 0)
                        {
                            SetText("图像质量不佳，请再次采集");
                            continue;
                        }
                        imgPtr[i - 1] = Marshal.AllocHGlobal(bmpSize);
                        Marshal.Copy(bmpBytes, 0, imgPtr[i - 1], bmpSize);
                        SetImage(bmpBytes);
                        SetText("按压成功，已获取第" + i + "次图像");
                        break;
                    }
                    Thread.Sleep(100);
                    //Console.WriteLine("get feature " + i + " result " + result +" size "+ size);
                }
            }

            //registerTempFeatures中已包含3次图像获取，可合成模板
            int templateSize = 0;
            //合成指纹模板
            result = WM_GenTemplateWithImage(imgPtr, 3, imageWidth, imageHeight, registerTemplate, out templateSize);
            if (result == 0)
            {
                SetText("注册指纹模板已生成");
                //string str = Convert.ToBase64String(registerTemplate);
                string str = byteToHexStr(registerTemplate, templateSize);
                SetText("注册指纹模板数据:" + str);
            }
            else
            {
                SetText("指纹模板合成失败，请重新点击 开始注册 按钮");
            }
            //释放资源
            for (int j = 0; j < 3; j++)
            {
                Marshal.FreeHGlobal(imgPtr[j]);
            }

        }

        private delegate void SetTextCallback(string text);
        //在给textBox1.text赋值的地方调用以下方法即可
        private void SetText(string text)
        {
            // InvokeRequired需要比较调用线程ID和创建线程ID
            // 如果它们不相同则返回true
            if (this.textBox1.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(SetText);
                this.Invoke(d, new object[] { text });
            }
            else
            {
                this.textBox1.Text = textBox1.Text + "\r\n" + text;
                this.textBox1.Focus();//获取焦点
                this.textBox1.Select(this.textBox1.TextLength, 0);//光标定位到文本最后
                this.textBox1.ScrollToCaret();//滚动到光标处t
            }
        }

        private delegate void SetImageCallback(byte[] bmpBytes);
        //在给textBox1.text赋值的地方调用以下方法即可
        private void SetImage(byte[] bmpBytes)
        {
            // InvokeRequired需要比较调用线程ID和创建线程ID
            // 如果它们不相同则返回true
            if (this.textBox1.InvokeRequired)
            {
                SetImageCallback d = new SetImageCallback(SetImage);
                this.Invoke(d, new object[] { bmpBytes });
            }
            else
            {
                MemoryStream ms = new MemoryStream(bmpBytes);
                Image returnImage = Image.FromStream(ms);
                this.pictureBox1.Image = returnImage;
            }
        }

        private void DoVerify()
        {
            SetText("请按压手指进行比对..");
            int size = 0;
            int result = 0;
            byte[] imageBuf = new byte[300000];
            byte[] bmpBytes = new byte[300000];
            byte[] feature = new byte[2048];
            while (size == 0)
            {
                //获取指纹图像
                result = WM_GetImage(handle, 0, imageBuf, out size);
                if (result == 0)
                {
                    //显示图像                    
                    int bmpSize = 0;
                    result = WM_RawToBMP(imageBuf, imageWidth, imageHeight, bmpBytes, out bmpSize);
                    SetImage(bmpBytes);
                }
                Thread.Sleep(100);
                //Console.WriteLine("get feature " + i + " result " + result +" size "+ size);
            }
            //提取指纹特征
            result = WM_Extract(bmpBytes, imageWidth, imageHeight, feature, out size);
            if (result != 0)
            {
                SetText("比对指纹特征提取失败，错误码：" + result);
                return;
            }
            //string str = Convert.ToBase64String(feature);
            string str = byteToHexStr(feature, size);
            SetText("比对指纹特征数据:" + str);
            int score = 0;
            //比对指纹模板和特征
            result = WM_Verify(registerTemplate, feature, out score);
            if (result == 0)
            {
                SetText("比对成功，比对分数：" + score);
            }
            else
                SetText("比对失败，如需重试请点击 开始比对 按钮");
        }

        private void button2_Click(object sender, EventArgs e)
        {
            Thread mythread = new Thread(GetFeature);
            mythread.Start();
        }

        private void button3_Click(object sender, EventArgs e)
        {
            Thread mythread = new Thread(DoVerify);
            mythread.Start();
        }

        private void pictureBox1_Click(object sender, EventArgs e)
        {

        }
    }
}
