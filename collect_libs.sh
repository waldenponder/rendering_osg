#!/bin/bash
# 用法: ./collect_libs.sh <你的可执行文件路径> <目标输出目录>

BINARY=$1
DEST_DIR=$2

if [ -z "$BINARY" ] || [ -z "$DEST_DIR" ]; then
    echo "用法: $0 <可执行文件> <输出目录>"
    exit 1
fi

mkdir -p "$DEST_DIR"

# 获取所有依赖库的绝对路径
LIBS=$(ldd "$BINARY" | grep '=>' | awk '{print $3}' | grep -v '^$')

for LIB in $LIBS; do
    if [ -f "$LIB" ]; then
        echo "正在复制: $LIB"
        # 复制实体文件
        cp -L "$LIB" "$DEST_DIR/"
        
        # 处理符号链接 (保留原始 .so 和 .so.x 的链接关系)
        BASENAME=$(basename "$LIB")
        DIRNAME=$(dirname "$LIB")
        find "$DIRNAME" -maxdepth 1 -type l -name "${BASENAME}*" -exec cp -P {} "$DEST_DIR/" \;
    fi
done

echo "✅ 所有依赖库已收集到 $DEST_DIR"