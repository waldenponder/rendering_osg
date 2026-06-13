#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 1. 设置常规动态库搜索路径
export LD_LIBRARY_PATH="$SCRIPT_DIR/lib:$LD_LIBRARY_PATH"

# 2. 【关键】告诉 Mesa 去本地的 dri 目录寻找 swrast_dri.so
export LIBGL_DRIVERS_PATH="$SCRIPT_DIR/lib/dri"

# 3. 强制使用软件渲染
export LIBGL_ALWAYS_SOFTWARE=1

echo "🚀 正在启动程序..."
exec "$SCRIPT_DIR/rendering_osg" "$@"