# M0-01：Windows 依存の詳細（調査項目 1〜13）

表記（[C] / [A] / [U] / [B] と 9 つの分類）は `00_platform_inventory.md` の 1 章に従う。
`PCERecomp/` は `external/PCERecomp/` を指す。

---

## 項目 1：Windows.h、Win32 API、COM、Direct3D、DXGI、XInput、WASAPI

| 箇所 | API | 呼び出し元 | ガードと `#else` | 分類 | 確度 |
|---|---|---|---|---|---|
| `runtime/app/vblank.c:6-9` | `<windows.h>`、`<dwmapi.h>` | ― | `#ifdef _WIN32`。`#else` は 36 行目から | Windows hostへ隔離 | [C] |
| `vblank.c:13-16` | `DwmGetCompositionTimingInfo`、`ZeroMemory`、`FAILED` | `vblank_query()` ← `main.c:1099` `screen_vblank()` ← `main.c:1120`（`screen_update_pacing`。1155、1223、3120 から呼ばれる）と `main.c:3192`（毎フレーム、`pace_plan` へ渡す） | `#else` は 0 を返す（41-46） | Windows hostへ隔離 ／ macOS hostが必要（任意） | [C] |
| `vblank.c:20-21` | `QueryPerformanceFrequency` | 同上 | ― | SDL3で吸収可能（`SDL_GetPerformanceFrequency`） | [C] |
| `vblank.c:34` | `SetPriorityClass(ABOVE_NORMAL)` | `vblank_raise_priority()` ← `main.c:2880`（`--turbo` のときは呼ばない） | `#else` は空の関数（37-39） | Windows hostへ隔離（macOS では任意。QoS か `setpriority`） | [C] |
| `runtime/app/gpu_time.c:7-12` | `COBJMACROS`、`<d3d11.h>`、`<dxgi.h>` | ― | `#ifdef _WIN32`。`#else` は 165 行目から | Windows hostへ隔離 | [C] |
| `gpu_time.c:67-162` | `SDL_PROP_RENDERER_D3D11_DEVICE_POINTER` / `_SWAPCHAIN_POINTER`、`ID3D11Device_CreateQuery`、`ID3D11DeviceContext_Begin/End/GetData`、`IDXGISwapChain_GetFrameStatistics`、`*_Release` | `gpu_time_init` ← `main.c:2725`（`--frame-timing` のときだけ）。`gpu_time_mark` ← 1320、1340、1885、3203。`gpu_time_presented` ← 1333。`gpu_time_report` ← 3540。`gpu_time_quit` ← 3596。init と report、quit 以外は `if (g_gpu_time)` の中 | `#else` はスタブ 5 本で、`init` は 0 を返す（166-186） | Windows hostへ隔離 ／ macOS hostが必要（任意、Metal） | [C] |
| `PCERecomp/runtime/util/u8file.c:3-17` | `MultiByteToWideChar`、`_wfopen`、`wchar_t[1024]` | `u8_fopen` を呼ぶ全箇所 | `#else` は `fopen`（18-23） | PORTABLE | [C] |
| `PCERecomp/tools/trace/profiler.c:4-209` | DbgHelp、`timeBeginPeriod`、`SuspendThread`、`GetThreadContext`、`RtlVirtualUnwind`（x64 専用） | `profiler_start/stop` ← `main.c:2886, 3499`（`--profile`） | `#else` のスタブは -1 を返す（210-213） | Windows hostへ隔離 | [C] |
| `tests/app/app_tests.c:3334-3338` | `_fullpath` | `absolute_path()`（app_tests.c:3332） | `#else` は `realpath` | PORTABLE | [C] |

- **XInput**：直接の使用はない。`main.c:2679` のコメントで、SDL が内部で使うと書かれているだけ [C]。
- **WASAPI、mmdevice、COM の初期化**：使用なし [C]。
- **見つからなかった API**：`Sleep`、`CreateThread`、`_beginthreadex`、`CRITICAL_SECTION`、`GetModuleFileName`、`DwmFlush` [C]。

---

## 項目 2：SDL3 で吸収済みの機能と、そうでない機能

| 機能 | 使っている SDL3 API（file:line） | 状態 | 分類 |
|---|---|---|---|
| ウィンドウと描画 | `SDL_CreateWindowAndRenderer`（main.c:2709）。**SDL_Renderer を使う（SDL_GPU ではない）**。ほかに `SDL_RenderGeometry`、`SDL_ComposeCustomBlendMode`（gpu_draw.c:55、present.c:308）、描画先テクスチャ、`SDL_RenderReadPixels` | 吸収済み。macOS では Metal バックエンドになる [A]。独自ブレンドが拒否された場合の代替経路あり（gpu_draw.c:62） | PORTABLE（実行での確認は [U]） |
| 表示モードと全画面 | `SDL_GetFullscreenDisplayModes`、`SDL_SetWindowFullscreenMode`（1184）、`SDL_GetDisplayUsableBounds`、`SDL_SetHint(FORCE_RAISEWINDOW)`（1189） | 吸収済み。macOS の Spaces と排他全画面の動きは Windows と違う [A] | PORTABLE（[U]） |
| HiDPI | `SDL_WINDOW_HIGH_PIXEL_DENSITY` を指定していない [C]（エージェント報告。grep で `HIGH_PIXEL` が見つからないことを確認） | Retina では point 単位（論理解像度）で描かれる [A] | macOS hostが必要（調整） |
| 音声 | `SDL_OpenAudioDeviceStream`（2690）、`SDL_PutAudioStreamData`、`SDL_SetAudioStreamFrequencyRatio` | 吸収済み | PORTABLE |
| 入力 | `SDL_INIT_GAMEPAD`（2682）、`SDL_OpenGamepad`（input.c:257）。キーボードはスキャンコード | 吸収済み。Mac の Option+Return、F6/F12 の Fn キーは操作上の違い | PORTABLE |
| ダイアログ | `SDL_ShowFileDialogWithProperties(OPENFOLDER)`（main.c:186）、`SDL_ShowSimpleMessageBox`（221） | 吸収済み。メインスレッドから呼んでいる（macOS の要件に合う）[C] | PORTABLE |
| 時間 | `SDL_GetTicksNS`、`SDL_DelayNS`、`SDL_DelayPrecise`（1327）、`SDL_GetPerformanceCounter` | 吸収済み | PORTABLE |
| スレッドと同期 | `SDL_CreateThread`（main.c:1855 prep、2897 game、launcher_sound.c:139 sfx）、`SDL_Mutex`、`SDL_Semaphore`、`SDL_SpinLock`、`SDL_AtomicInt`、`SDL_SetCurrentThreadPriority` | 吸収済み | PORTABLE（スタックの大きさは項目 4） |
| ファイルシステム | `SDL_GetBasePath`（main.c:1947、2554、app_tests.c:485）、`SDL_CreateDirectory`、`SDL_GlobDirectory`（disc_id.c:165）、`SDL_RenamePath`、`SDL_RemovePath`。**`SDL_GetPrefPath` は使っていない** | 一部だけ吸収済み。書き込み先の方針は項目 8 | SDL3で吸収可能 |
| **吸収されていないもの** | 合成器の vblank 時刻（vblank.c）、GPU タイムスタンプ（gpu_time.c）、プロセスの優先度、サンプリングプロファイラ、アプリの包装（.rc / Info.plist）、書き込み先の OS 慣習 | ― | Windows hostへ隔離 ／ macOS hostが必要 |

---

## 項目 3：パスの処理

| 項目 | 箇所 | 状態 | 分類 | 確度 |
|---|---|---|---|---|
| UTF-16、`wchar_t`、`_wfopen` | `u8file.c` の中だけ | macOS 側は UTF-8 のまま `fopen` | PORTABLE | [C] |
| `u8_fopen` を通さない `fopen` | PCERecomp：bios_profile.c:156、lockstep.c:167、coverage.c:245、cd_info.c:51、pce_trace.c の多数、profiler.c:181 | macOS では影響なし。Windows で非 ASCII のパスを使うと開けない（既存の問題） | PORTABLE（macOS） | [C] |
| `\` も区切りとして扱う | main.c:238、243、2589、2613、2733、2758、3623、disc_id.c:52、172、app_tests.c:3229 | `/` と `\` の両方で分ける。POSIX で名前に `\` を含むファイルは誤って分かれる（まれ） | PORTABLE | [C] |
| ドライブ文字 | app_tests.c:106、119-128、430、444（設定の往復試験の文字列のみ） | 値として扱うだけ | PORTABLE | [C] |
| CUE の FILE 名 | `cdimage.c:65-72`（絶対パスの判定は `/`、`\`、`X:`。それ以外は `"%s/%s"` で結合）、197-200（引用符の中をバイト列のまま使う） | NFC への正規化はない。`\` を含むサブフォルダ指定は macOS で開けない | PORTABLE ／ 日本語名は [U]（review_jp/00 の 17 章 3 項） | [C] |
| `cdimage.py:45` | CUE を latin-1 で読む。結合は `os.path.join`。NFC への正規化なし | UTF-8 の日本語名は文字化けして開けない。C 側（バイト列）とも食い違う。Windows でも同じ | PCERecomp側の修正が必要（日本版の時点で） | [C] |
| セッションのヘッダーに書かれたパス | `games/ys/sessions/*.txt` の `game:`、`bios:` 行が `roms\...` や `D:\Roms\...` の形（エージェントの集計では後者が 32 本） | `run_bios_check.py:64-66` と `check_all_sessions.py:60, 66` が `os.path.join(ROOT, …)` で結合する。macOS では見つからず 77 になる | **BLOCKER**（試験の偽 skip） | [C] |
| `realpath` のバッファ | app_tests.c:3337（1024 バイト） | macOS の `PATH_MAX` は 1024 なので安全 | PORTABLE | [C] |
| 実行ファイルの隣に書く | 項目 8 を参照 | .app の中では書けない | macOS hostが必要 | [C] |
| 既定のセーブ先が相対パス | main.c:2616 `"saves/%.*s.brm"`（コマンドラインで ROM を渡したときだけ） | Finder から起動すると作業ディレクトリは `/` [A] | SDL3で吸収可能 | [C] |

---

## 項目 4：時間とスレッド

| 項目 | 箇所 | 状態 | 分類 | 確度 |
|---|---|---|---|---|
| QueryPerformanceCounter | vblank.c:20-21 のみ。そのほかは `SDL_GetPerformanceCounter` | `screen_vblank`（main.c:1099-1104）は、DWM の QPC 時刻を SDL の counter に換算する。Windows では両者が同じ時計なので成り立つ | Windows hostへ隔離 | [C] |
| macOS で同じことをする場合 | ― | `SDL_GetPerformanceCounter` は macOS では `mach_absolute_time` を使う [A]。macOS 版の vblank 時刻も、この単位で返す必要がある | macOS hostが必要 | [A] |
| Sleep | 使用なし。`SDL_DelayNS` / `SDL_DelayPrecise` を使う | ― | PORTABLE | [C] |
| スレッド | 3 本：game（`emu_thread`、main.c:892）、prep（`prep_thread`、1805）、sfx（`capture_thread`、launcher_sound.c:132） | すべて `SDL_CreateThread`。スタックの大きさは指定していない | PORTABLE | [C] |
| ロックと順序 | `g_prep.lock` → `frame_lock` → `slot_lock`（main.c:1438 に記述）。`SDL_Mutex` と `SDL_Semaphore`、スピンロックは 417、447、495 | OS に依存しない | PORTABLE | [C] |
| スタック上の大きなバッファ | 64 KB を超えるのは `app_tests.c:103, 113, 125`（`Config` 約 69.6 KB × 3）と `432, 441` だけ。どれもメインスレッドで動く | 本体の大きなバッファは static かヒープ（`Emu`、`g_prep`、`PrepSlot`、各モジュールの static 作業領域） | PORTABLE | [C] |
| 2 次スレッドのスタック（macOS の既定は 512 KB [A]） | game スレッドが生成 C を動かす | JSR などの関数の出口は C の関数から return し、dispatcher に戻る（`codegen.py:6, 136, 201-206`）。そのため C の再帰は深くならない [C]。ただし、巨大な生成関数のスタックフレームが -O0 / -O2 でどれだけになるかは不明 | UNKNOWN（M3 で測る。必要なら `SDL_PROP_THREAD_CREATE_STACKSIZE_NUMBER`） | [U] |

---

## 項目 5：MSVC 固有の機能

| 項目 | 箇所 | 分類 | 確度 |
|---|---|---|---|
| `__forceinline` | `huc6280_exec.h:19-23`。`_MSC_VER` でないときは `inline __attribute__((always_inline))`。使用は exec.h で 260 箇所、ほかに `hu_pre` / `hu_post`（huc6280_recomp.h:22, 31） | PORTABLE | [C] |
| `#pragma warning` | launcher_image.c:20-26、生成コードの先頭（codegen.py:285）。どちらも `_MSC_VER` で囲む | PORTABLE | [C] |
| `__declspec`、`#pragma comment(lib)`、`#pragma pack`、`*_s` 関数、`_stricmp`、`_snprintf`、`_strdup`、`__int64`、`_aligned_malloc`、intrinsic、`__assume` | runtime/、tools/trace、tests の中に見つからない | PORTABLE | [C] |
| packed struct、構造体のまま fread/fwrite | なし。BRAM は 0x800 バイトの配列、`launcher_sfx.bin` と `menu_font.bin` は幅を固定して書く | PORTABLE | [C] |
| `/utf-8` | CMakeLists.txt:7-17。Clang はソースを UTF-8 として読むのが既定。非 ASCII の文字列定数は `\x` で書かれている（app_tests.c:99） | CMake差分（変更不要） | [C] |
| `_CRT_SECURE_NO_WARNINGS` | CMake の `if(MSVC)` 内 | PORTABLE | [C] |

---

## 項目 6：データ型

| 項目 | 箇所 | 分類 | 確度 |
|---|---|---|---|
| `long`（Win64 では 4 バイト、macOS では 8 バイト） | 本体で約 171 箇所。PCERecomp では pce.h:140、cdimage.c:54-62 など。カウンタ、フレーム番号、`ftell` に使う。バイナリファイルに書く箇所はない | PORTABLE | [C] |
| `%ld` と引数の型 | 抜き取りで確認したところ、すべて一致していた。全件の確認は Clang の `-Wformat` で行う | PORTABLE ／ 全件は [U] | [C]/[U] |
| ポインタ幅と整数の変換 | `intptr_t` やポインタを int に変換する箇所はない。ハッシュは `memcpy` を使う（main.c:1532-1535） | PORTABLE | [C] |
| アラインメント | `coverage.c:234` の `*(const uint64_t *)` は、要素が本当に uint64_t なので問題ない | PORTABLE | [C] |
| strict aliasing | 型を偽った読み書きは見つからない | PORTABLE | [C] |
| 符号付き整数の縮小変換 | `(int8_t)` などへの変換（huc6280_exec.h:224, 1424、psg.c ほか）は処理系定義だが、MSVC と Clang で同じ結果になる | PORTABLE | [C] |
| 符号付き整数のオーバーフロー | 全件は確認していない。ハッシュは uint32_t を使う | UNKNOWN | [U] |
| `char` の符号 | MSVC x64 も Apple arm64 も signed [A]。`tolower` には `(unsigned char)` を渡している | PORTABLE | [C] |
| エンディアン | 両方とも little-endian。`launcher_sfx.bin` はネイティブの順で書くので、OS をまたいでも互換 | PORTABLE | [C] |
| 浮動小数点の縮約（FMA） | Apple Clang の arm64 は既定で `-ffp-contract=on`、MSVC x64 は縮約しない [A]。影響を受けうるのは psg.c:58, 73, 279、mix.c、cdaudio.c:114-168、adpcm.c、ys_trace_ext.c（`double` 27 箇所） | CMake差分（`-ffp-contract=off`）。採用するかはユーザーが決める | [A] |

---

## 項目 7：ビルド

| 項目 | 箇所 | 分類 | 確度 |
|---|---|---|---|
| PowerShell 専用 | `build.ps1`（VS BuildTools の cmake.exe のパスを固定 17-20、`-G "Visual Studio 17 2022" -A x64` 34-37、`--config Release`、出力の絞り込みが `warning C\d`、並列数 = コア数 / 2、ctest `--repeat until-pass:2`）。PCERecomp/build.ps1 と reference_trace/build.ps1 も同じ形 | Windows hostへ隔離。macOS 用のシェルスクリプトか CMakePresets が必要 | [C] |
| ジェネレータの固定 | build.ps1 だけで固定している。CMakeLists.txt では固定していない | CMake差分 | [C] |
| 単一構成のジェネレータ | `CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON`（58）は Release 構成のときだけ効く。Ninja や Makefiles で `CMAKE_BUILD_TYPE` を指定しないと、最適化も LTO もない | CMake差分 | [C] |
| 出力先 | `CMAKE_RUNTIME_OUTPUT_DIRECTORY`（21）。VS では `build/Release/`、Ninja では `build/` | CMake差分 | [C] |
| `.exe` の固定 | CMakeLists.txt:44 `gg_trace.exe`。PCERecomp/CMakeLists.txt:114 `gg_cdinfo.exe`。Python 側の既定値：pcerecomp_dir.py:23、check_all_sessions.py:32、run_bios_check.py:20、check_session.py:26、run_perf_check.py:76、make_package.py:19, 65、run_package_test.py:72 | CMake差分 ／ Windows hostへ隔離 | [C] |
| `.dll` | 固定名なし（SDL は静的リンク） | PORTABLE | [C] |
| `.rc` | 296-298 | Windows hostへ隔離。macOS は `MACOSX_BUNDLE`、Info.plist、icns | [C] |
| `/MT` | `CMAKE_MSVC_RUNTIME_LIBRARY`。Clang では意味がない | CMake差分 | [C] |
| Python の選択 | `find_program(PYTHON_EXE NAMES python python3)`（144）。PATH 上で最初に見つかったものを使う（ここでは 3.10.7） | CMake差分 ／ **BLOCKER**（3.11 以上を保証する仕組みがない） | [C] |
| SDL3 の取得 | `FetchContent_Declare(SDL3 URL …release-3.2.30.tar.gz DOWNLOAD_EXTRACT_TIMESTAMP TRUE)`（51-53）。`URL_HASH` なし | CMake差分（ハッシュの固定を推奨） | [C] |
| ROM の固定名 | 24-25。存在しないと YsRecomp、ys_trace、67 本の試験が定義されない（240） | CMake差分（M2 で対応予定） | [C] |
| `file(GLOB)` | 249（`pcerecomp/*.py`、依存の追跡だけに使う） | PORTABLE（`._*.py` が混ざっても依存が増えるだけ） [A] | [C] |

---

## 項目 8：保存先（設定、セーブ、ログ、パッケージ）

| 出力 | 決める場所 → 書く場所 | 現在の置き場所 | 分類 | 確度 |
|---|---|---|---|---|
| `YsRecomp.ini` | main.c:2553-2556（`SDL_GetBasePath()`）→ `config_save`（config.c:45-55。main.c:229、278、298 から呼ばれる） | 実行ファイルの隣 | SDL3で吸収可能 ／ macOS hostが必要 | [C] |
| `saves/<cue>.brm` | main.c:245（アプリモード、ini の隣）、2616（CLI、相対パス）→ 3620-3630 | ini の隣 ／ 作業ディレクトリ | 同上 | [C] |
| `sessions/` | main.c:2592 → `recording_start/save`（466-510、一時ファイルに書いてから `SDL_RenamePath`） | ini の隣 | 同上 | [C] |
| `menu_font.bin` | main.c:2596 → menu_font.c:59-73（main.c:1014 は game スレッド、1981） | ini の隣 | 同上 | [C] |
| `launcher_sfx.bin` | main.c:2019、2761 → launcher_sound.c:273-291 | ini の隣 | 同上 | [C] |
| `launcher/*.jpg`（読み込みのみ） | main.c:1947-1958 | 実行ファイルの隣。なければ `PCE_DOCS_DIR`（CMake:301、ソースの絶対パス） | PORTABLE（.app では Resources に置く） | [C] |
| ログ | stdout / stderr のみ。ログファイルはない | ― | PORTABLE | [C] |
| 試験の出力 | `--config work/…`、`build/app_output`、`tempfile`。app_package は実行ファイルの隣に書く。app_menu（app_tests.c:485）も `SDL_GetBasePath()` に書く | ― | ― | [C] |
| パッケージ | make_package.py → `dist/YsRecomp/`（YsRecomp.exe、README.txt を CRLF で、launcher/、menu_font.bin、launcher_sfx.bin）と zip | Windows の形式 | macOS hostが必要（.app、DMG または zip） | [C] |

- **macOS で問題になる点** [A]
  - `.app` の中では、`SDL_GetBasePath()` は `Contents/Resources/` を返す。
  - そこに書くと、署名が壊れるか、書き込めない。
  - .app の外に置いた素の実行ファイルなら、今と同じ動作になる。
- どの方式にするか（ポータブルな配置か、`~/Library/Application Support/`）は、ユーザーが決める事項（03 の D3）。

---

## 項目 9：external/PCERecomp 内の Windows 依存と、運用方法の比較

### 9.1 依存（4 箇所とも対応済み）

| 箇所 | 内容 | 判定 | 確度 |
|---|---|---|---|
| `runtime/util/u8file.c:3-23` | `_wfopen`。`#else` は `fopen` | 修正は不要 | [C] |
| `runtime/cd/cdimage.c:12-16, 85-89` | `_fseeki64` / `_ftelli64`。`#else` は `fseeko` / `ftello`。gnu11（`CMAKE_C_EXTENSIONS` の既定 ON）が前提 | 修正は不要 | [C] |
| `runtime/cpu/huc6280_exec.h:19-23` | `__forceinline`。`#else` は always_inline | 修正は不要 | [C] |
| `tools/trace/profiler.c:4-213` | DbgHelp。`#else` はスタブ | 修正は不要（macOS 版は任意） | [C] |

### 9.2 修正が必要、または望ましい箇所

| 箇所 | 内容 | 必要性 | 確度 |
|---|---|---|---|
| `tools/recomp/recomp.py:68` | `os.listdir` を `.endswith(".txt")` で絞るので `._x.txt` も拾う。`coverage.py:62` は ascii で読むので UnicodeDecodeError になる | 望ましい（exFAT、SMB、zip 経由のときだけ起きる） | [C] |
| `tools/recomp/pcerecomp/cdimage.py:45` | CUE を latin-1 で読み、NFC への正規化もしない | 日本版の CUE が非 ASCII の FILE 名を持つなら必要（Phase 2 で確定）[U] | [C] |
| `CMakeLists.txt:114` | `gg_cdinfo.exe`、`build/Release` を固定 | 望ましい（macOS では 77 で skip） | [C] |
| `CMakeLists.txt:92` | `find_program(PYTHON_EXE NAMES python python3)` | 親から `-DPYTHON_EXE=` で上書きすれば済む（変更は不要） | [C] |
| 生成器のシンボル（`huc6280_recomp.h:40-42`、`codegen.py:320-335`） | `recomp_lookup` と `recomp_rom_crc32` が 1 組しかない | 日米を 1 本の実行ファイルにするなら必要（版の問題で OS の問題ではない）。Phase 1 の設計課題 | [C] |

**結論**：macOS でビルドするためだけなら、PCERecomp を変更する必要はない [A]（M3 で確認する）。
変更が必要になるのは、`._*` への対策、日本語の CUE、日米を 1 本に同居させる場合の 3 点。

### 9.3 運用方法の比較（決定はユーザー）

| 観点 | A：上流へ提案する（PR） | B：自分のフォークに切り替える | C：パッチとして管理する（`patches/*.patch` を構成時に当てる） |
|---|---|---|---|
| 上流への追随 | 取り込まれれば最も楽 | 自分でマージを続ける必要がある | パッチの当て直しが必要（衝突すると壊れる） |
| 取り込みまでの時間 | 上流しだいで不明。拒否されることもある | すぐ使える | すぐ使える |
| `.gitmodules` | 変更なし | URL を自分のフォークに変える | 変更なし |
| 再現性 | 高い（コミットを固定） | 高い（コミットを固定） | 中程度（当てる手順が CMake かスクリプトに入る。当て忘れの危険） |
| 「PCERecomp に Ys のものを入れない」規則 | 汎用の修正しか通らないので、自然に守られる | 自分で守る必要がある | 自分で守る必要がある |
| 作業規則 v2 との関係 | 外部への送信になるので、ユーザーが行う | ユーザーの GitHub での操作 | リポジトリ内で完結する |
| 向いている修正 | `._*` の除外、`gg_cdinfo` のパス（汎用で小さい） | 日米同居のための生成器の変更（大きく、長く続く） | 試行中の一時的な修正 |

- 推奨（判断材料として）[A]
  - 汎用で小さい修正は A。並行して C で一時的に当てる。
  - 生成器を大きく変えるなら B。
  - M1 時点では、PCERecomp の変更は不要の見込み。

---

## 項目 10：生成コード rc_part00〜23.c を Apple Clang でコンパイルするときの問題

| 項目 | 内容 | 分類 | 確度 |
|---|---|---|---|
| 構文 | 関数ごとに `switch (c->pc)` で各命令のラベルへ `goto` する。各命令は `hu_op_XX(...)` を always_inline で展開する。computed goto（GNU 拡張）や MSVC 専用の構文はない（codegen.py:142-208） | PORTABLE | [C] |
| `/W1` に相当する設定 | Clang には警告フラグが付かない（CMake の else 節がない）。`-Wall` を付けると `-Wunused-label` が大量に出る。生成コードには `-w` か `-Wno-unused-label` が適切 [A] | CMake差分 | [C] |
| `/bigobj` に相当する設定 | Mach-O にはセクション数の上限がないので、相当するものは不要 [A] | CMake差分（不要） | [A] |
| `/MP` に相当する設定 | Ninja はファイル単位で並列にビルドする | CMake差分 | [A] |
| 1 ファイルの大きさ | 合計で約 40 MB、1 本あたり約 1.7 MB（CMake のコメントとエージェントの集計による。生成物は存在しないので実測していない） | UNKNOWN | [A] |
| コンパイル時間とメモリ | 巨大な switch / goto 関数と always_inline の組み合わせ。Clang の -O2 では、関数ごとの最適化（GVN、レジスタ割り当て）が非線形に重くなることがある。MSVC では約 12 分（生成を含む） | **UNKNOWN**（M3 で計測。必要なら `-O1`、`PARTS` を増やす、`-fno-inline-functions` などを検討） | [U] |
| LTO | ys_recomp は IPO OFF（260）。Clang でも同じ扱いになる | PORTABLE | [C] |
| Debug ビルド | always_inline は -O0 でも展開されるので、Debug ビルドも重い [A] | ― | [A] |
| PARTS の二重定義 | `codegen.py:28 PARTS = 24` と CMakeLists.txt:243 の `RANGE 0 23` | OS とは無関係（既存の問題） | [C] |

---

## 項目 11：`recomp.py generate` は macOS の Python で動くか（実行はせず、コードから判断）

| 項目 | 内容 | 判定 | 確度 |
|---|---|---|---|
| `tomllib` | config.py:29、discmap.py:30 | **3.11 以上が必要**。今の `.venv`（3.10.7）では動かない | [C] **[B]** |
| OS 固有のモジュール | `os.startfile`、`msvcrt`、`winreg`、`shell=True`、`sys.platform` の分岐はない | PORTABLE | [C] |
| multiprocessing | 使っていない（macOS の spawn 方式の問題は起きない）。`if __name__ == "__main__"` もある（recomp.py:333） | PORTABLE | [C] |
| パス | `os.path.join`、`os.makedirs` を使う（codegen.py:264、282、293） | PORTABLE | [C] |
| 文字コード | 生成コードは `encoding="utf-8"` で書く。`newline=` は指定しないので、改行は Windows では CRLF、macOS では LF | PORTABLE（改行だけが違う） | [C] |
| 出力の決定性 | 書き出す前にすべて並べ替えている（codegen.py:79-81、272、296、308） | PORTABLE。OS が違っても、改行以外は同じ内容になる見込み [A] | [C] |
| 入力 | ディスクを `"rb"` で読む。CUE は latin-1（項目 3） | PORTABLE（ASCII の名前なら） | [C] |
| CMake からの起動 | `${PYTHON_EXE}`（CMakeLists.txt:250-255） | 3.11 以上を明示して渡す必要がある | [C] |

**判定**：Python 3.11 以上で動かせば、macOS でも動く見込みが高い [A]。実際に動くかは M3 で確かめる。

---

## 項目 12：vblank.c と gpu_time.c の macOS での置き換え先

| 機能 | Windows の実装 | macOS の候補 [A] | 必要性 | 分類 |
|---|---|---|---|---|
| 次のリフレッシュ時刻と周期（`vblank_query`） | `DwmGetCompositionTimingInfo` の `qpcVBlank` と `rateRefresh` | `CVDisplayLink`（macOS 15 で非推奨 [A]）、`CAMetalDisplayLink`（macOS 14 以降 [A]）、または `NSScreen.maximumRefreshInterval` / `displayLinkWithTarget` の時刻。どれを使う場合も、時刻を `SDL_GetPerformanceCounter` の単位（mach_absolute_time）に揃える | 任意。0 を返せば、`SDL_GetCurrentDisplayMode` のリフレッシュレートと、前の画像の時刻を基準にする経路になる（main.c:1117-1120、pacing.c:57）。滑らかさの質に関わる [A] | macOS hostが必要（`.m`、host 層のみ） |
| 優先度（`vblank_raise_priority`） | `SetPriorityClass` | `setpriority` または `pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE)`。スレッドの優先度は SDL が設定済み | 任意 | macOS hostが必要（C で書ける） |
| GPU 時間（`gpu_time_*`） | D3D11 のタイムスタンプ問い合わせと DXGI のフレーム統計 | `SDL_PROP_RENDERER_METAL_*` から取る `MTLCommandBuffer` の `GPUStartTime` / `GPUEndTime`、または `MTLDrawable.presentedTime`。ただし SDL_Renderer がコマンドバッファを外に出すかは [U] | 任意（`--frame-timing` という開発用機能） | macOS hostが必要（任意）／ UNKNOWN |

- **M1〜M3 の最低限**：いまの非 Windows 用スタブのまま使う。動作は保たれ、測定の機能だけが無効になる。
  - 作業規則 v2 の 10 に従い、機能が無効であることをログと文書に残す。
- **実装は M3 以降に任意で行う。**

---

## 項目 13：ソースの列挙に `._*` が混ざる可能性

| 箇所 | 方式 | 影響 | 分類 | 確度 |
|---|---|---|---|---|
| `PCERecomp/tools/recomp/recomp.py:68` | `os.listdir` を `.txt` で絞る | `--check games/ys/coverage` で `._*.txt` を読み、UnicodeDecodeError（coverage.py:62） | PCERecomp側の修正が必要 | [C] |
| `CMakeLists.txt:249` | `file(GLOB … *.py)` | 依存の対象が増えるだけ | PORTABLE | [A] |
| `tools/code_same.py:49-51`、`tools/py_same.py:49-53` | `os.walk` | `._*` を UTF-8 で読み、デコードエラー（手動のツール） | 標準C/C++で吸収可能（実際は Python の修正） | [C] |
| `tools/package/run_package_test.py:38, 55, 89` | `listdir` の結果を完全一致で比べる | `.DS_Store` があると FAILED | macOS hostが必要（除外の規則） | [C] |
| `tools/app/run_menu_under_test.py:48` | `listdir` → `int(n[1:7])` | `._q…bmp` があると ValueError | 同上 | [C] |
| `tools/app/run_menu_chip_test.py:65` | `listdir` を `.txt` で絞る | `._*.txt` を誤って拾う | 同上 | [C] |
| `runtime/app/disc_id.c:165` | `SDL_GlobDirectory(folder, "*.cue")` | `._x.cue` を拾うかは SDL の実装しだい。拾った場合は解析に失敗し、順位の低い候補になる見込み | UNKNOWN | [U] |
| Python の `glob.glob("*.x")` | run_bios_check.py:46、run_perf_check.py:91、check_all_sessions.py:105 など | ドットで始まる名前は返さない | PORTABLE | [C] |
| CMake のソースの列挙 | 本体も PCERecomp もソースを明示的に並べる（GLOB は 249 だけ） | ― | PORTABLE | [C] |
| Git | `._*` と `.DS_Store` は `.git/info/exclude` だけで除外している。共有される `.gitignore` にはない | 共有の除外設定を追加するかはユーザー判断（03 の D8） | [C] |

- APFS では通常 `._*` は作られない [A]。exFAT、SMB、zip の展開、Windows VM との共有フォルダで作られる。
- **Windows VM と共有フォルダでソースを受け渡す場合は特に注意。**
