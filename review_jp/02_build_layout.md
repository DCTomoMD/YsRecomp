# Phase 1-02：版（USA / JP）と OS の境界 ── CMake・試験・ツール・パッケージの構成

- 作成日：2026-11-19。方法：ソースの読み取りと grep のみ。ビルド・試験・ダウンロードはしていない。
- **本書は設計である。** CMake・試験・ツールは変更していない。
- 関連：profile 層は `01_profile_design.md`、host 層は `review_portability/05_host_layer_design.md`。
- 表記：[C] 確認済み、[A] 推測、[U] 不明。行番号は `b56201d` 時点の `CMakeLists.txt` など。
- 日本版の値は書かない。日本版の cue・BIOS のファイル名も [U]（M2 でキャッシュ変数にするので、名前を決め打ちしない）。

---

## 1. 2 つの軸

| 軸 | 選ぶもの | 選ぶ場所 | 選ぶ時 |
|---|---|---|---|
| OS（Windows / macOS） | host の実装、コンパイラのフラグ、リソースの形式、パッケージの形式 | `cmake/platform_<os>.cmake` | 構成時（実行する機械で決まる） |
| 版（USA / JP） | profile、disc.toml、生成 C、セッション、カバレッジ、実行ファイル名、試験 | `cmake/ys_edition.cmake` の関数 `ys_edition()` | 構成時（ROM と BIOS のキャッシュ変数があれば、その版を定義する） |

- 2 つの軸が出会うのは、テンプレートの展開（`.rc`、`Info.plist`）とパッケージ作成だけ（`05` の 5 章）。

---

## 2. CMake の構成（案）

### 2.1 今の形 [C]

- 版に関わるものは、ほぼ 1 つのブロックにまとまっている：`if(PYTHON_EXE AND EXISTS "${YS_CUE}" AND EXISTS "${SYSCARD2}")`（`CMakeLists.txt:240-813`）。
  - 生成 C：`recomp.py generate`（241-255）→ `ys_recomp`（256-263）
  - 装備フレーム：`equip_frames.py`（268-276、北米版のセッション 2 本に依存）→ `equip_frames`（277-279）、`equip_check`（281）
  - `ys_trace`（286-291）、`YsRecomp`（295-302）
  - 試験：`YsRecomp` を使うもの 18 本、`ys_trace` を使うもの約 40 本、`pce_trace` を使うもの 4 本、`equip_check` 1 本
- ブロックの外にも北米版の試験がある：`trace_ys_*`、`session_ys_*`、`fade_ys`、`coverage_ys`、`nobios_ys`、`nobios_cabin_ys`、`bios_c_ys`、`hle_timing_ys`、`equip_preview_ys`、`equip_ice_preview_ys`、`recomp_disc_ys`（154-231。`if(PYTHON_EXE)` だけで囲まれ、ROM がなければ 77）。
- `app_disc_id`（136）と `app_cd_menu_music`（140）は北米版の cue のパスを直書きしている（`${YS_CUE}` ではない）。
- PCERecomp の `cd_image` 試験に `PCE_TEST_CUE "${YS_CUE}"` を渡している（41-42）。
- ラベル：`TESTS_QUICK` 824-826、`TESTS_CONSOLE` 827-829、`TESTS_RECOMP` 830-836、`TESTS_HISTORY` 837-838、`TESTS_PACKAGE` 839、`TESTS_TEMPO_REAL` 844-845、ラベル付けのループ 846-853、全試験の環境変数 854-857、`RUN_SERIAL` 861-865。

### 2.2 関数 `ys_edition()`（案）

```cmake
# cmake/ys_edition.cmake
ys_edition(
    ID        usa                                  # edition id (also the ctest label)
    CUE       "${YS_USA_CUE}"                      # M2 cache variable
    BIOS      "${YS_USA_BIOS}"                     # M2 cache variable (USA: used by generation and --bios tests only)
    GAME_DIR  "${CMAKE_SOURCE_DIR}/games/ys"       # disc.toml, sessions/, coverage/, generated/
    PROFILE   runtime/profile/profile_usa.c
    EXE       YsRecomp                             # player executable name
    TRACE     ys_trace                             # recompiled trace tool name
    PTRACE    pce_trace                            # interpreter trace tool name (with the Ys extension)
    EQUIP     generated                            # generated | none (equip_none.c)
    SUFFIX    _ys)                                 # test name suffix (USA keeps today's names)

ys_edition(
    ID jp  CUE "${YS_JP_CUE}"  BIOS "${YS_JP_BIOS}"
    GAME_DIR "${CMAKE_SOURCE_DIR}/games/ys_jp"
    PROFILE runtime/profile/profile_jp.c
    EXE YsIandII_JP  TRACE ys_trace_jp  PTRACE pce_trace_jp
    EQUIP none
    SUFFIX _ysjp)
```

- 関数の中身は、今の 240-813 行のブロック（と 154-231 行の版の試験）を、変数を引数に置き換えて移したもの。
- **CUE と BIOS の両方があるときだけ**、その版のターゲットと試験を定義する（今と同じ考え方）。片方の版の ROM がなくても、もう片方はビルドできる。
- **生成 C は版ごとの `GAME_DIR/generated/c`**。`recomp_lookup` / `recomp_rom_crc32`（`PCERecomp/runtime/cpu/huc6280_recomp.h:40-42`）は実行ファイルごとに 1 組なので、別ターゲットなら衝突しない [A]（D11）。PCERecomp の生成器は変えない。
- 生成のステップ（約 12 分）は版ごとに走る。片方の `disc.toml` を変えても、もう片方は作り直さない（`DEPENDS` が版ごとなので）[A]。
- 日本版の `disc.toml` は Phase 2 の結果で作る。**それまで `games/ys_jp/disc.toml` は存在しないので、日本版のターゲットは定義されない**（関数の条件に `EXISTS GAME_DIR/disc.toml` を加える案）。

### 2.3 共通ライブラリ

| ターゲット | 版との関係（案） |
|---|---|
| `pce_core`、`pce_input`、`trace_profiler`（PCERecomp） | 版に依存しない。変えない |
| `pce_app`（`runtime/app/*`） | **版に依存しない共通ライブラリのまま**。`g_ys_profile` を参照するが定義しない。リンクする側が profile を 1 つ選ぶ |
| `ys_profile_usa`、`ys_profile_jp` | 新設の小さな静的ライブラリ（`profile_<id>.c` 1 本ずつ） |
| `pce_runtime`、`app_tests` | 今は `pce_app` をリンクし、北米版の値で動いている [C]。**`ys_profile_usa` をリンクして、今と同じ動作にする** |
| `pce_trace` | 今は ys_* のソースを直接コンパイルしている（105-109）。北米版は `ys_profile_usa` をリンク。日本版は `ys_edition()` が `pce_trace_jp` を作る（日本版の装備フレームの生成とコンソールの試験に使う） |

- 同じ実行ファイルに profile が 2 つリンクされると `g_ys_profile` の重複定義でリンクが失敗する。これは「混ざらない」ことの保証として扱う（意図した失敗）。
- profile がまったくリンクされない場合も未定義シンボルで失敗する。

### 2.4 OS の部分（M1 の作業、`05` を参照）

- `if(MSVC)` / `if(WIN32)` の中身を `cmake/platform_windows.cmake` へ移す（フラグは変えない）。
- `cmake/platform_macos.cmake`（M3）：`-ffp-contract=off`（D4）、arm64 のみ（D9）、`MACOSX_BUNDLE`、生成コードの警告の抑制。
- `ys_edition()` は OS を見ない。OS ごとの違い（生成 C のフラグなど）は `platform_*.cmake` が `ys_recomp_<id>` などのターゲットに後から付ける関数（例：`platform_generated_code(<target>)`）で入れる。

---

## 3. ディレクトリ（P7）

```
games/
  ys/                 USA: unchanged (disc.toml, sessions/, coverage/, generated/)
  ys_jp/              JP (new, Phase 3)
    disc.toml         Phase 2 results; every value confirmed (no USA value copied)
    sessions/         recorded with YsIandII_JP (buttons + BRAM; header "# edition: jp")
    coverage/         recorded with the JP disc and the JP System Card
    generated/        not versioned (.gitignore: games/*/generated/ already matches [C])
```

- `.gitignore` の `games/*/generated/` は `games/ys_jp/generated/` にも当たる [C]（`review_jp/00` 13 章）。
- `games/ys/` のファイルを移動しないので、既存の CMake・ツール・セッションの参照はそのまま [C]。

---

## 4. 試験

### 4.1 ラベル

| ラベル | 意味 |
|---|---|
| 既存：`quick`、`console`、`recomp`、`history`、`package` | 変えない |
| 新設：`usa`、`jp` | 版。`ys_edition()` がその版の試験に付ける |
| 新設（M0-04 の案）：`public`、`private` | ROM の要否 |

- 例：`ctest -L quick -L jp`（ctest は複数の `-L` を AND で扱う）。
- OS は実行する機械で決まるのでラベルにしない（M0-04 と同じ）。

### 4.2 北米版

- 試験の名前、期待値（`PASS_REGULAR_EXPRESSION`、フレーム番号、RAM の値）、セッションは**変えない**（作業規則 v2 の 6）。
- `ys_edition(usa ...)` に移したあと、Windows VM の `-TestAll` で、試験の本数・合格・skip の件数が移す前と同じことを確かめる。
- `app_disc_id`、`app_cd_menu_music` の直書きのパス（136、140）は M2 でキャッシュ変数に替える（試験の中身は変えない）。

### 4.3 日本版（段階的に増やす）

| 段階 | 試験 | ROM |
|---|---|---|
| 3A / 3B | profile の依存の規則（`01` の 9 章、`05` の 7 章の grep） | 不要（public） |
| 3B | `YsIandII_JP --version` 相当で、版が `jp` であること、無効な機能の一覧がログに出ること | 不要（public） |
| 3B | 合成した小さな CUE で、日本版がディスクを受け付けないこと／明示のエラー文が出ること（`disc` が NULL の間） | 不要（public）。ROM 由来のデータは入れない |
| 3B | 版の違うセッションを渡すと拒否されること | 不要（public） |
| 3C | `recomp_disc_ysjp`（日本版の disc.toml とカバレッジの確認） | 要（private） |
| 3C | ロックステップ（再コンパイル版とインタプリタ）`recomp_lockstep_ysjp` | 要（private）。BIOS 必須なので `--bios` で動かす |
| 3D | Geargrafx との比較（D5）、機能ごとの試験（値を確定させた機能から） | 要（private） |

- 日本版の期待値は**北米版の値を流用しない**。日本版の実行で得た値を、何を確かめているかの説明とともに記録する。
- 日本版の参照データ（セッション、BRAM、カバレッジ）を Git に入れるかは、ROM 由来の内容を含まないことを確かめてから、ユーザーが承認する（M0-04 の W-JP の条件）。

---

## 5. Python ツールとパッケージ

### 5.1 ツールの固定値 [C]（M1 の段階 4・M2 で直す対象に、版の引数を足す）

| ファイル:行 | 固定している値 | 案 |
|---|---|---|
| `tools/trace/run_perf_check.py:76, 77, 91` | `build/Release/ys_trace.exe`、北米版の cue、`games/ys/sessions` | `--edition`（既定 `usa`）で実行ファイル名・cue の変数・GAME_DIR を選ぶ |
| `tools/trace/run_bios_check.py:20, 46` | `bios_check.exe`、`games/ys/sessions` | 同上 |
| `tools/trace/check_all_sessions.py:32, 33, 41, 45` | `ys_trace.exe`、`games/ys/sessions`、`dist/YsRecomp/sessions`、`tasklist YsRecomp.exe` | 同上（`tasklist` は Windows のときだけ。R22） |
| `tools/trace/check_session.py:26, 86` | `pce_trace.exe` | セッションのヘッダーの `# edition:` から実行ファイルを選ぶ |
| `tools/equip/equip_preview.py:33, 38, 120, 269`、`equip_ice_preview.py:85` | `games/ys/sessions/...`、`games/ys/generated/...` | `--game-dir` |
| `tools/app/run_app_start_test.py:26`、`run_launcher_test.py:54` | `YsRecomp.ini` | 引数で ini 名を渡す（CMake が `ys_edition()` から渡す） |
| `tools/app/run_launcher_test.py:71, 91` | 出力の `"Ys Book I & II chosen"` | 期待する文言を引数で渡す（北米版の値は変えない） |
| `tools/package/run_package_test.py:43, 52, 59, 68, 69, 72` | `YsRecomp/`、`YsRecomp.ini`、`YsRecomp.exe` | `--edition` |
| `tools/package/make_icon.py:117` | 出力 `runtime/app/ysrecomp.ico` | 出力先を引数に（`05` の 5 章） |
| `tools/pcerecomp_dir.py:23` | `gg_trace.exe` | M2（D5）。版には関係しない |

- 既定値は `usa` にして、今のコマンドがそのまま動くようにする。

### 5.2 パッケージ

| 項目 | 今 [C] | 案 |
|---|---|---|
| フォルダ・zip・実行ファイルの名前 | `make_package.py:1, 9, 19, 54, 65, 76, 79, 81, 87` | `--edition` で `dist/<EXE>/`、`dist/<EXE>.zip` |
| 残すファイル（`KEEP`） | `YsRecomp.ini`（20） | 版の ini 名 |
| README | `package/README.txt`（北米版の前提：1, 4, 16-30, 95-118 行） | 版ごとに別ファイル（例 `package/README_jp.txt`）。日本版は「System Card が必要」と書く（P1） |
| ディスク由来のファイル | `menu_font.bin`、`launcher_sfx.bin`（21） | 同じ（実行時にユーザーのディスクから作る。配布物には入れない） |
| ランチャーの画像 | `docs/` の 4 枚（23） | 版で共通か別かは `01` の Q2 |
| アイコン | `ysrecomp.ico` を埋め込み、埋め込みを確認（18, 40-44） | 版ごとのテンプレート（`05` の 5 章） |

### 5.3 ビルドのスクリプト

| スクリプト | 案 |
|---|---|
| `build.ps1`（Windows） | `-Edition usa|jp|all`（既定 `all`：ROM のある版をすべて）。`-Test` などは `-L <label>` に `-L <edition>` を足す。`-Package` は版ごとに `make_package.py --edition` を呼ぶ |
| `scripts/build.sh`（macOS、M1 の段階 5） | 同じ引数（`--edition`） |

---

## 6. 実装の順序（案）

CLAUDE.md の作業規則 v2 のフェーズ順に合わせる。各段は小さく、終わるごとに試験と Windows VM での確認手順を付けて止まる。

| 段階 | 内容 | 版 | OS |
|---|---|---|---|
| Phase 2 | 日米ディスクの解析。`01` の 10 章の UNKNOWN を確定させる | JP の値 | ― |
| M1 | host 層の骨格（ファイルの移動）、`platform_*.cmake`、`scripts/build.sh`、ツールの `.exe` 固定の除去 | 変えない | Windows の動作は同じ |
| M2 | ROM・BIOS のキャッシュ変数、**`ys_edition()` を北米版だけで導入**（今のブロックを関数へ移す。試験の名前・期待値は同じ）、ラベル `usa` | USA のみ | ― |
| M3 | macOS で北米版をビルド・試験（新しい基準） | USA のみ | macOS |
| 3A | profile の骨格と北米版の値の移行（`01` の 5 章。同一動作） | USA | 両 OS |
| 3B | `profile_jp.c`（identity・BIOS 方針のみ、機能はすべて NULL）、`YsIandII_JP`、BIOS 必須の流れ、無効機能のログとメニュー、ROM 不要の試験 | JP | 両 OS |
| 3C | Phase 2 で確定した値を `games/ys_jp/disc.toml` と `profile_jp.c` に入れ、機能を 1 つずつ有効にする | JP | 両 OS |
| 3D | 日本版の試験（セッション、カバレッジ、Geargrafx 比較） | JP | 両 OS |
| Phase 4 | 統合試験（W-US、W-JP、M-US、M-JP） | 両方 | 両方 |

## 7. 未解決（構成）

| ID | 内容 | 決める段階 |
|---|---|---|
| B1 | 日本版の cue・BIOS のキャッシュ変数名と、`roms/jp`、`roms/bios` の中のファイル名（M2。roms/ の中身は M2 で確認する） | M2 |
| B2 | 日本版の試験名の接尾辞（案 `_ysjp`） | 3B |
| B3 | `build.ps1 -Edition` の既定値（案 `all`） | M2 |
| B4 | 日本版の参照データを Git に入れるか（ROM 由来の内容がないことの確認後） | 3D（ユーザー判断） |
| B5 | `pce_runtime`（汎用の実行ファイル）に北米版の profile をリンクし続けるか、Ys のフックを外した `profile_none` を作るか | 3A |
