#!/bin/bash
# Script tự động biên dịch và chạy game 3D Tower Defense (CHAI3D)

set -e

# Chuyển tới thư mục gốc chứa script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "========================================================"
echo "   KHIEM DANG BIEN DICH VA KHOI CHAY TOWER DEFENSE 3D   "
echo "========================================================"

# Tạo thư mục build nếu chưa có
mkdir -p build
cd build

# Cấu hình CMake nếu chưa cấu hình
if [ ! -f "Makefile" ]; then
    echo "[1/3] Cau hinh CMake..."
    cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release ..
fi

# Biên dịch chương trình
echo "[2/3] Bien dich TowerDefenseCHAI3D..."
cmake --build . --target TowerDefenseCHAI3D -j8

# Khởi chạy trò chơi
echo "[3/3] Dang khoi chay game 3D..."
echo "========================================================"
./TowerDefenseCHAI3D
