# Feature Recognition (DemoFR)

一個特徵辨識框架，最終會以 library 形式內嵌在 CAD 應用中。它負責的是「特徵之間的相依關係、辨識順序、結果的新舊管理」，**辨識演算法本身不屬於這個框架**，而是從外面插進來的實作。

## Language

### 對象與來源

**Host CAD**:
內嵌本框架的 CAD 應用程式（NX、CATIA、SolidWorks 等）。本框架以 library 形式被它呼叫。
_Avoid_: Client, Platform, Vendor

**Import**:
把 Host CAD 的幾何一次性轉換成本框架自有格式的動作，同時建立 Host 識別子與本框架識別子的對照。之後所有辨識都只看轉換後的結果，不再回頭查詢 Host CAD。
_Avoid_: Load, Parse, Read

**BRep**:
匯入後的幾何拓樸資料，本框架的自有格式，是所有演算法唯一的幾何來源。由 Face、Edge、Trim、Vertex 組成。只描述幾何事實，不含任何推導出來的結論。
_Avoid_: RawData, ModelRawData, Mesh, Geometry

**Model**:
一次辨識作業的對象：一份匯入後的 BRep，連同它所有的 Feature 結果。
_Avoid_: Part, Document, File

### 辨識

**Feature**:
由演算法算出、會過時、可以被其他 Feature 依賴的產物。Hole、Fillet、Rib 是使用者看得見的 Feature；AAG 也是 Feature，只是不對使用者顯示。相依與過時的規則一律以 Feature 為單位，不分內外。
_Avoid_: Shape, Pattern, Artifact, Entity

**Internal Feature**:
不對使用者顯示的 Feature，例如 AAG。使用者不需要知道它存在，但別的 Feature 可以依賴它。相對的是 User-Visible Feature。
_Avoid_: Hidden feature, Intermediate data

**FeatureInstance**:
在模型上找到的單一具體特徵，例如「這個零件上的第 3 個孔」。一次辨識會產生零到多個 FeatureInstance。
_Avoid_: Occurrence, Hit, Result

**AAG**:
面鄰接圖。從 BRep 推導出來、供其他辨識演算法使用的 Internal Feature。邊的凹凸性這類推導結論屬於 AAG，不屬於 BRep。
_Avoid_: Graph, FaceGraph, AdjacencyGraph

**Recognizer**:
某一種 Feature 的辨識實作。它向框架註冊自己、宣告依賴哪些 Feature，並在被呼叫時產生結果。框架不認得任何具體的 Recognizer。
_Avoid_: Strategy, Algorithm, Executor

**Recognition**:
對某個 Feature 執行它的 Recognizer、產生結果。同一個 Feature 只保留最後一次的結果，重跑就覆蓋。
_Avoid_: Detection, Extraction, Analysis

**Parameter**:
控制一次辨識行為的設定值，例如 AAG 的角度門檻、Fillet 的半徑上限。每個 Recognizer 為自己的參數提供預設值；使用者調整過**並且實際跑過辨識**的值會被記錄下來、取代預設值，之後的辨識（包含自動補算上游）都採用它。單純在畫面上改動而沒有執行辨識，不會改變任何東西。
_Avoid_: Config, Setting, Option

### 相依與新舊

**Dependency**:
Feature 之間單向的「必須先有」關係。Fillet 依賴 AAG，意思是沒有有效的 AAG 就無法辨識 Fillet。由 Recognizer 自己宣告。
_Avoid_: Prerequisite, Relation

**Upstream / Downstream**:
被依賴的一方是 Upstream，依賴別人的一方是 Downstream。AAG 是 Fillet 的 Upstream。

**Outdated**:
某個 Feature 的既有結果不再保證正確，因為它的 Upstream 被重新辨識過。結果本身還在，只是不可信。重新匯入模型不會造成 Outdated——那會直接清空所有結果。
_Avoid_: Dirty, Invalid, Stale
