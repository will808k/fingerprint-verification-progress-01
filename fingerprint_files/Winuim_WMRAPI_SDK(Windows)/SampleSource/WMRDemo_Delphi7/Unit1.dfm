object Form1: TForm1
  Left = 377
  Top = 121
  Width = 525
  Height = 482
  Caption = 'Form1'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = #23435#20307
  Font.Style = []
  OldCreateOrder = False
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 12
  object Label1: TLabel
    Left = 200
    Top = 384
    Width = 289
    Height = 12
    AutoSize = False
    Caption = 'Label1'
  end
  object Panel1: TPanel
    Left = 200
    Top = 32
    Width = 289
    Height = 345
    Color = clWindow
    TabOrder = 0
    object Image1: TImage
      Left = 1
      Top = 1
      Width = 287
      Height = 343
      Align = alClient
      PopupMenu = PopupMenu1
    end
  end
  object Button1: TButton
    Left = 8
    Top = 40
    Width = 75
    Height = 25
    Caption = #25171#24320#35774#22791
    TabOrder = 1
    OnClick = Button1Click
  end
  object Button2: TButton
    Left = 104
    Top = 40
    Width = 75
    Height = 25
    Caption = #20851#38381#35774#22791
    TabOrder = 2
    OnClick = Button2Click
  end
  object Button3: TButton
    Left = 8
    Top = 80
    Width = 75
    Height = 25
    Caption = #27880#20876#25351#32441
    TabOrder = 3
    OnClick = Button3Click
  end
  object Button4: TButton
    Left = 104
    Top = 80
    Width = 75
    Height = 25
    Caption = #27604#23545#25351#32441
    TabOrder = 4
    OnClick = Button4Click
  end
  object Memo1: TMemo
    Left = 8
    Top = 152
    Width = 177
    Height = 225
    Lines.Strings = (
      'Memo1')
    ScrollBars = ssVertical
    TabOrder = 5
  end
  object PopupMenu1: TPopupMenu
    Left = 352
    Top = 96
    object SaveImage1: TMenuItem
      Caption = 'Save Image'
      OnClick = SaveImage1Click
    end
  end
  object SaveDialog1: TSaveDialog
    Left = 272
    Top = 56
  end
end
