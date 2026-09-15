# XIAO Wi-Fi Touchpad

[![English](https://img.shields.io/badge/English-README-blue?style=for-the-badge)](README.md)

将 Seeed Studio XIAO 1.47 寸触摸屏（ESP32-S3 Plus）变成通过 Wi-Fi 控制 Windows 电脑的触摸板鼠标。

这是从 Pocket AI Terminal 提取出的可用鼠标 Demo。Chat、Voice 和 Control 页面仍是占位页面，尚未接入 AI 和语音功能。

## 硬件与软件

- XIAO ESP32-S3 Plus 和 172 × 320 JD9853A / AXS5106L 触摸屏板
- A-02 2.4 GHz 天线，连接到 XIAO 天线接口
- Arduino ESP32 Board Package 3.3.11
- Seeed_GFX2 v1.0.0，使用 Config_Seeed_1inch47_Touch_JD9853A
- Windows Python 3

## 配置与运行

1. 将 Pocket_AI_Terminal/wifi_config.h.example 复制为 Pocket_AI_Terminal/wifi_config.h。
2. 填写 WIFI_SSID、WIFI_PASSWORD，并将 GATEWAY_HOST 设置为电脑的局域网 IPv4 地址。Windows 中运行 ipconfig 查看地址。
3. 在 Arduino IDE 打开 Pocket_AI_Terminal/Pocket_AI_Terminal.ino，选择 XIAO_ESP32S3_PLUS 和对应端口，然后烧录。
4. 安装电脑端依赖：

    python -m pip install -r requirements.txt

5. 启动 Wi-Fi 网关：

    python gateway/pocket_gateway.py --tcp 192.168.1.100 8765

将示例地址替换为电脑实际局域网 IP。Windows 防火墙询问时允许 Python 通过专用网络。看到 READY 和持续 ping 消息表示连接正常。

本地 wifi_config.h 已被 Git 忽略。不要上传真实密码或包含真实密码的固件文件。

## 鼠标操作

| 功能 | 操作 |
| --- | --- |
| 移动指针 | 在中央触摸区滑动 |
| 左键单击 | 单击触摸区 |
| 双击 | 快速点击两次 |
| 拖动 / 选择文字 | 先单击，再在附近按住并滑动；松手释放 |
| 滚轮 | 在右侧窄条上下滑动 |
| 全选 / 复制 / 粘贴 / 撤销 | 点击左上角工具按钮 |
| 返回主页 | 按下并松开 HOME |

当前没有右键触摸绑定，静止长按不会开始拖动。

## USB 串口模式

如果 Wi-Fi 配置仍是模板值，可以使用：

    python gateway/pocket_gateway.py COM33

使用前关闭 Arduino 串口监视器。串口波特率是 115200。Wi-Fi 模式使用原始 TCP 和逐行 JSON，不使用 WebSocket。

## 验证状态

- 已使用 ESP32 3.3.11 和 Seeed_GFX2 v1.0.0 编译并烧录成功。
- 已确认屏幕通过 Wi-Fi 发送 hello、接收 READY，并持续发送心跳。
- Wi-Fi 鼠标移动、点击、双击、拖动、滚轮和快捷键已完成实机验证。
- 网关没有网络认证或加密，只在可信的私人局域网中使用，不要把 8765 端口暴露到互联网。

Seeed_GFX2 库及其许可证属于上游仓库，本项目不重复打包。
