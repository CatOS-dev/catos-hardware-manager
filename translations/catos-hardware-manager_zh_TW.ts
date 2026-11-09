<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_TW">
  <context>
    <name>ApplyConfirmation</name>
    <message>
      <source>Apply hardware configurations</source>
      <translation>應用硬體配置</translation>
    </message>
    <message>
      <source>Remove drivers</source>
      <translation>移除驅動</translation>
    </message>
    <message>
      <source>Install drivers</source>
      <translation>安裝驅動</translation>
    </message>
    <message>
      <source>Install hardware support</source>
      <translation>安裝硬體支援</translation>
    </message>
    <message>
      <source>Stop services</source>
      <translation>停用服務</translation>
    </message>
    <message>
      <source>Package and service changes</source>
      <translation>軟體包和服務變更</translation>
    </message>
    <message>
      <source>Collapsed</source>
      <translation>已收起</translation>
    </message>
    <message>
      <source>Expanded</source>
      <translation>已展開</translation>
    </message>
    <message>
      <source>Restart the computer after the driver change.</source>
      <translation>驅動更改完成後需要重啟電腦。</translation>
    </message>
    <message>
      <source>Without a replacement, the device falls back to the generic kernel driver.</source>
      <translation>沒有選擇替代驅動，裝置將改用核心自帶的通用驅動。</translation>
    </message>
    <message>
      <source>Stopped services stay disabled after a restart. Their packages stay installed.</source>
      <translation>停用的服務重啟後仍保持停用，相關軟體包不會被解除安裝。</translation>
    </message>
    <message>
      <source>Changes are applied in order. Completed steps may remain if a later step fails.</source>
      <translation>更改按順序執行。如果後面的步驟失敗，已完成的步驟會保留。</translation>
    </message>
    <message>
      <source>Checking packages and system state…</source>
      <translation>正在檢查軟體包和系統狀態…</translation>
    </message>
    <message>
      <source>Confirm and apply</source>
      <translation>確認應用</translation>
    </message>
    <message>
      <source>The DKMS driver cannot be built for kernels without headers: %1. Install the matching headers package first.</source>
      <translation>以下核心缺少標頭檔案，無法為其構建 DKMS 驅動：%1。請先安裝對應的 headers 軟體包。</translation>
    </message>
    <message>
      <source>Secure Boot is enabled. Modules built by DKMS must be signed before the kernel loads them.</source>
      <translation>已啟用安全啟動。DKMS 構建的模組需要簽名後才能被核心載入。</translation>
    </message>
    <message>
      <source>System updates are available. Update the whole system first to avoid a partial upgrade during the driver change.</source>
      <translation>系統有可用更新。建議先完整更新系統，避免切換驅動時出現部分升級。</translation>
    </message>
    <message>
      <source>A snapper snapshot is created before the drivers change.</source>
      <translation>切換驅動前會建立 snapper 快照。</translation>
    </message>
    <message>
      <source>snapper is not configured; no snapshot will be created.</source>
      <translation>未配置 snapper，不會建立快照。</translation>
    </message>
    <message>
      <source>Download size: %1</source>
      <translation>下載大小：%1</translation>
    </message>
  </context>
  <context>
    <name>DeviceDiscovery</name>
    <message>
      <source>Thunderbolt / USB4 controller</source>
      <translation>雷電 / USB4 控制器</translation>
    </message>
    <message>
      <source>Hybrid graphics</source>
      <translation>混合顯示卡</translation>
    </message>
    <message>
      <source>AMD processor</source>
      <translation>AMD 處理器</translation>
    </message>
    <message>
      <source>Intel processor</source>
      <translation>Intel 處理器</translation>
    </message>
    <message>
      <source>KVM virtual machine</source>
      <translation>KVM 虛擬機器</translation>
    </message>
    <message>
      <source>QEMU virtual machine</source>
      <translation>QEMU 虛擬機器</translation>
    </message>
    <message>
      <source>VirtualBox virtual machine</source>
      <translation>VirtualBox 虛擬機器</translation>
    </message>
    <message>
      <source>VMware virtual machine</source>
      <translation>VMware 虛擬機器</translation>
    </message>
    <message>
      <source>Hyper-V virtual machine</source>
      <translation>Hyper-V 虛擬機器</translation>
    </message>
    <message>
      <source>Virtual machine (%1)</source>
      <translation>虛擬機器（%1）</translation>
    </message>
    <message>
      <source>Battery</source>
      <translation>電池</translation>
    </message>
  </context>
  <context>
    <name>DeviceRow</name>
    <message>
      <source>No additional profile</source>
      <translation>沒有額外可用配置</translation>
    </message>
    <message>
      <source>The driver is loaded; the device itself has not been tested.</source>
      <translation>驅動已載入；裝置本身尚未經過功能測試。</translation>
    </message>
    <message>
      <source>Open sound settings</source>
      <translation>開啟聲音設定</translation>
    </message>
    <message>
      <source>Open Bluetooth settings</source>
      <translation>開啟藍牙設定</translation>
    </message>
    <message>
      <source>Open printer settings and test printing</source>
      <translation>開啟印表機設定與測試列印</translation>
    </message>
    <message>
      <source>Open camera test tool</source>
      <translation>開啟攝像頭測試工具</translation>
    </message>
    <message>
      <source>Open document scanner</source>
      <translation>開啟文件掃描工具</translation>
    </message>
    <message>
      <source>Enroll fingerprints</source>
      <translation>錄入指紋</translation>
    </message>
    <message>
      <source>Open power settings</source>
      <translation>開啟電源設定</translation>
    </message>
    <message>
      <source>Hardware Manager</source>
      <translation>硬體管理器</translation>
    </message>
    <message>
      <source>Could not open the device tool.</source>
      <translation>無法開啟裝置工具。</translation>
    </message>
  </context>
  <context>
    <name>DevicesPage</name>
    <message>
      <source>Hardware devices</source>
      <translation>硬體裝置</translation>
    </message>
    <message>
      <source>Refreshing…</source>
      <translation>正在重新整理…</translation>
    </message>
    <message>
      <source>Select recommended</source>
      <translation>選擇推薦配置</translation>
    </message>
    <message>
      <source>Refresh (F5)</source>
      <translation>重新整理 (F5)</translation>
    </message>
    <message>
      <source>Refresh</source>
      <translation>重新整理</translation>
    </message>
    <message numerus="yes">
      <source>%n device(s)</source>
      <translation>
        <numerusform>%n 個裝置</numerusform>
      </translation>
    </message>
    <message>
      <source>No detected devices</source>
      <translation>未檢測到支援的裝置</translation>
    </message>
    <message>
      <source>Diagnostic report</source>
      <translation>診斷報告</translation>
    </message>
  </context>
  <context>
    <name>HardwareCategory</name>
    <message>
      <source>Graphics</source>
      <translation>圖形裝置</translation>
    </message>
    <message>
      <source>Network</source>
      <translation>網路裝置</translation>
    </message>
    <message>
      <source>Audio</source>
      <translation>音訊裝置</translation>
    </message>
    <message>
      <source>Bluetooth</source>
      <translation>藍牙裝置</translation>
    </message>
    <message>
      <source>Cameras</source>
      <translation>攝像頭</translation>
    </message>
    <message>
      <source>Printers</source>
      <translation>印表機</translation>
    </message>
    <message>
      <source>Scanners</source>
      <translation>掃描器</translation>
    </message>
    <message>
      <source>Tablets</source>
      <translation>數位板</translation>
    </message>
    <message>
      <source>Thunderbolt</source>
      <translation>雷電裝置</translation>
    </message>
    <message>
      <source>Controllers</source>
      <translation>遊戲手柄</translation>
    </message>
    <message>
      <source>Mobile broadband</source>
      <translation>移動寬頻</translation>
    </message>
    <message>
      <source>Smart cards</source>
      <translation>智慧卡</translation>
    </message>
    <message>
      <source>Other devices</source>
      <translation>其他裝置</translation>
    </message>
    <message>
      <source>Fingerprint readers</source>
      <translation>指紋識別器</translation>
    </message>
    <message>
      <source>Virtual machines</source>
      <translation>虛擬機器</translation>
    </message>
    <message>
      <source>Power</source>
      <translation>電源</translation>
    </message>
    <message>
      <source>Processor</source>
      <translation>處理器</translation>
    </message>
  </context>
  <context>
    <name>HardwareManagerWindow</name>
    <message>
      <source>CatOS Hardware Manager</source>
      <translation>CatOS 硬體管理器</translation>
    </message>
    <message>
      <source>Hardware devices</source>
      <translation>硬體裝置</translation>
    </message>
    <message>
      <source>Installed profiles</source>
      <translation>已安裝配置</translation>
    </message>
    <message>
      <source>Profile catalog</source>
      <translation>配置庫</translation>
    </message>
    <message>
      <source>Discard changes</source>
      <translation>撤銷更改</translation>
    </message>
    <message>
      <source>Apply</source>
      <translation>應用</translation>
    </message>
    <message>
      <source>Hardware Manager</source>
      <translation>硬體管理器</translation>
    </message>
    <message>
      <source>%1 pending changes</source>
      <translation>%1 項待應用更改</translation>
    </message>
    <message>
      <source>Last operation</source>
      <translation>上次操作</translation>
    </message>
    <message>
      <source>Apply (Ctrl+Enter)</source>
      <translation>應用 (Ctrl+Enter)</translation>
    </message>
    <message>
      <source>Hardware information refreshed</source>
      <translation>硬體資訊已重新整理</translation>
    </message>
    <message>
      <source>Hardware query failed</source>
      <translation>硬體查詢失敗</translation>
    </message>
    <message>
      <source>Refreshing hardware information…</source>
      <translation>正在重新整理硬體資訊…</translation>
    </message>
    <message>
      <source>This profile cannot be changed right now.</source>
      <translation>此配置當前無法更改。</translation>
    </message>
    <message>
      <source>Wait for the current operation to finish before closing.</source>
      <translation>請等待當前操作結束後再關閉。</translation>
    </message>
    <message>
      <source>Close and discard pending changes?</source>
      <translation>是否關閉並放棄待應用的更改？</translation>
    </message>
    <message>
      <source>Diagnostic report preview</source>
      <translation>診斷報告預覽</translation>
    </message>
    <message>
      <source>This report excludes serial numbers, device names, addresses, hostnames and raw logs. Review it before saving.</source>
      <translation>本報告不包含序列號、裝置名稱、地址、主機名和原始日誌。儲存前請檢查內容。</translation>
    </message>
    <message>
      <source>Save diagnostic report</source>
      <translation>儲存診斷報告</translation>
    </message>
    <message>
      <source>JSON files (*.json)</source>
      <translation>JSON 檔案 (*.json)</translation>
    </message>
    <message>
      <source>Could not save diagnostic report.</source>
      <translation>無法儲存診斷報告。</translation>
    </message>
  </context>
  <context>
    <name>HardwareService</name>
    <message>
      <source>Cannot query service %1: %2</source>
      <translation>無法查詢服務 %1：%2</translation>
    </message>
    <message>
      <source>Driver query unavailable: %1</source>
      <translation>驅動查詢不可用：%1</translation>
    </message>
    <message>
      <source>Invalid CHWD response</source>
      <translation>CHWD 返回資料無效</translation>
    </message>
    <message>
      <source>Cannot query package dependency satisfaction; using installed package names.</source>
      <translation>無法查詢軟體包依賴滿足情況，改用已安裝的軟體包名判斷。</translation>
    </message>
    <message>
      <source>%1 is installed, but neither the initramfs nor the boot loader entries load it. Add the microcode hook to mkinitcpio or reference /boot/%1.img in the boot entry.</source>
      <translation>已安裝 %1，但 initramfs 和引導項都沒有載入它。請在 mkinitcpio 的 HOOKS 中加入 microcode，或在引導項中引用 /boot/%1.img。</translation>
    </message>
  </context>
  <context>
    <name>InstalledProfilesPage</name>
    <message>
      <source>Installed profiles</source>
      <translation>已安裝配置</translation>
    </message>
    <message>
      <source>CHWD configurations</source>
      <translation>CHWD 驅動配置</translation>
    </message>
    <message>
      <source>System support</source>
      <translation>系統硬體支援</translation>
    </message>
    <message>
      <source>No installed profiles</source>
      <translation>沒有已安裝配置</translation>
    </message>
  </context>
  <context>
    <name>NoticeBanner</name>
    <message>
      <source>Show all</source>
      <translation>全部顯示</translation>
    </message>
    <message>
      <source>Hide</source>
      <translation>收起</translation>
    </message>
    <message>
      <source>Notice details shown</source>
      <translation>已展開提示詳情</translation>
    </message>
    <message>
      <source>Notice details hidden</source>
      <translation>已收起提示詳情</translation>
    </message>
    <message>
      <source>Hardware query failed: %1</source>
      <translation>硬體查詢失敗：%1</translation>
    </message>
    <message numerus="yes">
      <source>%1 (and %n more notice(s))</source>
      <translation>
        <numerusform>%1（另有 %n 條提示）</numerusform>
      </translation>
    </message>
  </context>
  <context>
    <name>Presentation</name>
    <message>
      <source>Needs attention: %1</source>
      <translation>需要處理：%1</translation>
    </message>
    <message>
      <source>Changes pending</source>
      <translation>有待應用的更改</translation>
    </message>
    <message>
      <source>no open-source driver supports this reader yet</source>
      <translation>開源驅動暫不支援此指紋裝置</translation>
    </message>
    <message>
      <source>Driver not found</source>
      <translation>未找到驅動</translation>
    </message>
    <message>
      <source>recommended driver is not installed</source>
      <translation>未安裝推薦驅動</translation>
    </message>
    <message>
      <source>recommended support components are missing</source>
      <translation>缺少推薦的支援元件</translation>
    </message>
    <message>
      <source>a support service is not running</source>
      <translation>支援服務未執行</translation>
    </message>
    <message>
      <source>Normal</source>
      <translation>正常</translation>
    </message>
    <message>
      <source>Driver: %1</source>
      <translation>驅動：%1</translation>
    </message>
    <message>
      <source>CHWD: %1</source>
      <translation>CHWD：%1</translation>
    </message>
    <message>
      <source>  · Recommended</source>
      <translation>  · 推薦</translation>
    </message>
    <message>
      <source>Will be removed</source>
      <translation>將移除</translation>
    </message>
    <message>
      <source>Will be installed</source>
      <translation>將安裝</translation>
    </message>
    <message>
      <source>Installed</source>
      <translation>已安裝</translation>
    </message>
    <message>
      <source>Not applicable</source>
      <translation>不適用</translation>
    </message>
    <message>
      <source>Not installed</source>
      <translation>未安裝</translation>
    </message>
    <message>
      <source>Installed; device disconnected</source>
      <translation>已安裝；裝置未連線</translation>
    </message>
    <message>
      <source>Managed by %1</source>
      <translation>已由 %1 管理</translation>
    </message>
    <message>
      <source>Services will be stopped</source>
      <translation>服務將停用</translation>
    </message>
    <message>
      <source>Ready</source>
      <translation>已就緒</translation>
    </message>
    <message>
      <source>Service not running</source>
      <translation>服務未執行</translation>
    </message>
    <message>
      <source>Missing components</source>
      <translation>缺少元件</translation>
    </message>
  </context>
  <context>
    <name>ProfileCatalogPage</name>
    <message>
      <source>Profile catalog</source>
      <translation>配置庫</translation>
    </message>
    <message>
      <source>Search profiles</source>
      <translation>搜尋配置</translation>
    </message>
    <message>
      <source>No matching profiles</source>
      <translation>沒有匹配的配置</translation>
    </message>
  </context>
  <context>
    <name>ProfileRow</name>
    <message>
      <source>Missing packages</source>
      <translation>缺少的軟體包</translation>
    </message>
    <message>
      <source>Inactive services</source>
      <translation>未啟用的服務</translation>
    </message>
    <message>
      <source>Profile details</source>
      <translation>配置詳情</translation>
    </message>
    <message>
      <source>Collapsed</source>
      <translation>已收起</translation>
    </message>
    <message>
      <source>Expanded</source>
      <translation>已展開</translation>
    </message>
    <message>
      <source>Keep services running</source>
      <translation>保持服務執行</translation>
    </message>
    <message>
      <source>Stop services</source>
      <translation>停用服務</translation>
    </message>
    <message>
      <source>Stop and disable %1. Packages stay installed.</source>
      <translation>停止並停用 %1，軟體包保持安裝。</translation>
    </message>
    <message>
      <source>Profile details: %1</source>
      <translation>%1 的配置詳情</translation>
    </message>
    <message>
      <source>Packages</source>
      <translation>軟體包</translation>
    </message>
    <message>
      <source>Services</source>
      <translation>系統服務</translation>
    </message>
    <message>
      <source>Services not enabled</source>
      <translation>未啟用的服務</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <source>Hardware Manager</source>
      <translation>硬體管理器</translation>
    </message>
    <message>
      <source>The privileged executor is not installed.</source>
      <translation>尚未安裝管理器的授權執行元件。</translation>
    </message>
    <message>
      <source>Applying hardware configurations</source>
      <translation>正在應用硬體配置</translation>
    </message>
    <message>
      <source>Some changes could not be applied.</source>
      <translation>部分硬體配置未能完成。</translation>
    </message>
    <message>
      <source>Operation</source>
      <translation>操作</translation>
    </message>
    <message>
      <source>Waiting for administrator authorization…</source>
      <translation>正在等待管理員授權…</translation>
    </message>
    <message>
      <source>Result</source>
      <translation>結果</translation>
    </message>
    <message>
      <source>Copy log</source>
      <translation>複製日誌</translation>
    </message>
    <message>
      <source>Command output</source>
      <translation>命令輸出</translation>
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
      <translation>正在執行</translation>
    </message>
    <message>
      <source>Completed</source>
      <translation>已完成</translation>
    </message>
    <message>
      <source>Failed</source>
      <translation>失敗</translation>
    </message>
    <message>
      <source>Restore driver: %1</source>
      <translation>恢復驅動：%1</translation>
    </message>
    <message>
      <source>Needs attention</source>
      <translation>需要處理</translation>
    </message>
    <message>
      <source>Interrupted</source>
      <translation>已中斷</translation>
    </message>
    <message>
      <source>Authorization was cancelled. Nothing was changed.</source>
      <translation>已取消授權，未做任何更改。</translation>
    </message>
    <message>
      <source>Hardware configurations applied.</source>
      <translation>硬體配置已應用。</translation>
    </message>
    <message>
      <source>Hardware configurations applied. Restart the computer to use the new drivers.</source>
      <translation>硬體配置已應用。請重啟電腦以使用新驅動。</translation>
    </message>
    <message>
      <source>Completed steps were kept. If a snapshot was created, you can restore it with snapper.</source>
      <translation>已完成的步驟已保留。如果建立了快照，可以用 snapper 恢復。</translation>
    </message>
    <message>
      <source>Previous change did not finish</source>
      <translation>上次更改未完成</translation>
    </message>
    <message>
      <source>The last hardware change stopped before it finished, for example because the computer was turned off. Drivers may be half installed. Review the log, then refresh and check the affected devices.</source>
      <translation>上一次硬體更改在完成前中斷了（例如電腦被關機）。驅動可能只裝了一半。請先檢視日誌，然後重新整理並檢查相關裝置。</translation>
    </message>
    <message>
      <source>View log</source>
      <translation>檢視日誌</translation>
    </message>
    <message>
      <source>Interrupted change</source>
      <translation>中斷的更改</translation>
    </message>
    <message>
      <source>Could not save the operation log. Copy it before closing.</source>
      <translation>無法儲存操作日誌。請在關閉前複製日誌。</translation>
    </message>
    <message>
      <source>Last operation</source>
      <translation>上次操作</translation>
    </message>
    <message>
      <source>No saved operation log.</source>
      <translation>沒有已儲存的操作日誌。</translation>
    </message>
  </context>
  <context>
    <name>TransactionProtocol</name>
    <message>
      <source>Download packages</source>
      <translation>下載軟體包</translation>
    </message>
    <message>
      <source>Create snapshot</source>
      <translation>建立快照</translation>
    </message>
    <message>
      <source>Remove driver</source>
      <translation>移除驅動</translation>
    </message>
    <message>
      <source>Install driver</source>
      <translation>安裝驅動</translation>
    </message>
    <message>
      <source>Install support components</source>
      <translation>安裝支援元件</translation>
    </message>
    <message>
      <source>Start service</source>
      <translation>啟動服務</translation>
    </message>
    <message>
      <source>Stop service</source>
      <translation>停用服務</translation>
    </message>
    <message>
      <source>Another package manager is running. Close it and try again.</source>
      <translation>另一個包管理器正在執行。請關閉它後重試。</translation>
    </message>
    <message>
      <source>The request was rejected. Refresh and review the changes again.</source>
      <translation>請求被拒絕。請重新整理並重新檢查更改。</translation>
    </message>
    <message>
      <source>The hardware state changed since it was shown. Refresh and review the changes again.</source>
      <translation>硬體狀態在顯示後發生了變化。請重新整理並重新檢查更改。</translation>
    </message>
    <message>
      <source>The changes ran, but the final state could not be read. Refresh to check the result.</source>
      <translation>更改已執行，但無法讀取最終狀態。請重新整理檢視結果。</translation>
    </message>
    <message>
      <source>The hardware state could not be read. Refresh and try again.</source>
      <translation>無法讀取硬體狀態。請重新整理後重試。</translation>
    </message>
    <message>
      <source>%1 is still installed after removal.</source>
      <translation>移除後 %1 仍然處於已安裝狀態。</translation>
    </message>
    <message>
      <source>%1 was not installed.</source>
      <translation>%1 沒有安裝成功。</translation>
    </message>
    <message>
      <source>The services of %1 are still running.</source>
      <translation>%1 的服務仍在執行。</translation>
    </message>
    <message>
      <source>%1 is not ready after installation.</source>
      <translation>%1 安裝後仍未就緒。</translation>
    </message>
    <message>
      <source>"%1" failed for %2.</source>
      <translation>%2 的“%1”步驟失敗。</translation>
    </message>
  </context>
</TS>
