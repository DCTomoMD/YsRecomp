# Phase 1-05：host 層（OS：Windows / macOS）の設計

- 作成日：2026-11-19。方法：ソースの読み取りと grep のみ。ビルド・試験・ダウンロードはしていない。
- **本書は設計である。** 製品コード・CMake・試験・サブモジュールは変更していない。
- 出発点：`02_portability_architecture.md`（M0 の案）と `03_macos_risk_register.md` 4.1 節（ユーザーの決定 D1〜D12）。本書はその案を Phase 1 の決定（`review_jp/01_profile_design.md` 1 章の P1〜P8）と合わせて確定させ、M0 の棚卸しの漏れを補う。
- 表記：[C] 確認済み、[A] 推測、[U] 不明。行番号は `b56201d` 時点。

---

## 1. 確定する方針

1. **用途ごとに小さなヘッダーを作る**（大きな HostServices は作らない）。M0-02 の 1 章のとおり。
2. **OS の選択は CMake のソースの選択で行う。** `runtime/main.c`、`runtime/app/`、`runtime/profile/` に `#ifdef _WIN32` / `__APPLE__` を新しく書かない。既存の分岐（`vblank.c`、`gpu_time.c`）は host へ移したうえで OS ごとのファイルに分ける。
3. **host は版を知らない。** 版によって変わる値（アプリの名前、フォルダ名）が host に要るときは、main.c が**素の文字列として引数で渡す**（3 章）。host は `runtime/profile/` を include しない。
4. **SDL3 で済むものは SDL3 を使う。** host にするのは SDL3 が提供しないものと、OS ごとに方針が違うものだけ。
5. **PCERecomp は変更しない。** 既存の `#ifdef`（`u8file.c`、`cdimage.c`、`huc6280_exec.h`、`profiler.c`）はそのまま使う。
6. **Objective-C は macOS host の `.m` に限る。** `.mm` は使わない見込み [A]（作業規則 v2 の 9）。
7. **Windows の動作を変えない。** M1 の変更はファイルの移動と CMake の整理にとどめる（Windows VM で確認が必要）。

---

## 2. host の API（確定案）

| ヘッダー | 関数 | Windows | macOS | null（実装なし） | 今の場所 [C] |
|---|---|---|---|---|---|
| `host/host_display_timing.h` | `int host_vblank_query(uint64_t *next, uint64_t *period)`（単位は `SDL_GetPerformanceCounter`） | DWM | M3 までは null。以後に任意で CAMetalDisplayLink（`.m`、macOS 14 以降 [A]） | 0 を返す。滑らかな動きの pacing は推定で続く（R11） | `runtime/app/vblank.c:6-47` |
| `host/host_process.h` | `void host_raise_process_priority(void)` | `SetPriorityClass` | `setpriority` か QoS（任意） | 何もしない | `runtime/app/vblank.c`、呼び出しは `main.c:2880` |
| `host/host_gpu_timing.h` | `gpu_time_init/mark/presented/report/quit`（今の名前のまま） | D3D11 / DXGI | null（Metal 版は任意 [U]） | init は 0、ほかは何もしない。`--frame-timing` に「gpu 計測なし」と出す（R12） | `runtime/app/gpu_time.c:7-187` |
| `host/host_paths.h` | 3 章 | 3 章 | 3 章 | ― | `main.c:1947, 2553-2556` |

- 関数名は案。M1 で決めるとき、変更量を減らすために今の名前（`vblank_query` など）を残す選択肢もある。
- 新しく作る host API は上の 4 つにとどめる。プロファイラ（`PCERecomp/tools/trace/profiler.c`）は PCERecomp の既存のものをそのまま使い、macOS では今のスタブ（`--profile` は無効）のままにする。

---

## 3. host_paths と版の受け渡し

### 3.1 API（案）

```c
/* host/host_paths.h — plain C, no OS headers, no profile headers. */

/* Called once by main.c before any other host_paths function.
 * app_id: a short ASCII folder-safe name chosen by the caller (main.c passes the edition's app id). */
int host_paths_init(const char *app_id);

/* Read-only resources (launcher images, ...). Ends with a separator. */
const char *host_resource_dir(void);

/* Writable user data (ini, saves/, sessions/, menu_font.bin, launcher_sfx.bin). Ends with a separator. */
const char *host_user_dir(void);
```

| OS | `host_resource_dir()` | `host_user_dir()` |
|---|---|---|
| Windows | `SDL_GetBasePath()`（今と同じ） | `SDL_GetBasePath()`（今と同じ：実行ファイルの隣。`app_id` は使わない） |
| macOS（D3） | `.app` の `Contents/Resources`（`SDL_GetBasePath()` が返す場所 [A]、M3 で確認） | `SDL_GetPrefPath(<org>, app_id)`：`~/Library/Application Support/<app_id>/` [A] |

- **版の情報の流れ**：`main.c` が `g_ys_profile.identity->app_id`（例：`"YsRecomp"` / `"YsIandII_JP"`。日本版の名前はユーザーの決定 P9（`review_jp/01` 1 章））を `host_paths_init()` に渡すだけ。host の中では版を区別しない。北米版と日本版は `app_id` が違うので、macOS の書き込み先は自然に分かれる。Windows は実行ファイルが別のフォルダにあれば分かれる（P4）。
- `<org>` の値（SDL の組織名）は M3 で決める（空文字にするか、作者名にするか）。

### 3.2 置き換える書き込み先 [C]

| 書き込むもの | 今の場所 | 置き換え |
|---|---|---|
| ini | `main.c:2553-2556`（`SDL_GetBasePath()` の隣） | `host_user_dir()` |
| `saves/`、`sessions/`、`menu_font.bin`、`launcher_sfx.bin` | ini のあるフォルダ（`main.c:245, 2592, 2596, 2019, 2761`） | 変えない（ini のフォルダを基準にしているので、ini の場所が決まれば従う） |
| ランチャーの画像 | `main.c:1942-1958`（実行ファイルの隣の `launcher/`、なければ `PCE_DOCS_DIR`） | `host_resource_dir()`。配布物では `PCE_DOCS_DIR`（ソースの絶対パス、R23）に頼らない |
| 開発用の出力（`lockstep_divergence.txt`、`--save` の既定 `saves/`、`--coverage`、`--record`） | 作業フォルダからの相対パス（`main.c:803, 2609-2619`）、引数のパス（`main.c:473, 3609, 3628`） | **変えない**（開発用で、作業フォルダに書くのが意図 [A]）。M0 の棚卸しに記載がなかったので、本書で記録する |

---

## 4. M0 の棚卸し（`01_windows_dependencies.md`）に漏れていたもの

Phase 1 の調査で見つかった。どれも Windows の動作には影響しない。

| 項目 | 場所 [C] | 分類（`00` の 1.2） | 扱い |
|---|---|---|---|
| `lockstep_divergence.txt` を作業フォルダに書く | `main.c:803` | PORTABLE（開発用） | 3.2 のとおり変えない |
| `--coverage`、`--record` の出力先と `SDL_CreateDirectory` | `main.c:473, 3609, 3628` | PORTABLE | 変えない |
| 分岐時のウィンドウのタイトルが直書き（`"pce_recomp - DIVERGENCE ..."`） | `main.c:3354` | PORTABLE（版の問題に近い） | 版の名前は profile の `identity` から取る（`review_jp/01` 7 章）。host には関係しない |
| 全画面の切り替えが Alt+Enter（`SDL_KMOD_ALT`） | `main.c:2054`（ランチャー）、`main.c:2954` | macOS hostが必要（調整） | macOS の慣例（Cmd+Ctrl+F）を足すかは M3 で決める。キーの割り当ての表を host に置く案と、両方を常に受け付ける案がある。後者なら host は不要 |
| `SDL_HINT_FORCE_RAISEWINDOW` | `main.c:1189` | PORTABLE [A] | M3 で macOS の挙動を確かめる |
| フォルダ選択の待ちループ（`SDL_PumpEvents` + `SDL_Delay` で結果を待つ） | `main.c:187-190` | SDL3で吸収可能 | メインスレッドで呼んでいるので macOS でも動く見込み [A]。日本版の BIOS 選択（`review_jp/01` 6.2）も同じ形にする |
| `.rc` の製品名・ファイル名・言語（0x409） | `runtime/app/ysrecomp.rc:5, 17-27` | Windows hostへ隔離 ＋ 版 | 5 章 |

---

## 5. OS と版が交わるもの

C のコードの中で OS と版を交わらせないため、交わるものは**ビルド時に CMake が組み立てる**。

| もの | OS の軸 | 版の軸 | 組み立て方（案） |
|---|---|---|---|
| Windows のリソース（`.rc`） | `runtime/host/win/app.rc.in`（テンプレート） | 製品名、ファイル名、アイコンのパス | `configure_file` に版の変数（`YS_EDITION_EXE`、`YS_EDITION_PRODUCT` など）を渡し、`build/<edition>/app.rc` を作る |
| macOS の `Info.plist` | `runtime/host/mac/Info.plist.in` | バンドル名、識別子（例 `<org>.<app_id>`）、アイコン | 同じく `configure_file`（M3） |
| アイコン | `.ico`（Windows）、`.icns`（macOS） | 版ごとの絵柄を分けるかは未定 | `tools/package/make_icon.py` が両方の形式を作る（M3）。ROM 由来のデータは使わない |
| パッケージ | zip（Windows）、`.app` を入れた zip か DMG（macOS、D10・D12） | 名前、README、ini 名 | `make_package.py --edition <id> --platform <os>`（`review_jp/02_build_layout.md` 5 章） |
| 書き込み先 | 3 章の表 | `app_id` | 実行時に main.c が文字列を渡す（3.1） |

- 版の変数は `ys_edition()`（`review_jp/02_build_layout.md` 2 章）の引数から来る。OS の変数は `cmake/platform_<os>.cmake` から来る。**両方を参照するのはテンプレートの展開だけ**。

---

## 6. ディレクトリ構成（確定案）

```
runtime/
  main.c                         common (no new #ifdef _WIN32 / __APPLE__)
  app/                           common: PC-side enhancements and ys_* hooks (read the profile)
  profile/                       edition data (review_jp/01). No OS, no SDL, no host.
  host/
    host_display_timing.h        interfaces only
    host_gpu_timing.h
    host_process.h
    host_paths.h
    common/host_paths_sdl.c      SDL-based; the OS policy comes from a compile definition set by cmake/platform_*.cmake
    win/display_timing_dwm.c     moved from vblank.c (_WIN32 side)
    win/process_win.c
    win/gpu_timing_d3d11.c       moved from gpu_time.c (_WIN32 side)
    win/app.rc.in, *.ico         template (configure_file per edition)
    mac/Info.plist.in, *.icns    M3
    mac/display_timing_*.m       optional, M3 or later
    null/display_timing_null.c
    null/gpu_timing_null.c
    null/process_null.c
cmake/
  platform_windows.cmake         MSVC flags, /MT, RC, dwmapi, generated-code /W1 /bigobj /MP
  platform_macos.cmake           Apple Clang, -ffp-contract=off (D4), arm64 only (D9), MACOSX_BUNDLE (M3)
  ys_edition.cmake               edition function (review_jp/02)
scripts/
  build.sh                       macOS (D10)
```

---

## 7. 依存の規則と機械的な確認（確定案）

| 禁止する依存 | 確認（M1 / Phase 3A で quick の試験にする候補） |
|---|---|
| common（`main.c`、`app/`、`profile/`）→ OS のヘッダー、`_WIN32` / `__APPLE__` の新設 | `grep -E '#include <(windows|dwmapi|d3d11|dxgi|Cocoa/|Metal/)|_WIN32|__APPLE__'` が 0 件 |
| host → `app/`、`profile/`、`ys_`、Ys の定数 | `runtime/host/` に `grep -E 'app/|profile/|ys_|g_ys_profile'` が 0 件 |
| host の実装どうし（win ↔ mac ↔ null） | `runtime/host/win/` に `mac/`・`null/` の include がない（逆も同じ） |
| PCERecomp → host、app、profile、Ys | `grep -E '\bYs\b|ys_|app/|Adol|host/|profile/'` が 0 件 |
| host での、エミュレートされたコンソールの状態の変更 | `runtime/host/` が `pce/` のヘッダーを include しない |

- 既存の `vblank.c`・`gpu_time.c` を移すまでは、最初の確認は 2 件の例外を持つ。例外は試験の中に名前で書き、移した時点で消す（期待値を緩めるのではなく、移行の途中であることを明示する）。

---

## 8. 段階との対応

| 段階 | host 層の作業 |
|---|---|
| M1 | 2 章のヘッダーと win/null 実装（ファイルの移動だけ）、`host_paths`（Windows は今と同じ）、`cmake/platform_*.cmake`、`scripts/build.sh` |
| M2 | ROM・BIOS のパスの変数化、`GG_TRACE` の `.exe` の固定の除去（`review_jp/02` と一緒に `ys_edition()` を北米版だけで導入） |
| M3 | macOS のビルド、`host_paths` の macOS の既定値（`SDL_GetPrefPath`）、`Info.plist.in`、`.icns`、`.app`、Retina・Metal の確認、`-ffp-contract=off` |
| Phase 3B | 日本版の `app_id` を `host_paths_init()` に渡すだけ（host の変更はない見込み） |

## 9. 未解決（host 層）

| ID | 内容 | 決める段階 |
|---|---|---|
| H1 | 関数名を `host_*` にそろえるか、今の名前を残すか | M1 |
| H2 | `SDL_GetPrefPath` の `<org>` の値 | M3 |
| H3 | macOS の全画面のショートカット（Cmd+Ctrl+F を足すか） | M3 |
| H4 | 版ごとにアイコンの絵柄を分けるか | Phase 3B（ユーザー判断） |
| H5 | macOS の最低バージョン（D9：使うライブラリで決める） | M3 |
