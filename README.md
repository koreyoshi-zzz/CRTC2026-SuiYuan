# 随缘 · CRTC2026 中期考核仓库

> 队伍：随缘  
> 队长：张川  
> 实训战队：GSing  
> 赛事：重庆大学机器人训练大赛 CRTC2026（新生组）

本仓库用于提交 CRTC2026 中期考核材料，包含技术文档、STM32 固件源码、机械 STEP 模型与演示视频。

---

## 仓库结构

```text
CRTC2026-SuiYuan-GitHub/
├── README.md                       # 本文件
├── .gitignore                      # Git 忽略规则
├── docs/
│   ├── CRTC2026_中期考核技术文档.md   # 中期考核主文档
│   └── images/                     # 文档配图
│       ├── system_block.png        # 整车系统框图
│       ├── power_sch.png           # 电源原理图
│       ├── software_flow.png       # 软件流程图
│       ├── pcb_top.png / pcb_bottom.png / pcb_3d.png
│       └── dim_length.jpg / dim_width.jpg / dim_height.jpg  # 尺寸实测照片
├── firmware/
│   └── CRTC_CAR/                   # STM32CubeIDE 工程
│       ├── Core/
│       │   ├── Inc/                # 头文件
│       │   │   ├── main.h
│       │   │   ├── motor.h         # 电机 / 麦轮解算接口
│       │   │   ├── ps2.h           # PS2 手柄接口
│       │   │   ├── servo.h         # 舵机接口
│       │   │   └── tim.h
│       │   ├── Src/                # 源文件
│       │   │   ├── main.c          # 主循环 + 安全逻辑
│       │   │   ├── motor.c         # DRV8833 + 麦轮逆运动学
│       │   │   ├── ps2.c           # GPIO 软件模拟 SPI
│       │   │   ├── servo.c         # TIM4 硬件 PWM 舵机
│       │   │   ├── debug_config.h  # 调试模式总开关（在 Inc/）
│       │   │   ├── debug_servo.c   # 舵机独立调试程序
│       │   │   ├── debug_motor.c   # 电机独立调试程序
│       │   │   ├── debug_ps2.c     # PS2 手柄独立调试程序
│       │   │   └── tim.c / gpio.c  # CubeMX 生成
│       │   └── Startup/
│       ├── Drivers/                # STM32 HAL 库（CubeMX 生成）
│       ├── CRTC_CAR.ioc           # CubeMX 配置文件
│       └── ...                    # 链接脚本、Eclipse 工程文件
├── mechanical/
│   ├── parts/                       # 9 个 STEP 机械零件
│   ├── images/                      # 模型截图（V1/V2）
│   └── README.md
├── pcb/                             # 嘉立创 EDA 工程与原理图
│   ├── CRTC_copy.eprj2
│   ├── CRTC_copy_schematic.pdf
│   └── README.md
└── media/                           # 演示视频
    ├── chassis_ps2.mp4              # 底盘遥控（1920×1080 / 30 fps / 12.8 s）
    ├── drop_test.mp4                # 跌落测试（2560×1440 / 120 fps / 20.9 s）
    ├── appearance_360.mp4           # 整车外形展示（1584×720 / 25 fps / 10.0 s）
    ├── servo_demo.mp4               # 舵机运动演示（1584×720 / 25 fps / 14.9 s）
    ├── demo_servo_mechanism_1.mp4   # 舵机机构想法演示 1（720p，仅参考）
    ├── demo_servo_mechanism_2.mp4   # 舵机机构想法演示 2（720p，仅参考）
    ├── pcb_demo.mp4                 # PCB 演示
    └── README.md                    # 视频清单与待补清单
```

---

## 快速开始

### 打开固件工程

1. 安装 **STM32CubeIDE 2.x**（本队使用 2.1.1）。
2. 在 CubeIDE 中选择 `File → Import → Existing Projects into Workspace`。
3. 浏览到 `firmware/CRTC_CAR/` 目录并导入。
4. 连接 ST-LINK V2，编译并下载到 STM32F103C8T6 核心板。

### 硬件资源摘要

| 资源 | 分配 |
| ---- | ---- |
| 主控 | STM32F103C8T6，72 MHz |
| 电机 PWM | TIM1（PA8–PA11）+ TIM2（PA0–PA3），20 kHz |
| 舵机 PWM | TIM4 CH1–CH3（PB6 / PB7 / PB8），50 Hz |
| PS2 手柄 | PB12（CS）/ PB13（CLK）/ PB14（DAT）/ PB15（CMD）|
| 继电器 | PB5 |
| 状态 LED | PC13 |

### 手柄映射

| 输入 | 功能 |
| ---- | ---- |
| START | 安全解锁（需 R1 松开 + 四摇杆归中） |
| SELECT | 立即锁定 |
| 左摇杆 ↑/↓ | 前进 / 后退 |
| 左摇杆 ←/→ | 左右平移 |
| 右摇杆 ←/→ | 原地旋转 |
| R1 按住 | 底盘行驶使能 |
| R2 按住 | 精细档（三个运动分量减半） |
| 十字键 | 调节 SERVO-1 / SERVO-2 角度 |
| △ / □ | 调节 SERVO-3 角度 |
| ✕ | 切换无线继电器 |

---

## 分模块调试

板子只有一个 PC13 LED、没有串口，所以三个调试程序全部用 **LED 闪烁节奏** 编码状态。

### 使用方法

1. 打开 `Core/Inc/debug_config.h`
2. 把 `DEBUG_MODE` 改成下面四个值之一：

| `DEBUG_MODE` | 含义 | 用途 |
| --- | --- | --- |
| `DEBUG_MODE_NORMAL` (0) | 正常运行 | 手柄遥控整车（正式程序） |
| `DEBUG_MODE_SERVO` (1) | 只调舵机 | 逐个 / 同步扫掠，检查电源与行程 |
| `DEBUG_MODE_MOTOR` (2) | 只调电机 | 逐轮转向、整车前进/平移/旋转 |
| `DEBUG_MODE_PS2` (3) | 只调手柄 | 在线检测、按键、摇杆、解锁条件 |

3. 重新编译下载。**跑完记得改回 `DEBUG_MODE_NORMAL`**，否则正式程序不工作。

> ⚠️ **首次使用必看**：`debug_*.c` 是新增的源文件，而 CubeIDE 的 `Debug/Core/Src/subdir.mk`
> 是**硬编码的源文件列表**，不会自动发现新文件。若编译时报
> `undefined reference to Debug_Servo_Run`（或 Motor / PS2），按下面步骤处理：
> 1. 在 Project Explorer 里**右键项目 → Refresh**（或按 F5）
> 2. `Project → Clean...` → 勾上本项目 → Clean
> 3. 重新 Build
>
> 刷新后 CubeIDE 会重新生成 `subdir.mk`，把三个 `debug_*.c` 加进编译列表。

### 各程序的测试项（LED 快闪 N 下 = 进入第 N 项）

**舵机**（`debug_servo.c`）
1. 三路回中 90° → 2. 舵机1 扫掠 → 3. 舵机2 扫掠 → 4. 舵机3 扫掠 → 5. 三路同步扫掠 → 6. 三路保持 45°

**电机**（`debug_motor.c`）⚠️ 调试前务必四轮离地
1. 四轮逐个正转 → 2. 四轮逐个反转 → 3. 四轮同时正转 → 4. 前进/后退 → 5. 右移/左移 → 6. 顺/逆时针 → 7. 斜向运动

**PS2**（`debug_ps2.c`）
1. 在线检测 → 2. 按键测试 → 3. 左摇杆X → 4. 左摇杆Y → 5. 右摇杆X → 6. 右摇杆Y → 7. 解锁条件测试

每个文件头部注释里都写了**故障对照表**（不动/转向反/掉线等怎么查），现场排查时可直接对照。

---

## 当前完成状态

| 指标 | 状态 |
| ---- | ---- |
| 1 外形展示 | ✅ 已实现（`appearance_360.mp4`） |
| 2 电机运动（单一） | ✅ 已实现 |
| 3 电机运动（全部） | ✅ 已实现 |
| 4 舵机运动（单一） | ✅ 已实现（`servo_demo.mp4`） |
| 5 舵机运动（多项） | ✅ 已实现（`servo_demo.mp4`） |
| 6 底盘运动（单一） | ✅ 已实现，视频已录制（`chassis_ps2.mp4`） |
| 7 底盘运动（全部） | ❌ 未实现（仅 1 台完整底盘） |
| 8 尺寸规范 | ✅ 已实测 **180 × 165 × 155 mm**，三向卷尺照片已入档 |
| 9 方块携带（单一） | ❌ 未完成（3D 打印件未产出） |
| 10 方块携带（多项） | ❌ 未完成 |
| 11 结构稳定性测试 | ✅ 已完成（180 mm 跌落 2 次，视频 `drop_test.mp4`） |
| 12 规划 / 采购单 | 选型完成，单价待补 |

> 详见 `docs/CRTC2026_中期考核技术文档.md` 第 1.1 节。

---

## 提交材料说明

**仓库内容**

- 中期考核主文档：`docs/CRTC2026_中期考核技术文档.md`
- 文档配图：`docs/images/`（系统框图、电源原理图、软件流程图、PCB 三视图、尺寸实测照片）
- 机械模型：`mechanical/parts/` 下 9 个 STEP 文件
- 固件源码：`firmware/CRTC_CAR/`
- PCB 工程：`pcb/`（嘉立创 EDA 工程 + 原理图 PDF）
- 演示视频：`media/`（底盘遥控、跌落测试已达 1080p/30fps；舵机想法演示为 720p 参考素材）

**邮件提交（按手册「材料提交方式」）**

| 项目 | 内容 |
| ---- | ---- |
| 收件人 | zhang.guohao@lark.cquqian.li |
| 邮件主题 | **随缘 张川** |
| 正文 | 罗列本次中期考核完成的指标 + 本仓库地址 |
| 附件 | **`GSing随缘张川.zip`**（约 60 MB，含全部演示视频） |

> 附件 zip 按手册要求命名为「实训战队名称＋队伍名称＋队长姓名」，已生成在仓库同级目录，**不放入仓库**（避免视频重复占用空间）。

---

## 开源与许可

- `STM32 HAL 库` 与 `CMSIS` 为 STMicroelectronics / ARM 官方库，遵循其自带许可。
- `ps2.c`、`motor.c`、`servo.c`、`main.c` 等本队自研代码归本队所有，可用于 CRTC2026 中期考核提交。
- 机械模型为本队原创设计。
