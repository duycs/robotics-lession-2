# 3D Tower Defense - CHAI3D Game Simulation

Dự án game **Bảo vệ tháp 3D (3D Tower Defense)** được mô phỏng và phát triển bằng thư viện mô phỏng 3D & haptic **CHAI3D**, kết hợp với **OpenGL** và **GLFW**.

---

## 1. Kiến trúc & Tính năng Chương trình

### Ụ súng Động học 2-DOF (2-DOF Kinematic Turret)
- **Base Mesh (Parent)**:
  - Đại diện cho phần bệ ụ súng xoay quanh trục đứng (Yaw axis - Z).
  - Góc xoay Yaw được giới hạn ngắt cứng trong khoảng **120°** (\([-60^\circ, +60^\circ]\)).
- **Gun Mesh (Child)**:
  - Đại diện cho nòng súng, được gắn là đối tượng con của Base (`m_baseMesh->addChild(m_gunMesh)`).
  - Gun tự động kế thừa góc xoay Yaw từ Base và xoay góc Pitch quanh trục ngang (X) với giới hạn **60°** (\([-30^\circ, +30^\circ]\)).
- **Tính toán Hướng bắn (Barrel Tip & Direction)**:
  - Sử dụng ma trận biến đổi toàn cục `m_gunMesh->getGlobalTransform()` để tính toán chính xác vị trí đầu nòng súng \(\vec{p}_{tip}\) và hướng bắn tuyến tính \(\vec{d}_{forward}\) trong không gian 3D.

### Quái thú 3D Uốn lượn & Tăng tốc (Smooth Wave Monster)
- **Hình học 3D**: Mô hình Sphere 3D metallic với vật liệu màu Crimson rực rỡ.
- **Chuyển động uốn lượn 3D mượt mà**: Quái thú di chuyển tịnh tiến từ xa về phía Ụ súng theo quỹ đạo sóng Sin 3D (uốn lượn ngang trái-phải và nhấp nhô dọc lên-xuống).
- **Tốc độ tinh chỉnh**: Tốc độ di chuyển và tần số sóng uốn lượn được giảm chậm mượt mà (\(v_{base} = 0.10\,\text{m/s}\)), giúp người chơi thoải mái ngắm bắn.
- **Hiệu ứng sát thương**: Quái thú có **10 máu (HP)**. Mỗi cú bắn trúng làm giảm 1 HP và chuyển đổi màu sắc từ đỏ thẫm sang xám nhạt dần.

### Quy tắc Thắng / Thua cuộc (Win / Loss Rules)
- **NGƯỜI CHƠI THẮNG (VICTORY)**: Người chơi bắn trúng **10 viên đạn** tiêu diệt hoàn toàn quái thú trước khi nó di chuyển qua ụ súng.
- **NGƯỜI CHƠI THUA (DEFEAT)**: Quái thú di chuyển uốn lượn vượt qua vị trí **Ụ súng (`Y <= -7.0m`)**.

### Giao diện HUD & Đo đạc Telemetry Thực thời
- **Căn lề thẳng hàng bên trái màn hình (`X = 20px`)**:
  - Tiêu đề game, trạng thái trận đấu, điểm số, HP quái thú (10 HP), thời gian combat.
  - Góc xoay 2-DOF & cảnh báo giới hạn khớp (`SPRING LIMIT!`).
  - Khối thông số đo đạc Telemetry & Haptics: Lực phản hồi \(F\) (N), Khoảng cách tới quái thú \(D\) (m), Vận tốc quái \(V\) (m/s), Tọa độ 3D Muzzle đầu nòng.
- **Bảng Thông Báo Kết Quả**: Tự động tính toán vị trí tâm và hiển thị rực rỡ ở **chính giữa trung tâm màn hình**.

---

## 2. Hướng dẫn Biên dịch & Chạy Chương trình

### Yêu cầu hệ thống
- **CMake**: Phiên bản 3.16 trở lên.
- **Trình biên dịch C++**: Hỗ trợ C++17 (Clang, GCC, hoặc MSVC).
- **Hệ điều hành**: macOS (Apple Silicon/Intel), Linux, hoặc Windows.

### Chạy nhanh 1 bước bằng Script
```bash
./run.sh
```

### Hoặc các bước biên dịch thủ công

1. Mở terminal tại thư mục gốc của dự án:
   ```bash
   cd /Users/duycs/Projects/github/robotics-lession-2
   ```

2. Tạo thư mục build và cấu hình CMake:
   ```bash
   mkdir -p build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   ```

3. Biên dịch và chạy game:
   ```bash
   cmake --build . --target TowerDefenseCHAI3D -j8
   ./TowerDefenseCHAI3D
   ```

---

## 3. Bảng Phím Điều khiển (Controls)

| Phím | Thao tác | Chức năng |
| :--- | :--- | :--- |
| `A` | Giữ phím | Xoay Base sang Trái (Giới hạn -60°) |
| `D` | Giữ phím | Xoay Base sang Phải (Giới hạn +60°) |
| `W` | Giữ phím | Xoay Gun Ngẩng lên (Giới hạn +30°) |
| `S` | Giữ phím | Xoay Gun Hạ xuống (Giới hạn -30°) |
| `SPACE` | Nhấn phím | Bắn đạn tuyến tính tốc độ cao (có phản hồi lực Haptics) |
| `N` | Nhấn phím | Bắt đầu lượt chơi mới (New Round) |
| `Q` / `ESC` | Nhấn phím | Thoát khỏi trò chơi |
