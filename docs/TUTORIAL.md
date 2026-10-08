# Demo 導覽

一步一步操作 console demo，把框架的每個核心行為走過一次。名詞定義見 [CONTEXT.md](../CONTEXT.md)，行為規格見 [SPEC.md](../SPEC.md)。

## 準備

```bash
cmake -B build && cmake --build build
./build/demofr
```

啟動後會自動匯入假模型（6 faces），然後出現 `>` 提示符。以下指令都在提示符後輸入。

## ① 看初始狀態

```
status
```

四個特徵全是「尚未辨識」。注意 `aag（internal）`——它是 Internal Feature，使用者理論上不需要認識它，接下來你會看到框架自己照顧它。

## ② 只辨識 fillet，帶參數 radius=3

```
recognize fillet=3
```

**看重點**：你沒有要求 aag，但畫面先出現「▶ 正在辨識 aag...」才輪到 fillet，報告裡 aag 標著「（自動補算上游）」。

fillet 依賴 aag，而 aag 還不存在——框架沿相依圖自己補齊，並即時回報它在做什麼（SPEC §3.3）。

## ③ 確認結果

```
status
```

fillet 顯示 `radius=3 mm`——你這次請求給的參數，不是預設值 1（SPEC §3.4 的第一優先）。

## ④ 重跑 aag，改角度

```
recognize aag=45
```

**看重點**：出現「⚠ fillet 已標記為過時（因 aag 重新辨識）」。

上游重新辨識成功，所有下游無條件失效——不比對結果，就算算出來一樣也照標（[ADR-0002](./adr/0002-always-invalidate-downstream.md)）。

## ⑤ 過時的結果還讀得到

```
status
```

fillet 變成「已過時」，但仍顯示 radius=3 那份舊結果。UI 可以先給使用者看舊的，讓他自己決定要不要重算（SPEC §3.5）。

## ⑥ 關鍵一步：只要求 rib

```
recognize rib
```

**看重點有兩個**：

1. fillet 是過時的，所以被自動補算；
2. 補算完再 `status` 看 fillet——**radius 還是 3**，不是預設的 1。

框架記住了「上次成功辨識實際使用的參數」，自動補算時沿用它（SPEC §3.4）。使用者調好的參數不會被無聲換回預設值。

## ⑦ 重新匯入模型

```
reimport
```

```
status
```

全部回到「尚未辨識」——模擬使用者在 CAD 裡改了模型：識別子全部重配，舊結果指向的面已經是錯的，所以直接清空而不是標過時（SPEC §3.6）。

## ⑧ 清空後直接要 rib

```
recognize rib
```

整條鏈 aag → fillet → rib 自動跑完，而且 fillet **依然用 radius=3**——參數設定存在 ParameterStore，不隨 reimport 消失（SPEC §3.6）。

## ⑨ 亂序請求

```
recognize fillet aag
```

故意把 fillet 寫在前面，執行順序仍然是 aag 先——呼叫端給的順序完全不影響結果，框架照相依圖做拓樸排序（SPEC §3.3）。

## ⑩ 離開

```
quit
```

## 還可以玩的

- `recognize hole`——hole 也依賴 fillet，觀察同一套補算機制
- `recognize aag` 之後馬上 `recognize rib hole`——一次請求多個目標
- `help`——重看指令清單
