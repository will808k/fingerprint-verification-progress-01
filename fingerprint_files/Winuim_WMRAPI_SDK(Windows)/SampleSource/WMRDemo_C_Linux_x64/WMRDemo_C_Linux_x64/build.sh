#! /bin/sh

# exit if any error occur
set -e

C_SCR_PATH=./
C_INC_PATH=./

C_SCR_LIST="WMRDemo.c"

cd $C_SCR_PATH

export LD_LIBRARY_PATH=./
gcc $C_SCR_LIST -I"$C_INC_PATH" -ldl -L./ -lwmrapi -lm -o ./WMRDemo_C

echo "完成...请将编译后的程序与API库（so文件）拷贝到同一个目录中运行"
echo "注意： 运行时需要root权限"
