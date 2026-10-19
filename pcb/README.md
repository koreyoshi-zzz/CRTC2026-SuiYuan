# CRTC_copy PCB 工程

本目录存放 CRTC2026 随缘战队的自制 PCB 工程文件。

## 文件

- `CRTC_copy.eprj2`：EasyEDA Pro（立创EDA专业版）工程文件，2026-10-19 20:41 的当前版本。
- `CRTC_copy_schematic.pdf`：当前工程原理图的 A4 横向 PDF。

## 打开方式

1. 先关闭 EasyEDA Pro 中已经打开的 `CRTC_copy` 原工程，或直接退出 EasyEDA Pro。
2. 打开 EasyEDA Pro → 文件 → 打开工程，选择本目录的 `CRTC_copy.eprj2`。
3. 不要直接双击 `.eprj2`（若系统未关联），也不要在原工程仍打开时重复打开副本；同一工程 UUID 同时只允许一个实例。

## 打不开时的排查

- **提示工程已打开/锁定**：关闭原工程或退出应用后重试。
- **从当前路径打开失败**：把 `.eprj2` 复制到纯英文路径（如 `D:\CRTC PCB\`）后再打开；部分 EasyEDA Pro 版本对中文路径兼容性较差。
- **仍无法打开**：在 EasyEDA Pro 中打开当前工程，使用“导出工程/另存为项目包”生成 `.epro2`，把 `.epro2` 放到本目录，并把文档链接改为 `.epro2`；`.epro2` 是更可靠的跨机器/归档格式。
- 文件完整性：本 `.eprj2` 已通过 SQLite 完整性检查（36 张表、66 条历史记录），不是下载损坏。

## 说明

`.eprj2` 是 EasyEDA Pro 的本地工程文件，不是通用压缩包；跨机器分发建议优先使用 `.epro2` 导出包。
