# Plugin target 參考

[English](../en/PLUGIN_TARGETS.md) · [Plugin SDK 2](PLUGINS.md)

Machine-readable registry：`data/plugins/targets.json`。

```sh
lunadash-create-plugin --list
```

用 `--target`、`--id`、`--output` 產生 native effect 專案（標示 `(C example)` 的 target 可加 `--language c`），產生器依各 target 在 registry 的 `example` 直接寫出檔案，專案使用 LunaDash CMake SDK。這是開發 API，不保證跨 SDK major 的 native binary ABI。

## Native hook targets

所有 target 都是合成器原生 hook。視覺類的 Quickshell／OpenGL slot 屬於舊 QML 殼層，已隨之移除。

每個 target 的 registry `response` schema 才是正式規格；下表只是摘要，並列出 schema 無法表達、需要 host 狀態的檢查。

| Target | Response | 約束 |
| --- | --- | --- |
| `window-animation` | duration/offset/opacity/scale/easing | 完整 profile；桌面 animation off 仍優先 |
| `shell-animation` | duration | 0–600 ms |
| `window-rules` | workspace/maximized | initial map，workspace 必須有效 |
| `window-layout` | windows[] rectangles | ID 必須一一對應、正尺寸、在 work area |

一般 layout 禁止 overlap。Native replacement 宣告 `windowTemplate: stacking` 可選 host 的 persistent stacking strategy；hook context 會提供 `windowTemplate` 與 `allowOverlap`。Host 仍持有 membership、focus、minimize/maximize 與 workspace state。

Stacking 時 Alt+left-drag 移動，Shift+Alt+left-drag resize；focus-to-front。範例：[stacking-windows](../../examples/plugins/stacking-windows/README.md)。

## Registry 欄位

Plugin manager、SDK validator 與 scaffolder 不含任何針對特定 target 的程式碼；target 需要的一切都寫在 `targets.json` 的 entry：

| 欄位 | 用途 |
| --- | --- |
| `id`、`name`、`category`、`types`、`selection` | 識別、設定頁分類、支援的 plugin type，以及是否只能啟用一個 plugin（`single`） |
| `settings` | Built-in 選項（settings schema） |
| `response` | `PluginManager::filter` 對每個 plugin 結果檢查的 schema。Object 必須剛好包含 `properties`；array 用 `items`；integer/number 支援 `minimum`/`maximum`；任何值都可用 `enum`。沒有 `response` 的 target 會拒絕所有 plugin 結果 |
| `options` | 該 target 的 plugin 可在 manifest 宣告的額外欄位（settings schema），例如 `windowTemplate`；其他 target 會拒絕 |
| `replaceOnly` | 需要 `mode: replace` 的選項值，例如 `{"windowTemplate": ["stacking"]}` |
| `removedOptions` | 已移除的 manifest 欄位與使用時顯示的替代提示 |
| `example` | Scaffold 資料：產生專案用的 `settings`（與 `response` property 同名的設定會被複製到結果），以及選填的完整 `response`，有它才提供 C scaffold |

## 新增 target

1. 在 `targets.json` 加入 target，含 `settings`、`response`，需要時加 `options` 與 `example`。
2. 在擁有該狀態的 domain boundary 呼叫 `PluginManager::filter`（參考 `ExtensionHooks.cpp`）。只有需要即時狀態的檢查（例如視窗 ID、workspace 數量）才傳 validator。
3. Replacement ready 前一定保留 built-in。
4. 補文件與 regression test。Manager、scaffolder、CMake validator 與設定分類會自動讀取 registry。

Registry entry 本身不會自動實作 host hook；沒有實際 mount/call 的 target 不應發布。
