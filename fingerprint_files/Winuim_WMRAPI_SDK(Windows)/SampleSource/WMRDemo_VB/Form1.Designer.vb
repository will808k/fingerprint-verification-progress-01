Namespace WMPlatformCSharpAppLocalDllDemo
	Partial Public Class Form1
		''' <summary>
		''' 必需的设计器变量。
		''' </summary>
		Private components As System.ComponentModel.IContainer = Nothing

		''' <summary>
		''' 清理所有正在使用的资源。
		''' </summary>
		''' <param name="disposing">如果应释放托管资源，为 true；否则为 false。</param>
		Protected Overrides Sub Dispose(ByVal disposing As Boolean)
			If disposing AndAlso (components IsNot Nothing) Then
				components.Dispose()
			End If
			MyBase.Dispose(disposing)
		End Sub

		#Region "Windows 窗体设计器生成的代码"

		''' <summary>
		''' 设计器支持所需的方法 - 不要
		''' 使用代码编辑器修改此方法的内容。
		''' </summary>
		Private Sub InitializeComponent()
            Me.button1 = New System.Windows.Forms.Button()
            Me.button3 = New System.Windows.Forms.Button()
            Me.button5 = New System.Windows.Forms.Button()
            Me.textBox1 = New System.Windows.Forms.TextBox()
            Me.groupBox1 = New System.Windows.Forms.GroupBox()
            Me.pictureBox1 = New System.Windows.Forms.PictureBox()
            Me.groupBox1.SuspendLayout()
            CType(Me.pictureBox1, System.ComponentModel.ISupportInitialize).BeginInit()
            Me.SuspendLayout()
            '
            'button1
            '
            Me.button1.Font = New System.Drawing.Font("宋体", 11.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(134, Byte))
            Me.button1.Location = New System.Drawing.Point(12, 11)
            Me.button1.Name = "button1"
            Me.button1.Size = New System.Drawing.Size(108, 32)
            Me.button1.TabIndex = 0
            Me.button1.Text = "打开设备"
            Me.button1.UseVisualStyleBackColor = True
            '
            'button3
            '
            Me.button3.Font = New System.Drawing.Font("宋体", 11.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(134, Byte))
            Me.button3.Location = New System.Drawing.Point(12, 57)
            Me.button3.Name = "button3"
            Me.button3.Size = New System.Drawing.Size(108, 32)
            Me.button3.TabIndex = 2
            Me.button3.Text = "开始注册"
            Me.button3.UseVisualStyleBackColor = True
            '
            'button5
            '
            Me.button5.Font = New System.Drawing.Font("宋体", 11.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(134, Byte))
            Me.button5.Location = New System.Drawing.Point(13, 103)
            Me.button5.Name = "button5"
            Me.button5.Size = New System.Drawing.Size(108, 32)
            Me.button5.TabIndex = 4
            Me.button5.Text = "开始比对"
            Me.button5.UseVisualStyleBackColor = True
            '
            'textBox1
            '
            Me.textBox1.Font = New System.Drawing.Font("宋体", 11.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(134, Byte))
            Me.textBox1.Location = New System.Drawing.Point(13, 159)
            Me.textBox1.Multiline = True
            Me.textBox1.Name = "textBox1"
            Me.textBox1.ReadOnly = True
            Me.textBox1.ScrollBars = System.Windows.Forms.ScrollBars.Both
            Me.textBox1.Size = New System.Drawing.Size(273, 279)
            Me.textBox1.TabIndex = 6
            Me.textBox1.WordWrap = False
            '
            'groupBox1
            '
            Me.groupBox1.Controls.Add(Me.pictureBox1)
            Me.groupBox1.Font = New System.Drawing.Font("宋体", 11.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(134, Byte))
            Me.groupBox1.Location = New System.Drawing.Point(292, 12)
            Me.groupBox1.Name = "groupBox1"
            Me.groupBox1.Size = New System.Drawing.Size(345, 432)
            Me.groupBox1.TabIndex = 7
            Me.groupBox1.TabStop = False
            Me.groupBox1.Text = "图像显示"
            '
            'pictureBox1
            '
            Me.pictureBox1.Location = New System.Drawing.Point(6, 20)
            Me.pictureBox1.Name = "pictureBox1"
            Me.pictureBox1.Size = New System.Drawing.Size(333, 406)
            Me.pictureBox1.TabIndex = 0
            Me.pictureBox1.TabStop = False
            '
            'Form1
            '
            Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 12.0!)
            Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
            Me.ClientSize = New System.Drawing.Size(649, 456)
            Me.Controls.Add(Me.groupBox1)
            Me.Controls.Add(Me.textBox1)
            Me.Controls.Add(Me.button5)
            Me.Controls.Add(Me.button3)
            Me.Controls.Add(Me.button1)
            Me.Name = "Form1"
            Me.Text = "WMR_VB.NET_Demo"
            Me.groupBox1.ResumeLayout(False)
            CType(Me.pictureBox1, System.ComponentModel.ISupportInitialize).EndInit()
            Me.ResumeLayout(False)
            Me.PerformLayout()

        End Sub

		#End Region

		Private WithEvents button1 As System.Windows.Forms.Button
		Private WithEvents button3 As System.Windows.Forms.Button
		Private WithEvents button5 As System.Windows.Forms.Button
		Private textBox1 As System.Windows.Forms.TextBox
		Private groupBox1 As System.Windows.Forms.GroupBox
		Private pictureBox1 As System.Windows.Forms.PictureBox
	End Class
End Namespace

