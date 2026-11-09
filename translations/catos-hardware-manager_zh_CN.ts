<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
  <context>
    <name>ApplyConfirmation</name>
    <message>
      <source>Apply hardware configurations</source>
      <translation>应用硬件配置</translation>
    </message>
    <message>
      <source>Remove drivers</source>
      <translation>移除驱动</translation>
    </message>
    <message>
      <source>Install drivers</source>
      <translation>安装驱动</translation>
    </message>
    <message>
      <source>Install hardware support</source>
      <translation>安装硬件支持</translation>
    </message>
    <message>
      <source>Stop services</source>
      <translation>停用服务</translation>
    </message>
    <message>
      <source>Package and service changes</source>
      <translation>软件包和服务变更</translation>
    </message>
    <message>
      <source>Collapsed</source>
      <translation>已收起</translation>
    </message>
    <message>
      <source>Expanded</source>
      <translation>已展开</translation>
    </message>
    <message>
      <source>Restart the computer after the driver change.</source>
      <translation>驱动更改完成后需要重启计算机。</translation>
    </message>
    <message>
      <source>Without a replacement, the device falls back to the generic kernel driver.</source>
      <translation>没有选择替代驱动，设备将改用内核自带的通用驱动。</translation>
    </message>
    <message>
      <source>Stopped services stay disabled after a restart. Their packages stay installed.</source>
      <translation>停用的服务重启后仍保持停用，相关软件包不会被卸载。</translation>
    </message>
    <message>
      <source>Changes are applied in order. Completed steps may remain if a later step fails.</source>
      <translation>更改按顺序执行。如果后面的步骤失败，已完成的步骤会保留。</translation>
    </message>
    <message>
      <source>Checking packages and system state…</source>
      <translation>正在检查软件包和系统状态…</translation>
    </message>
    <message>
      <source>Confirm and apply</source>
      <translation>确认应用</translation>
    </message>
    <message>
      <source>The DKMS driver cannot be built for kernels without headers: %1. Install the matching headers package first.</source>
      <translation>以下内核缺少头文件，无法为其构建 DKMS 驱动：%1。请先安装对应的 headers 软件包。</translation>
    </message>
    <message>
      <source>Secure Boot is enabled. Modules built by DKMS must be signed before the kernel loads them.</source>
      <translation>已启用安全启动。DKMS 构建的模块需要签名后才能被内核加载。</translation>
    </message>
    <message>
      <source>System updates are available. Update the whole system first to avoid a partial upgrade during the driver change.</source>
      <translation>系统有可用更新。建议先完整更新系统，避免切换驱动时出现部分升级。</translation>
    </message>
    <message>
      <source>A snapper snapshot is created before the drivers change.</source>
      <translation>切换驱动前会创建 snapper 快照。</translation>
    </message>
    <message>
      <source>snapper is not configured; no snapshot will be created.</source>
      <translation>未配置 snapper，不会创建快照。</translation>
    </message>
    <message>
      <source>Download size: %1</source>
      <translation>下载大小：%1</translation>
    </message>
  </context>
  <context>
    <name>DeviceDiscovery</name>
    <message>
      <source>Thunderbolt / USB4 controller</source>
      <translation>雷电 / USB4 控制器</translation>
    </message>
    <message>
      <source>Hybrid graphics</source>
      <translation>混合显卡</translation>
    </message>
    <message>
      <source>AMD processor</source>
      <translation>AMD 处理器</translation>
    </message>
    <message>
      <source>Intel processor</source>
      <translation>Intel 处理器</translation>
    </message>
    <message>
      <source>KVM virtual machine</source>
      <translation>KVM 虚拟机</translation>
    </message>
    <message>
      <source>QEMU virtual machine</source>
      <translation>QEMU 虚拟机</translation>
    </message>
    <message>
      <source>VirtualBox virtual machine</source>
      <translation>VirtualBox 虚拟机</translation>
    </message>
    <message>
      <source>VMware virtual machine</source>
      <translation>VMware 虚拟机</translation>
    </message>
    <message>
      <source>Hyper-V virtual machine</source>
      <translation>Hyper-V 虚拟机</translation>
    </message>
    <message>
      <source>Virtual machine (%1)</source>
      <translation>虚拟机（%1）</translation>
    </message>
    <message>
      <source>Battery</source>
      <translation>电池</translation>
    </message>
  </context>
  <context>
    <name>DeviceRow</name>
    <message>
      <source>No additional profile</source>
      <translation>没有额外可用配置</translation>
    </message>
    <message>
      <source>The driver is loaded; the device itself has not been tested.</source>
      <translation>驱动已加载；设备本身尚未经过功能测试。</translation>
    </message>
    <message>
      <source>Open sound settings</source>
      <translation>打开声音设置</translation>
    </message>
    <message>
      <source>Open Bluetooth settings</source>
      <translation>打开蓝牙设置</translation>
    </message>
    <message>
      <source>Open printer settings and test printing</source>
      <translation>打开打印机设置与测试打印</translation>
    </message>
    <message>
      <source>Open camera test tool</source>
      <translation>打开摄像头测试工具</translation>
    </message>
    <message>
      <source>Open document scanner</source>
      <translation>打开文档扫描工具</translation>
    </message>
    <message>
      <source>Enroll fingerprints</source>
      <translation>录入指纹</translation>
    </message>
    <message>
      <source>Open power settings</source>
      <translation>打开电源设置</translation>
    </message>
    <message>
      <source>Hardware Manager</source>
      <translation>硬件管理器</translation>
    </message>
    <message>
      <source>Could not open the device tool.</source>
      <translation>无法打开设备工具。</translation>
    </message>
  </context>
  <context>
    <name>DevicesPage</name>
    <message>
      <source>Hardware devices</source>
      <translation>硬件设备</translation>
    </message>
    <message>
      <source>Refreshing…</source>
      <translation>正在刷新…</translation>
    </message>
    <message>
      <source>Select recommended</source>
      <translation>选择推荐配置</translation>
    </message>
    <message>
      <source>Refresh (F5)</source>
      <translation>刷新 (F5)</translation>
    </message>
    <message>
      <source>Refresh</source>
      <translation>刷新</translation>
    </message>
    <message numerus="yes">
      <source>%n device(s)</source>
      <translation>
        <numerusform>%n 个设备</numerusform>
      </translation>
    </message>
    <message>
      <source>No detected devices</source>
      <translation>未检测到支持的设备</translation>
    </message>
    <message>
      <source>Diagnostic report</source>
      <translation>诊断报告</translation>
    </message>
  </context>
  <context>
    <name>HardwareCategory</name>
    <message>
      <source>Graphics</source>
      <translation>图形设备</translation>
    </message>
    <message>
      <source>Network</source>
      <translation>网络设备</translation>
    </message>
    <message>
      <source>Audio</source>
      <translation>音频设备</translation>
    </message>
    <message>
      <source>Bluetooth</source>
      <translation>蓝牙设备</translation>
    </message>
    <message>
      <source>Cameras</source>
      <translation>摄像头</translation>
    </message>
    <message>
      <source>Printers</source>
      <translation>打印机</translation>
    </message>
    <message>
      <source>Scanners</source>
      <translation>扫描仪</translation>
    </message>
    <message>
      <source>Tablets</source>
      <translation>数位板</translation>
    </message>
    <message>
      <source>Thunderbolt</source>
      <translation>雷电设备</translation>
    </message>
    <message>
      <source>Controllers</source>
      <translation>游戏手柄</translation>
    </message>
    <message>
      <source>Mobile broadband</source>
      <translation>移动宽带</translation>
    </message>
    <message>
      <source>Smart cards</source>
      <translation>智能卡</translation>
    </message>
    <message>
      <source>Other devices</source>
      <translation>其他设备</translation>
    </message>
    <message>
      <source>Fingerprint readers</source>
      <translation>指纹识别器</translation>
    </message>
    <message>
      <source>Virtual machines</source>
      <translation>虚拟机</translation>
    </message>
    <message>
      <source>Power</source>
      <translation>电源</translation>
    </message>
    <message>
      <source>Processor</source>
      <translation>处理器</translation>
    </message>
  </context>
  <context>
    <name>HardwareManagerWindow</name>
    <message>
      <source>CatOS Hardware Manager</source>
      <translation>CatOS 硬件管理器</translation>
    </message>
    <message>
      <source>Hardware devices</source>
      <translation>硬件设备</translation>
    </message>
    <message>
      <source>Installed profiles</source>
      <translation>已安装配置</translation>
    </message>
    <message>
      <source>Profile catalog</source>
      <translation>配置库</translation>
    </message>
    <message>
      <source>Discard changes</source>
      <translation>撤销更改</translation>
    </message>
    <message>
      <source>Apply</source>
      <translation>应用</translation>
    </message>
    <message>
      <source>Hardware Manager</source>
      <translation>硬件管理器</translation>
    </message>
    <message>
      <source>%1 pending changes</source>
      <translation>%1 项待应用更改</translation>
    </message>
    <message>
      <source>Last operation</source>
      <translation>上次操作</translation>
    </message>
    <message>
      <source>Apply (Ctrl+Enter)</source>
      <translation>应用 (Ctrl+Enter)</translation>
    </message>
    <message>
      <source>Hardware information refreshed</source>
      <translation>硬件信息已刷新</translation>
    </message>
    <message>
      <source>Hardware query failed</source>
      <translation>硬件查询失败</translation>
    </message>
    <message>
      <source>Refreshing hardware information…</source>
      <translation>正在刷新硬件信息…</translation>
    </message>
    <message>
      <source>This profile cannot be changed right now.</source>
      <translation>此配置当前无法更改。</translation>
    </message>
    <message>
      <source>Wait for the current operation to finish before closing.</source>
      <translation>请等待当前操作结束后再关闭。</translation>
    </message>
    <message>
      <source>Close and discard pending changes?</source>
      <translation>是否关闭并放弃待应用的更改？</translation>
    </message>
    <message>
      <source>Diagnostic report preview</source>
      <translation>诊断报告预览</translation>
    </message>
    <message>
      <source>This report excludes serial numbers, device names, addresses, hostnames and raw logs. Review it before saving.</source>
      <translation>本报告不包含序列号、设备名称、地址、主机名和原始日志。保存前请检查内容。</translation>
    </message>
    <message>
      <source>Save diagnostic report</source>
      <translation>保存诊断报告</translation>
    </message>
    <message>
      <source>JSON files (*.json)</source>
      <translation>JSON 文件 (*.json)</translation>
    </message>
    <message>
      <source>Could not save diagnostic report.</source>
      <translation>无法保存诊断报告。</translation>
    </message>
  </context>
  <context>
    <name>HardwareService</name>
    <message>
      <source>Cannot query service %1: %2</source>
      <translation>无法查询服务 %1：%2</translation>
    </message>
    <message>
      <source>Driver query unavailable: %1</source>
      <translation>驱动查询不可用：%1</translation>
    </message>
    <message>
      <source>Invalid CHWD response</source>
      <translation>CHWD 返回数据无效</translation>
    </message>
    <message>
      <source>Cannot query package dependency satisfaction; using installed package names.</source>
      <translation>无法查询软件包依赖满足情况，改用已安装的软件包名判断。</translation>
    </message>
    <message>
      <source>%1 is installed, but neither the initramfs nor the boot loader entries load it. Add the microcode hook to mkinitcpio or reference /boot/%1.img in the boot entry.</source>
      <translation>已安装 %1，但 initramfs 和引导项都没有加载它。请在 mkinitcpio 的 HOOKS 中加入 microcode，或在引导项中引用 /boot/%1.img。</translation>
    </message>
  </context>
  <context>
    <name>InstalledProfilesPage</name>
    <message>
      <source>Installed profiles</source>
      <translation>已安装配置</translation>
    </message>
    <message>
      <source>CHWD configurations</source>
      <translation>CHWD 驱动配置</translation>
    </message>
    <message>
      <source>System support</source>
      <translation>系统硬件支持</translation>
    </message>
    <message>
      <source>No installed profiles</source>
      <translation>没有已安装配置</translation>
    </message>
  </context>
  <context>
    <name>NoticeBanner</name>
    <message>
      <source>Show all</source>
      <translation>全部显示</translation>
    </message>
    <message>
      <source>Hide</source>
      <translation>收起</translation>
    </message>
    <message>
      <source>Notice details shown</source>
      <translation>已展开提示详情</translation>
    </message>
    <message>
      <source>Notice details hidden</source>
      <translation>已收起提示详情</translation>
    </message>
    <message>
      <source>Hardware query failed: %1</source>
      <translation>硬件查询失败：%1</translation>
    </message>
    <message numerus="yes">
      <source>%1 (and %n more notice(s))</source>
      <translation>
        <numerusform>%1（另有 %n 条提示）</numerusform>
      </translation>
    </message>
  </context>
  <context>
    <name>Presentation</name>
    <message>
      <source>Needs attention: %1</source>
      <translation>需要处理：%1</translation>
    </message>
    <message>
      <source>Changes pending</source>
      <translation>有待应用的更改</translation>
    </message>
    <message>
      <source>no open-source driver supports this reader yet</source>
      <translation>开源驱动暂不支持此指纹设备</translation>
    </message>
    <message>
      <source>Driver not found</source>
      <translation>未找到驱动</translation>
    </message>
    <message>
      <source>recommended driver is not installed</source>
      <translation>未安装推荐驱动</translation>
    </message>
    <message>
      <source>recommended support components are missing</source>
      <translation>缺少推荐的支持组件</translation>
    </message>
    <message>
      <source>a support service is not running</source>
      <translation>支持服务未运行</translation>
    </message>
    <message>
      <source>Normal</source>
      <translation>正常</translation>
    </message>
    <message>
      <source>Driver: %1</source>
      <translation>驱动：%1</translation>
    </message>
    <message>
      <source>CHWD: %1</source>
      <translation>CHWD：%1</translation>
    </message>
    <message>
      <source>  · Recommended</source>
      <translation>  · 推荐</translation>
    </message>
    <message>
      <source>Will be removed</source>
      <translation>将移除</translation>
    </message>
    <message>
      <source>Will be installed</source>
      <translation>将安装</translation>
    </message>
    <message>
      <source>Installed</source>
      <translation>已安装</translation>
    </message>
    <message>
      <source>Not applicable</source>
      <translation>不适用</translation>
    </message>
    <message>
      <source>Not installed</source>
      <translation>未安装</translation>
    </message>
    <message>
      <source>Installed; device disconnected</source>
      <translation>已安装；设备未连接</translation>
    </message>
    <message>
      <source>Managed by %1</source>
      <translation>已由 %1 管理</translation>
    </message>
    <message>
      <source>Services will be stopped</source>
      <translation>服务将停用</translation>
    </message>
    <message>
      <source>Ready</source>
      <translation>已就绪</translation>
    </message>
    <message>
      <source>Service not running</source>
      <translation>服务未运行</translation>
    </message>
    <message>
      <source>Missing components</source>
      <translation>缺少组件</translation>
    </message>
  </context>
  <context>
    <name>ProfileCatalogPage</name>
    <message>
      <source>Profile catalog</source>
      <translation>配置库</translation>
    </message>
    <message>
      <source>Search profiles</source>
      <translation>搜索配置</translation>
    </message>
    <message>
      <source>No matching profiles</source>
      <translation>没有匹配的配置</translation>
    </message>
  </context>
  <context>
    <name>ProfileRow</name>
    <message>
      <source>Missing packages</source>
      <translation>缺少的软件包</translation>
    </message>
    <message>
      <source>Inactive services</source>
      <translation>未启用的服务</translation>
    </message>
    <message>
      <source>Profile details</source>
      <translation>配置详情</translation>
    </message>
    <message>
      <source>Collapsed</source>
      <translation>已收起</translation>
    </message>
    <message>
      <source>Expanded</source>
      <translation>已展开</translation>
    </message>
    <message>
      <source>Keep services running</source>
      <translation>保持服务运行</translation>
    </message>
    <message>
      <source>Stop services</source>
      <translation>停用服务</translation>
    </message>
    <message>
      <source>Stop and disable %1. Packages stay installed.</source>
      <translation>停止并禁用 %1，软件包保持安装。</translation>
    </message>
    <message>
      <source>Profile details: %1</source>
      <translation>%1 的配置详情</translation>
    </message>
    <message>
      <source>Packages</source>
      <translation>软件包</translation>
    </message>
    <message>
      <source>Services</source>
      <translation>系统服务</translation>
    </message>
    <message>
      <source>Services not enabled</source>
      <translation>未启用的服务</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <source>Hardware Manager</source>
      <translation>硬件管理器</translation>
    </message>
    <message>
      <source>The privileged executor is not installed.</source>
      <translation>尚未安装管理器的授权执行组件。</translation>
    </message>
    <message>
      <source>Applying hardware configurations</source>
      <translation>正在应用硬件配置</translation>
    </message>
    <message>
      <source>Some changes could not be applied.</source>
      <translation>部分硬件配置未能完成。</translation>
    </message>
    <message>
      <source>Operation</source>
      <translation>操作</translation>
    </message>
    <message>
      <source>Waiting for administrator authorization…</source>
      <translation>正在等待管理员授权…</translation>
    </message>
    <message>
      <source>Result</source>
      <translation>结果</translation>
    </message>
    <message>
      <source>Copy log</source>
      <translation>复制日志</translation>
    </message>
    <message>
      <source>Command output</source>
      <translation>命令输出</translation>
    </message>
    <message>
      <source>Waiting</source>
      <translation>等待中</translation>
    </message>
    <message>
      <source>Step %1 of %2: %3</source>
      <translation>第 %1 步，共 %2 步：%3</translation>
    </message>
    <message>
      <source>Running</source>
      <translation>正在执行</translation>
    </message>
    <message>
      <source>Completed</source>
      <translation>已完成</translation>
    </message>
    <message>
      <source>Failed</source>
      <translation>失败</translation>
    </message>
    <message>
      <source>Restore driver: %1</source>
      <translation>恢复驱动：%1</translation>
    </message>
    <message>
      <source>Needs attention</source>
      <translation>需要处理</translation>
    </message>
    <message>
      <source>Interrupted</source>
      <translation>已中断</translation>
    </message>
    <message>
      <source>Authorization was cancelled. Nothing was changed.</source>
      <translation>已取消授权，未做任何更改。</translation>
    </message>
    <message>
      <source>Hardware configurations applied.</source>
      <translation>硬件配置已应用。</translation>
    </message>
    <message>
      <source>Hardware configurations applied. Restart the computer to use the new drivers.</source>
      <translation>硬件配置已应用。请重启计算机以使用新驱动。</translation>
    </message>
    <message>
      <source>Completed steps were kept. If a snapshot was created, you can restore it with snapper.</source>
      <translation>已完成的步骤已保留。如果创建了快照，可以用 snapper 恢复。</translation>
    </message>
    <message>
      <source>Previous change did not finish</source>
      <translation>上次更改未完成</translation>
    </message>
    <message>
      <source>The last hardware change stopped before it finished, for example because the computer was turned off. Drivers may be half installed. Review the log, then refresh and check the affected devices.</source>
      <translation>上一次硬件更改在完成前中断了（例如计算机被关机）。驱动可能只装了一半。请先查看日志，然后刷新并检查相关设备。</translation>
    </message>
    <message>
      <source>View log</source>
      <translation>查看日志</translation>
    </message>
    <message>
      <source>Interrupted change</source>
      <translation>中断的更改</translation>
    </message>
    <message>
      <source>Could not save the operation log. Copy it before closing.</source>
      <translation>无法保存操作日志。请在关闭前复制日志。</translation>
    </message>
    <message>
      <source>Last operation</source>
      <translation>上次操作</translation>
    </message>
    <message>
      <source>No saved operation log.</source>
      <translation>没有已保存的操作日志。</translation>
    </message>
  </context>
  <context>
    <name>TransactionProtocol</name>
    <message>
      <source>Download packages</source>
      <translation>下载软件包</translation>
    </message>
    <message>
      <source>Create snapshot</source>
      <translation>创建快照</translation>
    </message>
    <message>
      <source>Remove driver</source>
      <translation>移除驱动</translation>
    </message>
    <message>
      <source>Install driver</source>
      <translation>安装驱动</translation>
    </message>
    <message>
      <source>Install support components</source>
      <translation>安装支持组件</translation>
    </message>
    <message>
      <source>Start service</source>
      <translation>启动服务</translation>
    </message>
    <message>
      <source>Stop service</source>
      <translation>停用服务</translation>
    </message>
    <message>
      <source>Another package manager is running. Close it and try again.</source>
      <translation>另一个包管理器正在运行。请关闭它后重试。</translation>
    </message>
    <message>
      <source>The request was rejected. Refresh and review the changes again.</source>
      <translation>请求被拒绝。请刷新并重新检查更改。</translation>
    </message>
    <message>
      <source>The hardware state changed since it was shown. Refresh and review the changes again.</source>
      <translation>硬件状态在显示后发生了变化。请刷新并重新检查更改。</translation>
    </message>
    <message>
      <source>The changes ran, but the final state could not be read. Refresh to check the result.</source>
      <translation>更改已执行，但无法读取最终状态。请刷新查看结果。</translation>
    </message>
    <message>
      <source>The hardware state could not be read. Refresh and try again.</source>
      <translation>无法读取硬件状态。请刷新后重试。</translation>
    </message>
    <message>
      <source>%1 is still installed after removal.</source>
      <translation>移除后 %1 仍然处于已安装状态。</translation>
    </message>
    <message>
      <source>%1 was not installed.</source>
      <translation>%1 没有安装成功。</translation>
    </message>
    <message>
      <source>The services of %1 are still running.</source>
      <translation>%1 的服务仍在运行。</translation>
    </message>
    <message>
      <source>%1 is not ready after installation.</source>
      <translation>%1 安装后仍未就绪。</translation>
    </message>
    <message>
      <source>"%1" failed for %2.</source>
      <translation>%2 的“%1”步骤失败。</translation>
    </message>
  </context>
</TS>
