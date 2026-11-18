# M0-02：移植のアーキテクチャ（common と host の境界）

表記は `00_platform_inventory.md` の 1 章に従う。本書は**設計案**である。実装は M1 以降に、ユーザーの承認を得てから行う。

---

## 1. 方針

1. **万能の HostServices（1 つの大きな関数表）は作らない。**
   - 用途ごとに、小さなヘッダーを 1 つずつ作る。
   - 今の OS 依存は少なく、それぞれ独立している（`01` の項目 1・12）。1 つにまとめると、関係のない機能まで結びついてしまう。
2. **OS の選択は CMake が「どのソースをコンパイルするか」で行う。**
   - `common` の C ファイルには `#ifdef _WIN32` / `__APPLE__` を新しく書かない。
   - 既存の分岐（vblank.c、gpu_time.c）は host 層へ移したうえで、OS ごとのファイルに分ける。
3. **ゲーム版の差（USA / JP）は profile に置き、OS の差（Windows / macOS）は host に置く。**
   - 両者を同じ条件分岐や同じファイルに混ぜない（作業規則 v2 の 8）。
   - profile は host を知らない。host は profile を知らない。
4. **SDL3 で済むものは SDL3 を使い、host にしない。**
   - 例：ダイアログ、時間、スレッド、入力、音声、パスの取得。
   - host にするのは、SDL3 が提供しないものと、OS ごとに方針が違うもの（書き込み先）だけ。
5. **PCERecomp は変更しない**（M1 の範囲では不要の見込み。`01` の 9.2）。
   - PCERecomp の中の既存の `#ifdef`（u8file、cdimage、huc6280_exec、profiler）はそのまま使う。
6. **ゲームコアは C のままにする。**
   - Objective-C は、macOS host の `.m` ファイルの必要な箇所に限る（作業規則 v2 の 9）。
   - `.mm`（Objective-C++）は使わない見込み [A]。
7. **Windows の動作を変えない。**
   - M1 の変更は、Windows ではファイルの移動と CMake の整理にとどめる。
   - 生成されるバイナリの動作は今と同じにする（Windows VM で確認が必要）。

---

## 2. 用途別の小さな host API（案）

| ヘッダー（案） | 関数（案） | Windows | macOS | 実装がない場合（null） | 今のファイル |
|---|---|---|---|---|---|
| `host/host_display_timing.h` | `int host_vblank_query(uint64_t *next_counter, uint64_t *period_counter)`：単位は `SDL_GetPerformanceCounter` | DWM（今の vblank.c の `_WIN32` 側） | M1〜M3 は null。M3 以降に任意で CAMetalDisplayLink など（`.m`） | 0 を返す | `runtime/app/vblank.c` |
| `host/host_process.h` | `void host_raise_process_priority(void)` | `SetPriorityClass` | `setpriority` または QoS（C で書ける）。任意 | 何もしない | `runtime/app/vblank.c` |
| `host/host_gpu_timing.h` | `gpu_time_init/mark/presented/report/quit`（今の名前のまま） | D3D11 / DXGI（今の gpu_time.c の `_WIN32` 側） | null（Metal 版は任意、[U]） | init は 0、ほかは何もしない | `runtime/app/gpu_time.c` |
| `host/host_paths.h` | `const char *host_resource_dir(void)`（読み込み専用：launcher 画像など）、`const char *host_user_dir(void)`（書き込み：ini、saves、sessions、*.bin） | どちらも `SDL_GetBasePath()`（今と同じ） | resource は `SDL_GetBasePath()`。user は D3 の決定しだい（ポータブルなら実行ファイルの隣、.app なら `SDL_GetPrefPath`） | ― | `main.c:1947, 2553-2556` |
| （任意）`host/host_profiler.h` | 新しく作らない。PCERecomp の `profiler_start/stop` をそのまま使う | DbgHelp | スタブ（`--profile` は無効と表示する） | ― | `PCERecomp/tools/trace/profiler.c` |

- 関数名は案。M1 で決める際、変更の量を減らすため、今の名前（`vblank_query` など）を残す選択肢もある。
- `host_paths` の実装は SDL3 だけで書ける。ただし既定の方針は OS ごとに違うので、host に置く。
- 新しく作る host API はこの 4 つ（＋任意 1 つ）にとどめる。これ以外は SDL3 か標準 C で済む（`01` の項目 2）。

---

## 3. ディレクトリ構成（案）

```
runtime/
  main.c                     common (no new #ifdef _WIN32 / __APPLE__ here)
  app/                       common: PC-side enhancements (menu, crop, motion, fade, equip, ys_* hooks, ...)
  host/
    host_display_timing.h    interfaces only (plain C, no OS headers)
    host_gpu_timing.h
    host_process.h
    host_paths.h
    common/host_paths_sdl.c  SDL-based implementation; the policy comes from a compile definition (CMake)
    win/display_timing_dwm.c       (moved from today's vblank.c, _WIN32 side)
    win/process_win.c
    win/gpu_timing_d3d11.c         (moved from today's gpu_time.c, _WIN32 side)
    win/ysrecomp.rc, ysrecomp.ico  (moved from runtime/app/)
    mac/Info.plist.in, YsRecomp.icns (M3; icns generated from make_icon.py, no ROM-derived data)
    mac/display_timing_*.m         (optional, M3 or later)
    null/display_timing_null.c     (moved from today's #else side)
    null/gpu_timing_null.c
    null/process_null.c
  profile/                   edition differences (USA / JP). Phase 1 / 3. Separate from host.
cmake/
  platform_windows.cmake     MSVC flags, /MT, RC, dwmapi, generated-code /W1 /bigobj /MP
  platform_macos.cmake       Apple Clang flags, -Wno-unused-label for generated code, MACOSX_BUNDLE (M3)
scripts/
  build.sh                   macOS / POSIX counterpart of build.ps1 (or CMakePresets.json)
tools/                       Python: replace .exe and path fixed names with arguments or a suffix variable
```

- `runtime/app/` に今ある Ys 固有のフック（`ys_*.c`）は、版の差を profile へ移す作業（Phase 1 / 3）の対象。host の作業とは別に進める。
- `external/PCERecomp/` は触らない。

---

## 4. 依存の方向と、禁止する依存

```
            profile (USA/JP data and hook tables)
                 ^
                 |  (app reads the profile)
main.c / app  ---+--->  host/*.h (interfaces)  <---  host/{win,mac,null}/*  ---> OS / SDL3
     |                                                 (implements)
     +---> PCERecomp (pce_core, pce_input)  ---> standard C only (+ existing u8file/cdimage #ifdefs)
     +---> SDL3
```

| 禁止する依存 | 理由 |
|---|---|
| host → app、profile、`ys_*`、Ys の定数 | host は版を知らない。OS の差と版の差を混ぜない |
| profile → host、OS のヘッダー、SDL | 版の差は OS に依存しない |
| common（main.c、app/、profile/）→ `<windows.h>`、`<dwmapi.h>`、`<d3d11.h>`、Cocoa、Metal | OS のヘッダーは host の実装だけが include する |
| common の中の `#ifdef _WIN32` / `__APPLE__` の新設 | OS の選択は CMake のソース選択で行う |
| PCERecomp → host、app、Ys | 上流の規則（Ys のものを入れない）。host も YsRecomp 側の層 |
| host の実装どうしの相互参照（win ↔ mac） | 各 OS の実装は独立させる |
| `if (japanese)` の散在 | 作業規則 v2 の 8。版の差は profile のデータと関数表で表す |
| host での「エミュレートされたコンソール」の状態の変更 | 上流の規則（PC 側はコンソールの状態を変えない） |

**機械的な確認の案**（M1 で試験にする候補。今回は作らない）
- `runtime/main.c`、`runtime/app/`、`runtime/profile/` に対して `grep -E '#include <(windows|dwmapi|d3d11|dxgi)\.h>|_WIN32|__APPLE__'` を実行し、該当が 0 件であることを確かめる。
- `runtime/host/` に対して `grep -E 'ys_|app/|profile/'` を実行し、該当が 0 件であることを確かめる。

---

## 5. 版（USA/JP）と OS（Win/mac）の分離

| 軸 | 置き場所 | 選択の方法 | 例 |
|---|---|---|---|
| OS | `runtime/host/{win,mac,null}`、`cmake/platform_*.cmake` | CMake（ビルド時） | vblank、GPU 時間、優先度、書き込み先、パッケージ、警告フラグ |
| 版 | `runtime/profile/`、`games/ys/<edition>/disc.toml`、生成コード | 実行時（ディスクの識別結果）またはビルド時。Phase 1 で決める | フックのアドレス、RAM、ディスクの指紋、フォント、試験の期待値 |
| 両方に関わるもの | ― | 片方の軸だけで表せるよう、分けて設計する | 例：日本語ファイル名の NFC。正規化は OS の問題として host_paths / 共通の関数で扱い、版の分岐にはしない |

- **生成コードのシンボルの衝突（`recomp_lookup`）は版の問題であり、OS の問題ではない**（review_jp/00 の 17 章 1 項）。M1 では扱わない。

---

## 6. M1 の変更順序（案）

どの段階でも、Windows の動作を変えないことを優先する。各段階は小さくし、Windows VM での確認手順を付けて止まる。

| 順 | 内容 | 変更するファイル（案） | 確認 |
|---|---|---|---|
| 0 | 準備（ユーザー判断）：`.venv` を Python 3.11 以上で作り直す | `.venv`（Git 管理外） | `.venv/bin/python --version` |
| 1 | CMake を OS ごとに分ける。`if(MSVC)` / `if(WIN32)` の中身を `cmake/platform_windows.cmake` へ移すだけで、フラグは変えない。単一構成のときの既定の `CMAKE_BUILD_TYPE=Release`、`PYTHON_EXE` のバージョン確認（3.11 未満なら構成エラー）を追加する | `CMakeLists.txt`、`cmake/*.cmake` | Windows VM：`.\build.ps1 -Test` が今と同じ結果になること |
| 2 | vblank.c と gpu_time.c を `runtime/host/{win,null}` へ分ける（中身は移すだけ）。`.rc` と `.ico` を移す | `runtime/host/**`、`CMakeLists.txt` | Windows VM：`-Test` と `-Package`。`--frame-timing` の出力に `gpu` の行が出ること |
| 3 | `host_paths` を作る。Windows の既定は今と同じ（実行ファイルの隣）。main.c の `SDL_GetBasePath()` の 2 箇所を置き換える | `runtime/host/host_paths.h`、`common/host_paths_sdl.c`、`runtime/main.c` | Windows VM：`app_start_ys` と `app_package_ys`。ini、saves、sessions が今と同じ場所にできること |
| 4 | Python ツールの `.exe` と `build/Release` の固定をなくす。実行ファイルの接尾辞を引数か `sys.platform` で決め、`tasklist` は Windows のときだけ使う。`._*` / `.DS_Store` を列挙から除く（YsRecomp 側のツールのみ） | `tools/**/*.py` | Windows VM：`-TestAll`。macOS：`python -m py_compile` 程度（ビルドはできない） |
| 5 | `scripts/build.sh`（または `CMakePresets.json`）を追加する。中身は build.ps1 と同じ手順 | `scripts/build.sh` | macOS では M3 まで、構成の段階までしか確かめられない |
| ― | M2 に回すもの：ROM のパスをキャッシュ変数にする、`GG_TRACE` を変数にする、セッションのヘッダーの `\` パスの扱い | ― | ― |
| ― | M3 に回すもの：macOS での全ビルド、生成 C のコンパイル時間の計測、`-ffp-contract` の方針、Metal での描画、.app、パッケージ試験の macOS 版 | ― | ― |

- 段階 1〜3 は Windows の動作を変えない。そのため、北米版の既存の試験がそのまま回帰試験になる（Windows VM で確認が必要）。
- 段階 4 は試験のツールを変える。作業規則 v2 の 6（期待値を緩めない、skip を追加しない）に反しないよう、期待値には触れない。
