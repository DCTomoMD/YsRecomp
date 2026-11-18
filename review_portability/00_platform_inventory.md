# M0-00：プラットフォーム依存の棚卸し（移植可能性の調査）

- 対象ブランチ：`feature/japanese-disc-support`（基準 `7452c1b`、PCERecomp `1b9d25a`）
- 調査日：2026-11-19
- 出発点：`review_jp/00_repository_inventory.md` の 14 章（Windows 専用部分）と 17 章（危険箇所）。同じ内容は繰り返さず、include・リンク・CMake ターゲットの依存まで追った。
- 方法：ソースの読み取りと grep のみ。
  - `roms/` の中身は読んでいない。
  - ビルド、コード生成、試験の実行、ダウンロード、インストールはしていない。
  - **macOS ではまだビルドしていない。本書の「PORTABLE」は、コードを読んだうえでの判断で、コンパイルで確かめたものではない。**
- Windows でのビルド・試験が必要な項目は「**Windows VM で確認が必要**」と書いた。

`PCERecomp/` は `external/PCERecomp/` を指す。行番号は基準コミット時点のもの。

---

## 1. 表記

### 1.1 確度

| ラベル | 意味 |
|---|---|
| **[C]** CONFIRMED | ソースで直接確認した事実 |
| **[A]** ASSUMPTION | 推測。検証が必要 |
| **[U]** UNKNOWN | 不明。ビルドや実行をしないと決まらない |
| **[B]** BLOCKER | 解消しないと、その組み合わせのビルド・試験が先へ進めない |

### 1.2 分類（全文書で共通）

| 分類 | 意味 |
|---|---|
| PORTABLE | 変更なしで両 OS に通る |
| SDL3で吸収可能 | SDL3 の既存 API に置き換えれば共通にできる |
| 標準C/C++で吸収可能 | 標準 C（または POSIX と MSVC の小さな差の吸収）で共通にできる |
| Windows hostへ隔離 | Windows 専用の実装として host 層に閉じ込める（多くは既に `#ifdef _WIN32` で閉じている） |
| macOS hostが必要 | macOS 用の実装を新しく書く必要がある |
| CMake差分 | ビルド設定の違いだけで済む |
| PCERecomp側の修正が必要 | サブモジュールを変える必要がある（運用方法はユーザーが決める） |
| BLOCKER | 上記 1.1 と同じ |
| UNKNOWN | 上記 1.1 と同じ |

---

## 2. 結論の要約

1. **OS の API を直接呼ぶ C のファイルは少ない** [C]。
   - YsRecomp 側：`runtime/app/vblank.c`、`runtime/app/gpu_time.c` の 2 本。
   - PCERecomp 側：`runtime/util/u8file.c`、`tools/trace/profiler.c` の 2 本。
   - 4 本とも `#ifdef _WIN32` の分岐を持ち、`#else` 側に非 Windows の実装かスタブがある。
2. **コンパイラの差もすでに吸収されている** [C]。
   - `huc6280_exec.h:19-23`：`__forceinline` と `__attribute__((always_inline))` の分岐がある。
   - `cdimage.c:12-16, 85-89`：`_fseeki64` / `_ftelli64` と `fseeko` / `ftello` の分岐がある。
   - `launcher_image.c:20-26`：`#pragma warning` は `_MSC_VER` のときだけ有効。
   - `codegen.py:285`：生成コードの `#pragma warning(disable: 4102)` も `_MSC_VER` のときだけ有効。
3. **XInput、WASAPI、COM、Direct3D をゲーム本体が直接使う箇所はない** [C]。
   - 入力は SDL3 の Gamepad API、音声は SDL3 の AudioStream、描画は SDL_Renderer を使う。
   - D3D11 / DXGI は `gpu_time.c` にしかなく、`--frame-timing` のときだけ使う。
4. **移植の作業の中心はコードではない** [C]。
   - CMake：ROM の固定名、Python の探し方、`gg_trace.exe`、`.rc`、ビルド種別。
   - 書き込み先：実行ファイルの隣に書く。
   - 試験とパッケージのツール：`.exe`、アイコンの確認、`tasklist`、`roms\` のパス。
   - 開発環境：`.venv` の Python が 3.10.7 で、`tomllib` がない。
5. **未確定の大きな項目** [U]
   - 生成 C（約 40 MB を 24 本に分割）を Apple Clang でコンパイルしたときの時間とメモリ。
   - Metal での描画結果と、実時間の pacing。
   - 浮動小数点の縮約（FMA）による音声・数値の差。

---

## 3. ターゲットと依存のグラフ [C]

```
SDL3::SDL3-static (FetchContent release-3.2.30, CMakeLists.txt:51-54)
   ^
   |                       pce_core (PCERecomp, 28 sources, no SDL)  <- pce_input
   |                         ^   (includes util/u8file.c -> windows.h only in _WIN32)
pce_app (runtime/app/*, CMakeLists.txt:64-95) --PUBLIC--> pce_core, SDL3
   |   WIN32: + dwmapi (93-95)            includes ${sdl3_SOURCE_DIR}/src/video (97, stb_image)
   |
   +-- pce_runtime (100-101) : pce_core pce_app pce_input trace_profiler SDL3
   +-- app_tests  (117-118)
trace_profiler (PCERecomp CMakeLists.txt:63-67) WIN32: + dbghelp winmm
pce_trace (105-109) : pce_trace_executable() -> pce_core pce_input trace_profiler + SDL3

[ROM と BIOS が CMakeLists.txt:24-25 の固定名で存在するときだけ (240)]
ys_recomp (256-263)    : games/ys/generated/c/rc_table.c + rc_part00..23.c   MSVC: /W1 /bigobj /MP, IPO OFF
equip_frames (268-279) : running pce_trace during the build produces equip_frames.c
ys_trace (286-291)     : ys_recomp equip_frames SDL3
YsRecomp (295-302)     : ys_recomp pce_app pce_input equip_frames trace_profiler SDL3   WIN32: + ysrecomp.rc (296-298)
```

- 生成ステップ（`add_custom_command`）は `${PYTHON_EXE}` を起動する（`CMakeLists.txt:250-255, 269-276`）。
  - `PYTHON_EXE` は `find_program(PYTHON_EXE NAMES python python3)`（144）で探す。
- OS に依存するライブラリのリンク：
  - `dwmapi`（YsRecomp 側、93-95）
  - `dbghelp`、`winmm`（PCERecomp 側、`PCERecomp/CMakeLists.txt:66`）
  - どちらも `if(WIN32)` で囲まれている。

---

## 4. OS・コンパイラに依存するファイルの一覧

| ファイル | 依存 | ガード | 分類 | 確度 |
|---|---|---|---|---|
| `runtime/app/vblank.c:6-47` | DWM、QPC、SetPriorityClass | `_WIN32` と `#else` スタブ | Windows hostへ隔離 ／ macOS hostが必要（任意） | [C] |
| `runtime/app/gpu_time.c:7-187` | D3D11、DXGI（COM の C マクロ） | `_WIN32` と `#else` スタブ | Windows hostへ隔離 ／ macOS hostが必要（任意） | [C] |
| `runtime/app/launcher_image.c:12-26` | SDL ソース内の stb_image、MSVC の pragma | `_MSC_VER` | PORTABLE | [C]（SDL 3.2.30 のヘッダー内容は [U]） |
| `runtime/app/ysrecomp.rc`、`.ico` | Windows リソース | CMake の `if(WIN32)` | Windows hostへ隔離 ／ macOS hostが必要（Info.plist、icns） | [C] |
| `runtime/main.c:1947, 2554` | `SDL_GetBasePath` の隣に書く | なし | SDL3で吸収可能（`SDL_GetPrefPath`）／ macOS hostが必要 | [C] |
| `tests/app/app_tests.c:3334-3338` | `_fullpath` と `realpath` | `_WIN32` | PORTABLE | [C] |
| `PCERecomp/runtime/util/u8file.c:3-23` | `_wfopen`、`MultiByteToWideChar` | `_WIN32` | PORTABLE（非 Windows 側は `fopen`） | [C] |
| `PCERecomp/runtime/cd/cdimage.c:12-16, 85-89` | `_fseeki64`、`_ftelli64` | `_MSC_VER` | PORTABLE | [C] |
| `PCERecomp/runtime/cpu/huc6280_exec.h:19-23` | `__forceinline` | `_MSC_VER` | PORTABLE | [C] |
| `PCERecomp/tools/trace/profiler.c:4-213` | DbgHelp、winmm、x64 のレジスタ | `_WIN32` と `#else` スタブ | Windows hostへ隔離（macOS 版は任意） | [C] |
| `PCERecomp/tools/recomp/pcerecomp/codegen.py:285` | 生成コードに MSVC の pragma を書く | `_MSC_VER` | PORTABLE | [C] |
| `CMakeLists.txt` | 1 章の各行 | 一部は `if(MSVC)` / `if(WIN32)` | CMake差分 | [C] |
| `build.ps1`、`PCERecomp/build.ps1`、`PCERecomp/tools/reference_trace/build.ps1` | PowerShell、VS のパス | ― | Windows hostへ隔離（macOS 用のスクリプトが必要） | [C] |
| `tools/trace/check_all_sessions.py:41` | `tasklist` | なし | Windows hostへ隔離 | [C] |
| `tools/package/make_package.py:37-44, 69` | PE の確認、アイコンの確認、CRLF | なし | BLOCKER（macOS のパッケージ） | [C] |

---

## 5. 開発環境の現状（macOS、読み取りで確認）

| 項目 | 状態 | 確度 |
|---|---|---|
| `.venv/bin/python` | `~/.pyenv/versions/3.10.7` へのリンク。**3.10.7 には `tomllib` がない**（3.11 以上が必要。CLAUDE.md の作業規則 v2 にも反する） | [C] **[B]** |
| PATH 上の `python3` | pyenv の shim（3.10.7） | [C] |
| ほかの Python | `/usr/local/bin/python3.11`、`/opt/homebrew/bin/python3.14`、`python3.15` がある | [C] |
| CMake、Ninja、Xcode CLT のバージョン | 調べていない | [U] |
| `.DS_Store` | 作業ツリーの直下にある。Git には追跡されていない（`.git/info/exclude` で除外） | [C] |

---

## 6. 調査で実行したコマンド（すべて読み取りのみ）

- `pwd`、`git branch --show-current`、`git status --short`
- `ls`、`wc -l`、`sed -n`、`grep -n` / `grep -rnE`。対象は追跡中のソース、CMake、Python、文書
- `ls -la .venv/bin/python*`、`.venv/bin/python --version`、`which python3 python3.11 python3.14`、`ls /opt/homebrew/bin/python3*`
- 読み取り専用の調査エージェント 3 本：アプリ層、PCERecomp、ビルド・ツール・試験。報告の主な行番号は、この文書を書く前に grep と sed で確かめ直した
