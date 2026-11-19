# M1：host 層の骨格、cmake/platform_*.cmake、scripts/build.sh、ツールの .exe 固定の除去

- 作成日：2026-11-19。基準：`06b62c6`（Phase 2 完了）。PCERecomp（`external/PCERecomp`、`1b9d25a`）は変更していない。
- 表記：[C] 確認済み（macOS で実行して確かめた）、[W] Windows VM での確認待ち、[A] 推測。
- **Windows ではビルドも試験もしていない。** Windows の動作が変わらないことは、5 章の手順で VM で確かめる必要がある。

---

## 1. M1 の前に決めたこと（2026-11-19、ユーザー）

| ID | 決定 |
|---|---|
| H1 | 関数名は**今の名前を残す**（`vblank_query`、`vblank_raise_priority`、`gpu_time_*`）。ヘッダーだけを `runtime/host/` に移す |
| D6 | SDL3 の URL を公式リリースのファイルに替え、**GitHub のリリース API が公開する sha256 と照合して**から固定する |
| M1 の macOS 確認 | 構成＋ `pce_app`・`app_tests` のビルド＋ ROM 不要の quick 試験 |
| 層の試験 | M1 で quick ラベルに追加する（`host_layers`） |
| （AI の判断、質問の前に提示） | `scripts/build.sh` は Ninja Multi-Config（実行ファイルが `build/Release/` に出る）。`GG_TRACE` の `.exe` 固定は M2（D5）。`run_package_test.py` のファイル一覧の検査は厳しいまま残す。`host_paths` の macOS の既定値（`SDL_GetPrefPath`）は M3 |

## 2. 変更の内容

### 2.1 host 層（`runtime/host/`）

| ファイル | 中身 | 元 |
|---|---|---|
| `host_display_timing.h` | `vblank_query()` | `runtime/app/vblank.h` |
| `host_process.h` | `vblank_raise_priority()` | `runtime/app/vblank.h` |
| `host_gpu_timing.h` | `gpu_time_*()` | `runtime/app/gpu_time.h`（移動、コメントのみ変更） |
| `host_paths.h` | `host_paths_init(app_id)`、`host_resource_dir()`、`host_user_dir()`（05 の 3.1 のとおり） | 新規 |
| `win/display_timing_dwm.c` | DWM の vblank | `vblank.c` の `_WIN32` 側（本体は同じ） |
| `win/process_win.c` | `SetPriorityClass` | `vblank.c` の `_WIN32` 側（本体は同じ） |
| `win/gpu_timing_d3d11.c` | D3D11 / DXGI | `gpu_time.c` の `_WIN32` 側（本体は同じ。`#ifdef` の 3 行と include の 1 行だけ変更） |
| `win/ysrecomp.rc`、`win/ysrecomp.ico` | Windows のリソース | `runtime/app/` から移動（中身は同じ。同じフォルダに並ぶので `.rc` の相対パスもそのまま） |
| `null/display_timing_null.c`、`null/process_null.c`、`null/gpu_timing_null.c` | 実装なし | 各 `#else` 側。**無効であることを stderr に 1 回出す**（作業規則 v2 の 10、R11・R12）：`host: display refresh timing not available on this platform ...`、`host: GPU time measurement not available ...`（後者は `--frame-timing` のときだけ） |
| `common/host_paths_sdl.c` | どちらも `SDL_GetBasePath()`（取得できなければ空文字。今の `exe ? exe : ""` と同じ） | 新規 |

- `runtime/main.c`：include の差し替え、`SDL_GetBasePath()` の 2 箇所（ランチャーの画像 → `host_resource_dir()`、ini → `host_user_dir()`）、引数の解析の直後に `host_paths_init("YsRecomp")` を 1 行。`app_id` は Phase 3B で profile から渡す。
- `runtime/app/volume.c`：`#include <stddef.h>` を追加。macOS（Apple Clang）で `NULL` が未定義でコンパイルできなかった（MSVC では別のヘッダー経由で定義される）。macOS で `pce_app` をビルドした結果、コンパイルエラーはこの 1 件だけだった [C]。Windows の動作には影響しない。

### 2.2 CMake

| 変更 | 内容 |
|---|---|
| `cmake/platform_windows.cmake` | `if(MSVC)` のフラグ（`/W4 /utf-8`、`/Zi`、`/DEBUG /OPT:REF /OPT:ICF`、`/MT`、`enable_language(RC)`）を**そのまま**移動。`PLATFORM_HOST_SOURCES`（win の 3 本）、`PLATFORM_HOST_LIBRARIES`（`dwmapi`）、`platform_generated_code()`（`/W1 /bigobj /MP`）、`platform_app_executable()`（`.rc`） |
| `cmake/platform_macos.cmake` | `CMAKE_OSX_ARCHITECTURES=arm64`（D9）、`-ffp-contract=off`（D4）、null の 3 本。生成 C のフラグと `.app` は M3（関数は空） |
| `CMakeLists.txt` | `if(WIN32)` / `elseif(APPLE)` で上のどちらかを include（場所は今の `if(MSVC)` と同じ。PCERecomp・SDL3 より前）。それ以外の OS は構成エラー。単一構成の生成器では `CMAKE_BUILD_TYPE=Release` を既定に。`PYTHON_EXE` が 3.11 未満なら構成エラー。SDL3 の URL と `URL_HASH`。試験 `host_layers` |

- SDL3：`https://github.com/libsdl-org/SDL/releases/download/release-3.2.30/SDL3-3.2.30.tar.gz`、`SHA256=4c3b09330d866dc52eb65b66259a6684ad387252ca8c57901b3a2b534eb42e3d`。値は GitHub のリリース API（`/repos/libsdl-org/SDL/releases/tags/release-3.2.30` の asset の `digest`）から取り、macOS の FetchContent が取得したファイルの `shasum -a 256` と一致することを確かめた [C]。展開したソースに `src/video/stb_image.h`（ランチャーが使う）があることも確かめた [C]。
  - 以前の URL は GitHub がタグから自動で作る tar.gz で、公式のリリースファイルとはファイルが違う。中身のソースは同じ版（3.2.30）[A]。Windows のビルド結果が同じかは [W]。

### 2.3 Python ツール

| ファイル | 変更 |
|---|---|
| `tools/build_paths.py`（新規） | `exe(name)` → `build/Release/<name>`＋（Windows のときだけ）`.exe` |
| `tools/trace/check_session.py`、`run_bios_check.py`、`run_perf_check.py`、`check_all_sessions.py` | 既定の実行ファイルを `build_paths.exe()` で決める。`check_all_sessions.py` の「ゲームが開いているか」は、Windows は今と同じ `tasklist`、それ以外は `pgrep -x`（判断できないときは今と同じく「開いている」とみなす） |
| `tools/py_same.py`、`tools/code_same.py`、`tools/app/run_menu_chip_test.py`、`run_menu_under_test.py` | 列挙から `._*`（と `code_same.py` は `.DS_Store`）を除く。`glob.glob("*…")` はもともとドットで始まる名前を拾わないので変更していない |
| `tools/package/make_icon.py`、`make_package.py` | `.ico` の場所を `runtime/host/win/` に |
| `tools/check_layers.py`（新規） | 05 の 7 章の 5 つの規則。試験 `host_layers`（quick）。違反を 1 件ずつ入れたコピーで、5 つの規則すべてが検出されることを確かめた [C] |

- 期待値（`PASS_REGULAR_EXPRESSION`、セッション、カバレッジ）は変更していない。skip も追加していない。
- `tools/pcerecomp_dir.py` と CMake の `GG_TRACE`（`gg_trace.exe`）は M2（D5）。`run_package_test.py`・`make_package.py` の `YsRecomp.exe` は Windows のパッケージ専用なので、macOS のパッケージ（M3）で扱う。

### 2.4 `scripts/build.sh`

build.ps1 と同じ手順（構成がなければ構成 → Release のビルド → 試験 → パッケージ）。オプション：`--test`、`--test-all`、`--test-hist`、`--package`（macOS では M3 まで「まだ使えない」と出して終了コード 1）、`--verbose`、`--target NAME`（M1 の確認用に追加）。`.venv/bin/python` があればそれを `PYTHON_EXE` に渡す。

### 2.5 文書

- `CLAUDE.md`：上流の本文の `runtime/app/vblank.c` を新しい場所に。作業規則 v2 の「ビルドと試験 → macOS」を記入。
- `docs/technical_notes.md`：`vblank.c` の場所を 1 箇所。
- 過去の調査文書（`review_portability/00〜05`、`review_jp/00〜03`）の古いパスは、その時点の記録なので書き換えていない。

## 3. macOS で実行したこと（結果）

| コマンド | 結果 |
|---|---|
| `scripts/build.sh --target pce_app --target app_tests` | 構成は成功（SDL3 を取得・ハッシュ照合）。初回は `volume.c` の `NULL` でコンパイルエラー → 2.1 の修正後に成功 [C]。`app_tests` は `Mach-O 64-bit executable arm64` [C] |
| `ctest --test-dir build -C Release -R '^(app_config\|app_volume\|app_menu\|app_video\|app_image\|app_crop\|app_redraw\|app_fade\|app_launcher\|app_input\|app_disc_id\|app_cd_menu_music\|host_layers)$'` | **11 件合格、2 件 skip**（`app_disc_id`、`app_cd_menu_music`：北米版の cue が上流の固定のパス `roms/` 直下にないため。M2 で解消。skip は合格ではない） |
| `python tools/check_layers.py --pcerecomp external/PCERecomp` | `check_layers: OK` |
| 古い Python（3.10.7）を `-DPYTHON_EXE` に渡して構成 | 「Python 3.11 or later needed」で構成エラー（意図どおり）[C] |
| `python -m py_compile`（変更した Python 12 本）、各ツールの `--help` | すべて成功 |
| `bash -n scripts/build.sh`、`--help`、不明なオプション | 成功、終了コード 2 |

- **macOS で確かめていないもの**：`runtime/main.c` のコンパイル（`pce_runtime`、`YsRecomp`）、`pce_trace`、生成 C、ROM が必要な試験すべて。これらは M2・M3。
- 既存の警告（今回の変更とは無関係）：`runtime/app/menu.c:448` の `-Wswitch`（`MENU_KEY_LEFT`/`RIGHT`）。SDL3 の非推奨 API の警告多数。M3 で扱う。

## 4. 未解決

| ID | 内容 | 段階 |
|---|---|---|
| M1-1 | Windows で、M1 の前後で動作が同じこと（5 章）。**2026-11-20、Windows 11 x64 の実機で確認済み**（C1〜C11 すべて合格）。結果：`07a_plan_a_addendum.md` の 3 章 | 実機（済） |
| M1-6 | `recomp_lockstep_ys` が全試験（`-j 2`）で 1 回目に失敗し 2 回目に合格することがある。照合は `LOCKSTEP OK` で、終了コードが 0 でない。M1 の前（`06b62c6`）でも起き、単独では 25 回すべて合格（M1 が原因ではない） | 原因調査は未定（07a の 3.3） |
| M1-2 | `host_paths` の macOS の方針（`SDL_GetPrefPath`、`<org>` の値 = H2） | M3 |
| M1-3 | macOS の host 実装（CAMetalDisplayLink、QoS）は null のまま | M3 以降（任意） |
| M1-4 | macOS のパッケージ（`.app`、`Info.plist.in`、`.icns`）、生成 C の警告フラグ | M3 |
| M1-5 | `.rc` を版ごとのテンプレート（`app.rc.in`）にする | Phase 3B（05 の 5 章） |

## 5. Windows VM での確認手順

- VM の作り方（Windows 11 ARM 版、画面イメージ付き）、M1 の前（`06b62c6`）との比べ方、チェックポイント C1〜C10、この確認に戻るときのプロンプト：`07_plan_a_windows_arm_vm.pdf`（A 案）。B 案を取り込んだ後の差し替え（フォークからの clone、確かめる対象 `e9c4f6e`、SDL3 のハッシュの探し方、C11、新しいプロンプト）：`07a_plan_a_addendum.md`。
- VM の用意ができるまでは、GitHub Actions の Windows で ROM 不要の範囲を確かめる（B 案、ブランチ `ci/windows-github-actions`、`08_plan_b_github_actions.md`）。

前提：これまでと同じ（VS 2022 Build Tools、Python 3.14、`roms/` 直下に上流の固定名の北米版 cue・bin と System Card 2.0）。SDL3 の URL が変わったので、**`build\` を消してから**行う（FetchContent が新しい URL から取り直す。生成 C は `games\ys\generated\` にあり、`disc.toml` と生成器が変わっていないので作り直されない見込み [A]）。

```powershell
git submodule update --init --recursive
Remove-Item -Recurse -Force build
.\build.ps1 -Test
```
期待：構成とビルドが成功。quick の試験が**すべて合格**し、件数は M1 の前より 1 件多い（`host_layers` が増えた）。

```powershell
Get-FileHash build\_deps\sdl3-subbuild\sdl3-populate-prefix\src\SDL3-3.2.30.tar.gz -Algorithm SHA256
```
期待：`4C3B09330D866DC52EB65B66259A6684AD387252CA8C57901B3A2B534EB42E3D`。

```powershell
.\build.ps1 -Package
```
期待：`app_package_ys`、`app_launcher_ys` が合格し、`dist\YsRecomp\` と `dist\YsRecomp.zip` ができる（`make_package.py` がアイコンの埋め込みを確かめる：新しい場所の `.ico` が `.exe` に入っていること）。エクスプローラーで `dist\YsRecomp\YsRecomp.exe` のアイコンが今までと同じ絵であること。

```powershell
$env:PCE_HIDDEN_WINDOW=1; $env:SDL_AUDIO_DRIVER="dummy"
build\Release\YsRecomp.exe --frame-timing --frames 600 "roms\Ys Book I & II (USA).cue" | Select-String "GPU \(Direct3D 11"
```
期待：`GPU (Direct3D 11 timestamps): ... images, average ...` の行が出る（D3D11 の実装が使われている）。`host: ... not available` の行は**出ない**（null の実装が Windows に入っていない）。

```powershell
.\build.ps1 -TestAll
```
期待：M1 の前と同じ結果（すべて合格。件数は `host_layers` の分だけ 1 件多い）。Python ツールを変えたので、`-TestAll` まで行う。

```powershell
python tools\trace\check_all_sessions.py --help
python tools\trace\run_bios_check.py --routines E009
```
期待：前者は使い方が出る。後者は今までと同じ結果（`build\Release\bios_check.exe` が見つかる）。

1 つでも違えば、出力の該当部分（`Select-String` で絞ったもの）を知らせてほしい。
