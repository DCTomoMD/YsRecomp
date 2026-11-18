# Phase 0：リポジトリ調査（日本版「イースI・II」対応の事前調査）

- 対象ブランチ：`feature/japanese-disc-support`
- 基準コミット：`7452c1b`（local_analysis/BASELINE_COMMIT.txt）／PCERecomp サブモジュール `1b9d25a`
- 調査日：2026-11-19
- 調査方法：ソース、文書、Git 履歴の読み取りのみ。
  - `roms/` の中身は読んでいない（`roms/README.md` を除く）。
  - ビルド、コード生成、ダウンロード、インストール、ファイル変更はしていない。
- **このプロジェクトは現時点で macOS ではビルドできない。** 本書の記述に動作確認済みのものはない。
  Windows でのビルド・試験は「Windows VM で確認が必要」と明記した。

## 表記

| ラベル | 意味 |
|---|---|
| **[C]** CONFIRMED | ソースや文書で直接確認した事実。行番号は基準コミット時点のもの。 |
| **[A]** ASSUMPTION | 推測。検証が必要。 |
| **[U]** UNKNOWN | 不明。Phase 2（日米ディスク解析）以降で確定させる。 |

`PCERecomp/` は `external/PCERecomp/` の略。

---

## 1. リポジトリ構造とサブモジュール

### 1.1 サブモジュール
- `external/PCERecomp` → `https://github.com/Fabiano-Gelain/PCERecomp.git`、コミット `1b9d25a`（`.gitmodules:1-3`）[C]
- PCERecomp の探索順は CMake と Python で同じ [C]。
  1. `PCERECOMP_DIR`（CMake のキャッシュ変数、または環境変数）
  2. 兄弟フォルダ `../PCERecomp`
  3. `external/PCERecomp`
  - CMake 側：`CMakeLists.txt:27-43`。Python 側：`tools/pcerecomp_dir.py:11-18`。

### 1.2 YsRecomp 本体（追跡ファイル約 297 本）

| パス | 役割 |
|---|---|
| `CMakeLists.txt`（865 行） | 全ターゲット、Ys の C コード生成、約 90 本の ctest 登録 [C] |
| `build.ps1` | Windows 専用のビルド・試験・パッケージ作成スクリプト [C] |
| `runtime/main.c`（3645 行） | プレイヤー本体。媒体の読込、フック登録、描画、音声、録画と再生、`PCE_APP_YS` のときの起動処理 [C] |
| `runtime/app/*.c/h`（28 モジュール） | PC 側の拡張。`ys_*` は Ys 固有のフック。ほかに crop、motion、menu、launcher、equip、battle、disc_id など [C] |
| `runtime/app/ysrecomp.rc`、`ysrecomp.ico` | Windows のリソース [C] |
| `games/ys/disc.toml` | 再コンパイラ用のディスクマップ（64 行） [C] |
| `games/ys/sessions/`（txt 約 78 本、brm 多数） | 作者が録画したボタン入力と、開始時の BRAM。回帰試験に使う [C] |
| `games/ys/coverage/`（55 本） | 実行アドレスのカバレッジ記録。`rom-crc32 FF2A5EC3` を含む [C] |
| `games/ys/generated/` | 生成された C コードの出力先。`.gitignore:16` で除外 [C] |
| `tests/app/app_tests.c` | アプリ層の単体試験。`--fingerprint` などのサブコマンドを持つ [C] |
| `tests/input/*` | 起動・ゲーム・セーブ用の入力スクリプトと `ys_save.brm` [C] |
| `tools/app/*.py` | YsRecomp.exe を動かす試験（16 本） [C] |
| `tools/trace/*` | ロックステップ、Geargrafx 比較、BIOS 比較。`pce_trace` の Ys 拡張（`ys_trace_ext.c`、`ys_actors.c`、`hud_old.c`）もここにある [C] |
| `tools/equip/*` | 装備の描画ルールと `equip_art.h`、`equip_frames.c` の生成 [C] |
| `tools/package/*` | 配布 zip の作成と試験、アイコン作成 [C] |
| `tools/code_same.py`、`tools/py_same.py` | 2 つの C／Python ツリーを、コメントと文字列を無視して比較する（翻訳でコードが変わっていないことの確認用） [C] |
| `docs/` | technical_notes、ys_notes（776 行）、ys_bios、devlog、ROADMAP、サイト用画像 [C] |
| `package/README.txt` | プレイヤー向けの説明書 [C] |
| `roms/README.md` | 必要なファイルの説明 [C] |

### 1.3 PCERecomp（追跡ファイル 137 本）
- `runtime/cpu`：HuC6280 インタプリタと再コンパイル用ヘッダ。`huc6280_recomp.h`、`recomp_sig.h` [C]
- `runtime/pce`：pce.c、vdc、vce、psg、mix、lockstep、coverage、bios_profile [C]
- `runtime/cd`：cdimage、scsi、cdrom、cdaudio、adpcm、origin [C]
- `runtime/bios`：C で書き直した BIOS（HLE）。hle_cd、hle_adpcm、hle_bram、hle_psg、hle_math、hle_irq、hle_ex、hle_font、hle_boot と、合成 BIOS の synth.c [C]
- `tools/recomp`：`recomp.py` と `pcerecomp/*.py`（cdimage、discmap、program、codegen、coverage など） [C]
- **PCERecomp には Ys 固有のコードがない。** 作者の規則でもそう定められている（`git show dcc02dc:CLAUDE.md` の「Nothing about Ys in PCERecomp」）。grep でも見つからない [C]

---

## 2. ビルドの流れ

### 2.1 build.ps1（Windows 専用）[C]
- VS 2022 BuildTools のパスが固定されている：`C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`（`build.ps1:17-20`）
- 初回は CMake で構成する：`-G "Visual Studio 17 2022" -A x64`（`build.ps1:34-37`）
- 次にビルドする：`--config Release --parallel`（`build.ps1:38-40`）
- 試験の指定（`build.ps1:44-54`）
  - `-Test`：`-L quick`
  - `-TestAll`：`-LE history`
  - `-TestHist`：`-L history`
  - 失敗した試験は `--repeat until-pass:2` で 1 回だけ再実行する
- `-Package`：`-L package` の試験のあと `python tools\package\make_package.py build\Release\YsRecomp.exe dist` を実行する（`build.ps1:55-64`）

### 2.2 CMakeLists.txt [C]
- 言語は C11 のみ（`project(YsRecomp C)`）（1-5）
- MSVC 用の設定（7-18）
  - `/W4 /utf-8`、`/MT`（静的ランタイム）
  - `enable_language(RC)`
  - Release では LTCG（58）。生成コードだけは LTCG を外す（260）
- SDL3 3.2.30 を FetchContent で取得し、静的ライブラリとしてビルドする（46-54）
  - **ビルド時にネットワーク接続が必要**
- ディスクと BIOS のファイル名が固定されている（24-25）
  - `YS_CUE = roms/Ys Book I & II (USA).cue`
  - `SYSCARD2 = roms/[BIOS] TurboGrafx CD System Card (USA) (v2.0).pce`
- **YS_CUE と SYSCARD2 の両方があるときだけ**、`ys_recomp`、`ys_trace`、`YsRecomp` の各ターゲットと、約 60 本の試験が定義される（240-813）
  - 片方でも欠けると YsRecomp.exe は作られない

### 2.3 ターゲット [C]

| ターゲット | 種類 | 行 | 内容 |
|---|---|---|---|
| `pce_core`、`pce_input`、`trace_profiler` | lib | PCERecomp/CMakeLists.txt | エミュレートされたコンソール |
| `pce_app` | lib | 64-95 | `runtime/app` の 28 本。WIN32 では `dwmapi` もリンクする |
| `pce_runtime` | exe | 100-101 | インタプリタのみ。汎用 |
| `pce_trace` | exe | 105-109 | トレースツールと Ys 拡張 |
| `app_tests` | exe | 117-118 | アプリ層の試験 |
| `ys_recomp` | lib | 256-263 | 生成された C。`/W1 /bigobj /MP` |
| `equip_frames` | lib | 268-279 | ディスクから生成した装備フレーム |
| `ys_trace` | exe | 286-291 | 再コンパイル版のトレースツール |
| `YsRecomp` | exe | 295-302 | `PCE_RECOMP`、`PCE_APP_YS`、`PCE_DOCS_DIR` を定義。WIN32 では .rc も入れる |

### 2.4 C コードの生成（静的再コンパイル）[C]
```
python PCERecomp/tools/recomp/recomp.py generate <SYSCARD2> --map games/ys/disc.toml --cd <YS_CUE> --out games/ys/generated/c
```
- 実行箇所：`CMakeLists.txt:252-253`
- 出力は `rc_table.c` と `rc_part00.c`〜`rc_part23.c`
  - 分割数 24 は `PCERecomp/tools/recomp/pcerecomp/codegen.py:28` の `PARTS = 24` と CMake の 243 行目で二重に定義されている
- 依存先：`disc.toml`、`pcerecomp/*.py`、`recomp.py`（254）
- 所要時間は約 12 分（README）
- `recomp.py` の処理（`disc_program()`、`recomp.py:127-136`）
  - BIOS を HuCard ROM として読む
  - CD イメージと DiscMap を読む
  - `CdProgram` で（ロード先、実バンク）ごとに仮想バンクを割り当てる
  - エントリ点から静的解析する
- `codegen.py` の出力
  - BIOS のバンク $20 未満は C に出力しない（`codegen.py:259-271`）
  - RAM 上の関数ごとに、ディスクのバイト列をシグネチャ（`HuRamSig`）として埋め込む（`codegen.py:250`、`recomp_sig.h:16-38`）
  - CD ゲームでは `recomp_rom_crc32 = 0`（`codegen.py:329`）
- 2 つ目の生成器（`CMakeLists.txt:269-276`）
  - `tools/equip/equip_frames.py` が `pce_trace`、YS_CUE、SYSCARD2 と、US のセッション 2 本（`session_20261023_000904`、`session_20261017_153505`）を使って `equip_frames.c` を作る
- `disc.toml` の読込には `tomllib` を使う。Python 3.11 以上が必要（`discmap.py:30`）

---

## 3. データの流れ

```
.cue/.bin ──► cdimage（C: PCERecomp/runtime/cd/cdimage.c / Py: pcerecomp/cdimage.py）
   │                │
   │   ビルド時：recomp.py generate + disc.toml + BIOS ──► rc_*.c ──► ys_recomp.lib
   │
   ▼ 実行時（YsRecomp.exe）
app_find_game()（main.c:202-248）
  ├─ YsRecomp.ini の game_folder ／ フォルダ選択ダイアログ（main.c:178-196）
  └─ disc_find_in_folder() → disc_check_cue()（disc_id.c）で北米版の指紋を照合
load_media()（main.c:126-136）→ pce_load_cd(NULL|bios, cue)
  └─ BIOS なし：合成 BIOS（synth.c）と C の BIOS（hle_*.c）、起動処理は hle_boot.c:176-236
recomp_lookup の設定（main.c:2624-2631）
  └─ RAM 関数ごとにシグネチャを照合し、合わなければインタプリタで実行
g_pce.on_instruction = app_instruction_hook（main.c:2660）
  └─ ys_engine / ys_windows / ys_scroll / ys_fade / ys_screen / ys_intro / ys_reward / ys_music（main.c:532-543）
フレームごと：ys_panel_read（577）、equip_read（594）、menu_font_capture（1012）
game_patch：ys_dialog_patch（ys_dialog.c:49）
BRAM：<ini のあるフォルダ>/saves/<cue 名>.brm（main.c:235-246, 2609-2620, 3617-3633）
```
上の流れはすべて [C]。

### 3.1 CUE/BIN の処理 [C]

C 側（`PCERecomp/runtime/cd/cdimage.c`）
- `cd_open()`（165-296）
  - FILE 名は、引用符で囲まれた部分をそのまま使う。囲まれていなければ `%1023s` で読む（194-207）
  - TRACK の型は `AUDIO`、`MODE1/2048`、`MODE1/2352` だけを受け付ける（231-240）
  - PREGAP、INDEX 00/01 を読む
- `open_bin()`（65-99）
  - 先頭が `/`、`\`、`X:` なら絶対パス、そうでなければ cue のフォルダからの相対パス
  - パスのバッファは 1024 バイト
- ファイルを開くときは `u8_fopen` を使う（`runtime/util/u8file.c:3-23`）
  - Windows では UTF-8 を `_wfopen` で開く。UTF-8 として不正なら ANSI で開く
- **NFC/NFD の正規化はどこにもない**（grep で確認）

Python 側（`pcerecomp/cdimage.py`）
- cue を `encoding="latin-1"` で開く（45）
- MODE2 は拒否しない。2352 バイト＋16 バイトのヘッダとして扱う（29, 111）
- トラック配置は Geargrafx の規則に従う（`_layout()` 67-96）。C 側の `layout()`（102-163）と同じ規則で、`tools/cd/run_cd_test.py:124-139` が両者の一致を確かめている

---

## 4. ディスクマップ `games/ys/disc.toml`

書式の定義は `PCERecomp/tools/recomp/pcerecomp/discmap.py:5-28`、読み込み処理は 60-114 [C]。

| キー | 値（北米版） | 行 | 意味 |
|---|---|---|---|
| `base_sector` | 3596 | 8 | データトラックの先頭セクタ |
| `[table] sector` | 3660 | 11 | ファイル 0 の位置 |
| `[table] offset` | 0x181 | 12 | ファイルサイズ表の位置（RAM の $4181）。読み込むのはローダのルーチン $4165 |
| `[table] entries` | 92 | 13 | ファイル数 |
| `[table] sectors_per_file` | 32 | 14 | ファイル N は base + (N+first)×32 から始まる |
| `[table] first` | 2 | 15 | |
| `[[far_call]]` | 0x4B63→81、0x4B69→82、0x4B6F→86（family=area） | 20-29 | 遠距離呼び出しの形：`JSR / BRA *+4 / .dw` |
| `boot` | sector 3596、2 セクタ、ram 0x10800、F8→窓 1、入口 1:F8:0B26 | 31-37 | IPL（$2800 に置き、$2B26 を呼ぶ） |
| `resident` | file 0、2 セクタ、ram 0、80→窓 2、入口 2:80:0000 | 39-45 | 常駐部分 |
| `program` | size 0x13、ram 0x0800、80/81/82/83、入口 2:80:0800、script {14=1, 04=2} | 47-54 | メインプログラム（$40E7 が $4800 に読み込む） |
| `area` | size 0x10、ram 0x6000、86→窓 3、loose_pointers、routine_records | 56-64 | エリアのコード（$410B が読み込む） |

- 値の出どころは `docs/ys_notes.md:748-765`（「Disc files and loader」）[C]
- **すべての数値は北米版から得たもの。日本版での値はすべて [U]。CLAUDE.md の規則 4 により流用しない。**
- 日本版でも同じ書式（family、far_call、table）で表せるかどうかは [A]。ローダの構造が同じなら表せる。

---

## 5. 北米版ディスクの識別

### 5.1 実行時の指紋（`runtime/app/disc_id.c`）[C]
- 定数（13-15）
  - `YS_TRACK_COUNT 43`
  - `YS_TOC_CRC 0x34B2F40E`
  - `YS_DATA_CRC 0xEC4E1070`
  - コメントによれば、`app_tests --fingerprint` で Redump 形式のイメージを測った値
- `disc_fingerprint()`（76-113）
  - TOC の CRC：トラックごとに（型、開始 LBA、セクタ数）の 9 バイトを並べて CRC32 をとる
  - データの CRC：最初のデータトラックの全セクタ 2048 バイトの CRC32
  - タイトル判定：データトラックの 2 番目のセクタに英語の文字列 `"Ancient Ys Vanished"` があるか（103-107）
- `disc_check_cue()`（115-148）
  - 判定の順序：Ys かどうか → トラック数 → TOC の CRC → データの CRC
  - エラー文はすべて英語で「USA」を前提にしている
- `disc_find_in_folder()`（162-185）
  - `SDL_GlobDirectory(folder, "*.cue", 大文字小文字を区別しない)` で cue を探し、最初に合格したものを使う
- 日本版について
  - 日本版に `"Ancient Ys Vanished"` があるかどうかは [U]
  - 日本版はこの検査で必ず拒否される [A]（CRC が違うため）

### 5.2 そのほかの北米版前提 [C]
- `main.c:161`：ウィンドウのタイトル `"Ys Book I & II"`
- `main.c:182`：ダイアログの文言 `"Choose the Ys Book I & II (USA) folder"`
- `main.c:219`：エラー文の `"Ys Book I & II (USA)."`
- `tests/app/app_tests.c:3189-3214`：`test_disc` が北米版 Redump の構成を作る
  - ファイル名 `"Ys Book I & II (USA) (Track %02d).bin"`、43 トラック、MSF は `00:03:06` と `00:02:03`
  - 日本語のフォルダ名（`configuração 日本`、99 行）の試験はある
- `tools/trace/run_perf_check.py:76-77`：北米版の cue 名が既定値
- coverage の `rom-crc32 FF2A5EC3` は北米版 System Card 2.0 の CRC
  - `recomp.py disc --check` は CRC が違うと拒否する（`coverage.py:133-134`）

---

## 6. ハードコードされたアドレスと runtime フック

### 6.1 仕組み [C]
- 命令フックの入口は `g_pce.on_instruction` だけ（main.c:2660）
- 各フックは、PC、MPR のバンク、コードのバイト列（シグネチャ）を順に照合する
  - アドレスが違っていても、フックが発火しないだけで、壊れはしない
  - ただしその場合は機能が黙って働かなくなる
- **ゲーム版（profile）の抽象化はない。** 値はすべてモジュールごとのリテラルか `#define`
- 版で分けているのは Ys I と Ys II の違いだけで、アドレスを 2 つずつ並べて判別している

### 6.2 モジュール別一覧 [C]

値はすべて北米版のもの。日本版での値は [U]。

| モジュール | フック／参照するもの | 主な値（ファイル:行） |
|---|---|---|
| `ys_engine.c` | バンク $80 のスプライトリスト作成、鉱山の明かり、主人公の生成 | `k_hooks` 0x4D58/4DAC/4E60/4DAA（15-20）、copy 0x7108・mpr3=86・$7673（30-44）、hero 0x75A5（Ys I）/0x75A7（Ys II）（49-60）、RAM 約 30 か所（$2F78、$2C33、$2ABB、$2C41 など。90-188） |
| `ys_windows.c` | バンク $81 のウィンドウ開閉、バンク $82 のレイアウト切替 | 0x6031/6037/6040/604D/6057/605D（17-22）、0x7AFC/7AAB/7B32/7AE1（35-56）、RAM $203E-43、$2C37-3A、$33C6/7 |
| `ys_scroll.c` | ライン割り込み | 0x4AE1/4B05/4AD8（16-18）、RAM $2C2F-31、$20F3、$2082、$220E-13 |
| `ys_fade.c` | バンク $80-$87 でバイト列を探す（PC は固定しない） | `RAM_COLORS 0x3C00`（8）、$221F、$2220/$2223。**版の違いに最も強い** [A] |
| `ys_screen.c` | プログラムローダ | `LOADER_PC 0x40E7`（8-10）、`BOOT_PC 0x40DC`、プログラム番号 $08/$30/$01/$04（46-58） |
| `ys_intro.c` | イントロのスクリプトとタイトルメニュー | `SCRIPT_PC 0x5AA4`、`MENU_PC 0x494C`、0x4C4A/4C6D/4D3D（6-29） |
| `ys_music.c` | 曲の開始 | `MUSIC_PC 0x4244`、`0x4247`（6-9） |
| `ys_dialog.c` | 文字送り速度を 0 にする（コンソールの状態を変える） | `SPEED 0x27F6`、0x58D6（Ys II）/0x5BD0（Ys I）/0x5BD5（20） |
| `ys_panel.c/h` | HUD パネルの VRAM タイル | タイル 256-266、449-460、282、506/507（`ys_panel.h:36-49`）、パネル位置 X/Y 8/179、RAM $2C48-4C |
| `ys_reward.c` | 敵の撃破、GOLD、EXP | Ys I $81:7959、Ys II $81:7998、$82:7097/7160、$82:707C/7145（19-26） |
| `ys_masks.c` | エンジンのデータの解釈 | flags&0x27==0x22、ポーズ 0x40-0x5F（132-139） |
| `equip.c` | アイテム表と装備スロット | $82:6321（582-586）、RAM $2C71-73 |
| `launcher_sound.c` | 画面に出さないコンソールで効果音を取り出す | SFX 0x13/0x12/0x17（12）、RUN を押すフレーム 1644-2560（17-31）、RAM $22D0/D1/E1/E2/$20E7 |
| `crop.c/h` | 画面の幾何、HUD の作り直し | FRAME 8/327/11、PANEL_Y 179、マップ 24/27/288/128、`hud_items()`（914-957） |
| `intro_skip.h` | フレームの上限 | `INTRO_MAX_FRAMES 4000`（35） |
| `tools/trace/ys_actors.c`、`ys_trace_ext.c`、`hud_old.c` | トレースツール用の拡張 | ys_engine のフックと同じものに、0x4B1B/0x4AA3 を追加（16-21） |

- 合計はおよそ PC フック 40 か所、RAM・タイル・表の定数 100 か所 [C、概数]
- 根拠となる一覧は `docs/technical_notes.md:25-43` と `docs/ys_notes.md:349-526`
- 日本版で同じアドレスがそのまま使える可能性は [A]。コードの位置が同じなら一部は使えるかもしれないが、仮定してはならない
- 日本版で特定し直す必要があるもの：PC、バンク、シグネチャのバイト列、RAM の配置 [U]

---

## 7. フォント、テキスト、ウィンドウ

### 7.1 ゲーム本文のフォント [C]
- Ys はディスクにフォントを持たない。本文は System Card の 12×12 フォントで描く（`docs/ys_notes.md:5-10`）
  - 実 BIOS があるとき：ゲームが BIOS の ROM を直接読む（$4057 → $455B）
  - BIOS がないとき：`ex_getfnt`（$E060）を呼ぶ。これは手描きの `PCERecomp/runtime/bios/hle_font.c` に置き換わっている
- **`hle_font.c` が描けるのは Shift-JIS の $8140-$8197 と $824F-$829A だけ**（全角の記号、数字、英字）
  - かなと漢字は空白になる（`hle_font.c:1-13`）
  - 実 BIOS のフォントは同梱できない（著作物のため）
- 日本版の本文はかなと漢字を `ex_getfnt` などで描くはず [A]
  - そうなら、BIOS なしで動かすと本文が出ない
  - 日本版対応で最大の課題になると予想する [A]
- ウィンドウの文字はタイルに画像として書き込む。書き込むコードは $81:6318-$64AC（`ys_notes.md:11-12`）
- テキストタスク（`ys_notes.md:53-60`）
  - バイトの変換は $5892
  - $20 未満の制御コードは $5852 の表で処理する
  - Ys I は $80:5A79、Ys II は $80:577F
  - 日本版の文字コード体系との関係は [U]

### 7.2 メニュー用 8×8 フォント（`runtime/app/menu_font.c/h`）[C]
- ディスクのセクタからではなく、VRAM のタイル 449-487 と 504 から取り込む
  - 取り込み：`menu_font_capture()`（`menu_font.c:31-37`）
  - 検証：`MENU_FONT_CRC 0xAADA7A4C`（`menu_font.h:22`）
  - 保存先：`menu_font.bin`（main.c:2596）
- `menu_font_glyph()`（`menu_font.c:164-187`）は ASCII の英数字と記号だけを対応づける
- 取り込みに失敗したときは、手描きの A-Z、0-9（110-152）を使う
- 日本版で同じタイル番号に同じ字形があるか、CRC が一致するかは [U]

### 7.3 英語表示を前提にした箇所 [C]
- HUD のラベル「H.P / EXP / GOLD / M.P」をタイル番号で見分けている（`ys_panel.h:6-9, 36-49`、`crop.c:914-957`）
- 「NEXT」は EXP ラベルの E と X のタイルを組み合わせて作っている（`crop.c:1069` 以降）
- 撃破時のポップアップ `"+%d EXP"`、`"+%d GOLD"`（`battle.c:215-219`）
- `fix_window`：北米版で文が長くなったことへの対処（`game_fixes.h:44-50`）
  - 日本版で必要かどうかは [U]
- `ys_windows.c` は位置と大きさ（$3F-$43、8 ピクセル単位）だけを扱う。文字や文字幅は扱わない

---

## 8. CDDA、ADPCM、効果音 [C]
- CDDA
  - `runtime/app` にトラック番号の直書きはない。再生はゲーム自身が操作する
  - イントロの曲はトラック 21（`ys_notes.md:283-284`、北米版）
- `ys_music.c`：$80:4244 で `cd_fade(0)` から `cd_play` までの間、CD の音を消す
- ADPCM
  - ゲーム側はローダの $414D を通して `ad_trans` / `ad_read` / `ad_play` を使う
  - アプリ側には音量の区分「voices」があるだけ（`volume.h:7`）
- PSG（チップ音）
  - BIOS のドライバ $E0D8 を使い、C で書き直したもの（`hle_psg.c`）がある
  - 音楽と効果音の区別は BIOS ドライバの RAM（$22DF、$2607+x）で判定する（`PCERecomp/runtime/pce/pce.c:591-603`）
  - 版によらない可能性が高い [A]
- 効果音の取り出し（`launcher_sound.c`）
  - 北米版のイントロの時間に合わせたフレーム番号で RUN を押し、効果音 ID を RAM に書き込んで鳴らす
  - 日本版では使えない [A]
- 音量の区分：master、music（CDDA）、chip_music、effects、voices（ADPCM）、chip_filter（`volume.c:14-20`）

---

## 9. セーブデータ [C]
- 2 KB の BRAM をそのまま保存する（`CD_BRAM_SIZE 0x800`、`PCERecomp/runtime/cd/cdrom.h:50`）
- 読み書きは `pce_load_bram` / `pce_save_bram`（`pce.c:136-162`）。サイズが違うファイルは読み込まない
- 保存場所
  - プレイヤー版：`<ini のフォルダ>/saves/<cue のファイル名>.brm`（main.c:235-246）
  - それ以外：作業フォルダの `saves/<cue のファイル名>.brm`（2609-2619）
  - `--save FILE` で指定できる（2479）
  - 終了時に書き出す（3617-3633）
- 中身は「HUBM」のヘッダと「YS.DATA.01」のエントリ（docs の調査結果。セッションの .brm 1 本で確認）
- アプリ側は BRAM の中身を解釈しない
- cue のファイル名でセーブが分かれるので、日米のセーブは自然に別ファイルになる [A]
- 日本版のファイル名とエントリ名は [U]

---

## 10. Ys I から Ys II への移行 [C]
- 「今が Ys II」という状態を明示的には持っていない。次の 3 つから推定している
  1. ローダのフックで読んだプログラム番号：$08 なら Ys I、$30 なら Ys II（`ys_screen.c:51-58`）
     - 順序は $04 イントロ → $28 → $08 → $20/$24（Ys I のエンディング）→ $30（`ys_notes.md:731-739`）
  2. 装備表 $82:6321 の先頭：{0,5,10,15} なら Ys I、{0,6,12,18} なら Ys II（`equip.c:582-586`）
  3. Ys I と Ys II で対になったフックアドレス（hero、reward、layout、dialog）
- Ys I のセーブから Ys II を始めると、99999 を超える EXP は 10000 になる（`ys_notes.md:549`）
- M.P は Ys II だけ。ラベルはタイル 266（`ys_panel.h:9, 42`）。`crop.c:1187-1193` で判定している
- 日本版のプログラム番号と移行の手順は [U]

---

## 11. テストと検証

### 11.1 ラベル（`CMakeLists.txt:815-853`）[C]

| ラベル | 内容 |
|---|---|
| quick | app_config、app_volume、app_menu、app_video、app_image、app_crop、app_redraw、app_fade、app_input、app_disc_id、app_cd_menu_music、app_start_ys、nobios_ys、app_launcher、equip_c_ys、app_mark_ys |
| console | Geargrafx・実 BIOS との比較。trace_ys_*、session_ys_*、fade_ys、coverage_ys、bios_c_ys、hle_timing_ys など |
| recomp | ロックステップ（再コンパイル版とインタプリタの比較） |
| history | 長いセッションでの古い不具合の再発確認 |
| package | app_package_ys、app_launcher_ys |

- すべての試験に `PCE_HIDDEN_WINDOW=1`、`SDL_AUDIO_DRIVER=dummy` を設定する（854-857）
- ROM や BIOS がないときは終了コード 77 で skip になる（`SKIP_RETURN_CODE 77`）
- 多くの試験は `PASS_REGULAR_EXPRESSION` に北米版のフレーム番号や数値を直接書いている
  - 例：`layout_switches_ys` の「3093 3178 3844 4012」（413）、`hud_save_ys`（560）、`battle_ys2`（734）、`--bat-routine 81:607E`（521-527）
- 録画済みのセッション、カバレッジ、`tests/input/*` は、北米版ディスクと北米版 BIOS のタイミングに依存する
  - **日本版の検証にはそのまま使えない** [A]

### 11.2 検証の仕組み [C]
- インタプリタを Geargrafx（`gg_trace.exe`、GPL-3、参照専用）と比べる
- 再コンパイル版をインタプリタとロックステップで比べる（`main.c:2635-2647`）
- C の BIOS を実 BIOS と比べる（`tools/trace/run_bios_check.py`）
- カバレッジを確認する（`recomp.py disc ... --check games/ys/coverage`）
- `gg_trace.exe` は `PCERecomp/tools/reference_trace/build/Release/` にあるもので、別にビルドが必要
  - `reference/` は Git に含まれない（`.gitignore:12`）

---

## 12. 作者の指示（上流）

### 12.1 上流 CLAUDE.md（`git show dcc02dc:CLAUDE.md`）[C]
- 開発環境は Windows 11、PowerShell、Python 3.14、VS 2022、`/MT`
- ROM、ISO、BIOS と、それらから生成したコードは Git に入れない
- 推測でハードウェアの詳細を決めない（`PCERecomp/docs/hardware/INDEX.md` と Geargrafx を参照する）
- 対症療法をしない。ゲームの挙動は画像ではなく、フック、逆アセンブル、RAM から読む
- **PCERecomp に Ys のものを入れない**
  - ゲーム固有の処理は TraceExt、セッションイベント、`disc.toml` の規則、`Pce.game_patch`、`Vdc.line_log` / `write_hook` を通して組み込む
- PC 側はエミュレートされたコンソールの状態を変えない
  - 例外はメニュー中の音楽と、文字を一度に表示する機能。どちらもセッションに記録される
- ファイルは `u8_fopen` で開く
- フレーム準備スレッドのロックの順序：`g_prep.lock` → `frame_lock` → `slot_lock`
- PowerShell 5.1 の `Get-Content` / `Set-Content` でファイルを書き換えない
- 開発用のオプション
  - `--bios`、`--record`、`--input`、`--bram`、`--save`、`--screen-dump`、`--lockstep`、`--interp-log`
  - 環境変数 `PCE_CD_LOG`、`PCE_AUDIO_LOG`、`PCE_RING_LOG` など

### 12.2 そのほかの文書 [C]
- `roms/README.md`、`package/README.txt:15-28, 116-118`：北米版 Redump（cue 1 本と 43 トラック）を前提にしている
- `tools/trace/README.md`：トレースツールとセッション形式の説明
- `docs/ROADMAP.md:17`：メニューは意図して英語にしている
- **日本版、地域の違い、ローカライズへの言及はリポジトリにも Git 履歴にもない**

### 12.3 注意：上流 CLAUDE.md の扱い [C]
- コミット `bc482c3` の件名は「append」だが、実際は上流の CLAUDE.md 本文（164 行）を削除して置き換えている
  - 現在の CLAUDE.md の末尾には「以下は … 作業規則」という区切りがあるが、その後に本文がない
- `local_analysis/CLAUDE.upstream.md` は上流の写しではない。現在の新しい規則とほぼ同じ内容で、違いは末尾の 6 行だけ
- **上流の作者の指示は Git 履歴（`dcc02dc`）にしか残っていない**
  - 復元するかどうかはユーザーが判断する（Phase 0 では変更しない）

---

## 13. ROM・BIOS・生成物の除外設定 [C]
- `.gitignore`
  - ROM とディスク：`roms/*`（`roms/README.md` は除く）、`*.pce`、`*.cue`、`*.bin`、`*.iso`、`*.img`、`*.chd`、`syscard*`
  - 生成物とビルド出力：`games/*/generated/`、`build/`、`out/`、`dist/`
  - 実行時の出力：`saves/`、`*.trace`、`*.log`、`tests/output/`、`*.conferencia`、`lockstep_divergence.txt`
  - その他：`reference/`、`.venv/`、`docs/pt/`、`docs/Icone_novo.png`
- `.git/info/exclude`（ローカルのみ）
  - `roms/`、`local_analysis/`、`.venv/`、`*.bin`、`*.cue`、`*.iso`、`*.pce`、`*.wav`、`._*`、`.DS_Store`、`CLAUDE.local.md`、`.claude/settings.local.json`
- 注意点
  - `.DS_Store`、`._*`、`local_analysis/` は `.git/info/exclude` だけで除外している。共有される `.gitignore` には書かれていない
  - `*.brm` は除外されていない。`games/ys/sessions/*.brm` と `tests/input/ys_save.brm` は意図して Git に入っている
    - 中身はゲームの状態（BRAM）で、コードではない
  - `menu_font.bin`、`launcher_sfx.bin`（ディスクから取り出したもの）は実行時に ini の隣に作られる
    - `*.bin` で除外される

---

## 14. Windows 専用部分の概要（詳しくは M0 で調べる）[C]

| 区分 | 箇所 |
|---|---|
| ビルド | `build.ps1`（VS2022 のパス固定、PowerShell）、CMake の MSVC フラグと `/MT`、`enable_language(RC)`、`ysrecomp.rc`（296-298） |
| OS API | `runtime/app/vblank.c:6-47`（DWM、QueryPerformance、SetPriorityClass。`#else` のスタブあり）、`gpu_time.c`（D3D11 のタイムスタンプ）、`dwmapi` のリンク（93-95） |
| コンパイラ | `launcher_image.c:20-27`（MSVC の pragma、SDL 内部の stb_image を使う）、`PCERecomp/runtime/cpu/huc6280_exec.h:19`（`__forceinline`）、`codegen.py:285` |
| ファイル入出力 | `PCERecomp/runtime/util/u8file.c`（`_wfopen`）、`cdimage.c:12-16, 85-89`（`_ftelli64`）、`app_tests.c:3334-3338`（`_fullpath`） |
| ツール | `PCERecomp/tools/trace/profiler.c`（dbghelp、winmm）、`*.exe` のパス（`CMakeLists.txt:44`、`tools/pcerecomp_dir.py:23`）、`check_all_sessions.py:41`（`tasklist`）、`make_package.py`（PE 形式の確認、CRLF） |
| 文書 | README と package の「Windows 10/11 only」、PCERecomp の README:82, 89 |

---

## 15. 北米版に固有と思われる箇所（まとめ）

| 項目 | 箇所 | 区分 |
|---|---|---|
| ディスクの指紋 | `disc_id.c:13-15, 104` | [C] 北米版専用 |
| ファイル名と文言 | `CMakeLists.txt:24-25, 136, 140`、`main.c:182, 219`、`package/README.txt` | [C] |
| ディスクマップ | `games/ys/disc.toml` 全体 | [C] 北米版から得た値 |
| フックと RAM | 6 章の全モジュール、`tools/trace/ys_*.c` | [C] 北米版で測った値 |
| HUD・ポップアップ | `ys_panel.h`、`crop.c`、`battle.c` | [C] 英語のタイルと文字列が前提 |
| メニューフォント | `menu_font.h:22`（CRC） | [C] |
| 効果音の取り出し | `launcher_sound.c:12-78` | [C] 北米版のイントロの時間が前提 |
| 試験データ | `games/ys/sessions`、`coverage`、`tests/input`、CMake の期待値 | [C] |
| BIOS | System Card 2.0（USA）、CRC FF2A5EC3、`hle.h:21` | [C] |

---

## 16. 日本版対応で調査が必要な箇所（Phase 2 以降）

| # | 調べること | 現状 |
|---|---|---|
| J1 | 日本版の CUE と BIN の構成（トラック数、データトラックの位置、MODE、FILE 名の文字コード） | [U] |
| J2 | IPL、ローダ（$40E7 / $410B / $4165 にあたるもの）、ファイル表の位置と大きさ | [U] |
| J3 | far call の形とルーチンのアドレス、family の分類 | [U] |
| J4 | 6 章の全フックについて、PC、バンク、シグネチャ、RAM を一つずつ特定し直す | [U] |
| J5 | テキストエンジン、文字コード、使うフォント（BIOS の 12×12 / 16×16 漢字） | [U] |
| J6 | HUD パネルのタイル配置と、メニュー用 8×8 フォントの位置 | [U] |
| J7 | プログラム番号と Ys I → II の移行の手順 | [U] |
| J8 | CDDA のトラックの割り当てと、ADPCM の使い方 | [U] |
| J9 | BRAM のエントリ名 | [U] |
| J10 | 日本版 System Card の種類（2.x / 3.0）、CRC、エントリ点。HLE が想定する US 2.0 と同じに動くか | [U] |
| J11 | `Pce.region_japan`（`pce.h:62`）。読まれる（`pce.c:254`）が、設定する場所がどこにもない | [C] 未設定 / 日本版で必要かは [U] |
| J12 | 日本版に `"Ancient Ys Vanished"` に相当する識別文字列があるか | [U] |

---

## 17. 危険箇所と、現時点で不明な事項

1. **生成コードのシンボルが 1 組しかない** [C]
   - `recomp_lookup` と `recomp_rom_crc32` は大域シンボルとして 1 つずつしかない（`huc6280_recomp.h:40-42`）
   - 日米の生成コードを 1 つの実行ファイルにリンクすると名前が衝突する [A]
   - 生成器に接頭辞を付けるか、版ごとの関数表にする必要がある。Phase 1 の設計課題
2. **BIOS なしで漢字が出ない** [C]
   - `hle_font.c` はかなと漢字を空白で返す
   - 日本版で BIOS なしの動作を保つには別の方法が必要 [A]
   - BIOS のフォントをソースに埋め込むことは規則 3 で禁止されている
3. **日本語ファイル名** [C]
   - `cdimage.py` は cue を latin-1 で読み、NFC への正規化もしない
   - C 側はバイト列をそのまま比べる
   - 日本語の FILE 名を含む cue や、macOS の NFD のファイル名では bin が見つからないおそれがある [A]
4. **roms/ の配置** [C/U]
   - CLAUDE.md は `roms/usa`、`roms/jp`、`roms/bios` の構成を想定している
   - CMake は `roms/` 直下の決まった名前（24-25）を参照する
   - 実際の `roms/` の中身は今回調べていない [U]。このままでは生成も試験も skip になる可能性がある [A]
5. **profile の抽象化がない** [C]
   - 約 140 個の定数が 15 以上のモジュールに散らばっている
   - CLAUDE.md の規則 8（版の差は profile に置く）に沿って作り直すと、変更の範囲が広い
6. **北米版の試験資産** [C]
   - セッション、カバレッジ、PASS の正規表現は北米版専用
   - 日本版には新しく録画した参照データが必要 [A]。ROM 由来のデータは Git に入れない点に注意
7. **フックの失敗が表に出ない** [C]
   - シグネチャが合わないとフックは黙って発火しなくなる
   - 日本版で一部の機能が気づかれないまま無効になるおそれがある
8. **上流 CLAUDE.md が消えている** [C]：12.3 章を参照
9. **ビルド時のネットワーク** [C]：SDL3 を FetchContent で取得する（CLAUDE.md の「ダウンロード禁止」との関係を各 Phase で確認する）
10. **Python のバージョン** [C]
    - 上流は Python 3.14 を前提にしている
    - `tomllib` を使うので 3.11 以上が必要
    - `.venv` の Python のバージョンは [U]（未確認）
11. **macOS で生成できるか** [U]：`recomp.py generate` が macOS で動くかどうか。M0 で調べる

---

## 18. ビルド・試験コマンドの候補（CLAUDE.md の「ビルドと試験」欄に記入する案）

> **Windows VM で確認が必要。** macOS では現時点で実行できない。以下は上流の README と build.ps1 から書き出したもので、未検証。

前提（Windows VM）
- Windows 10/11 x64、VS 2022 Build Tools（MSVC と付属の CMake）、Python 3.14、Git
- 初回の構成時に SDL3 をダウンロードするので、インターネット接続が要る
- `roms/` の直下に次の 2 つを置く
  - `Ys Book I & II (USA).cue` と、その .bin 43 本
  - `[BIOS] TurboGrafx CD System Card (USA) (v2.0).pce`

```powershell
git submodule update --init --recursive
.\build.ps1            # Release ビルド。初回は Ys の C 生成に約 12 分かかる
.\build.ps1 -Test      # quick ラベル（約 20 秒）
.\build.ps1 -TestAll   # history 以外の全試験（約 11 分、88 本。上流 CLAUDE.md の記載）
.\build.ps1 -TestHist  # history ラベル
.\build.ps1 -Package   # dist\YsRecomp\ と dist\YsRecomp.zip を作る
ctest --test-dir build -C Release -R <name> --output-on-failure   # 1 本ずつ詳しく見る
```

期待される結果（[A]、上流の記載による）
- ROM と BIOS を置いた場合：`-Test` が全件通過する
- 置いていない場合
  - YsRecomp.exe と ys_trace は作られない（`CMakeLists.txt:240`）
  - ROM を使う試験は skip（77）になる
  - `app_config` などの ROM 不要の試験だけが走る

macOS：M2 / M3 の後に記入する（現時点ではビルドできない）。

---

## 付録：Phase 0 で実行したコマンド（すべて読み取りのみ）
- `pwd`、`git branch --show-current`、`git status --short`、`ls -d roms local_analysis .claude`
- `git submodule status`、`cat .gitmodules`、`ls -la`、`git ls-files`（集計）、`git show --stat bc482c3`、`git show dcc02dc:CLAUDE.md`、`git log`、`git check-ignore -v`
- `cat` / `head` / `sed -n` / `grep -n` / `wc -l` / `diff`：対象は追跡中のソース、文書、`.gitignore`、`.git/info/exclude`、`local_analysis/BASELINE_*.txt`、`local_analysis/CLAUDE.upstream.md`、`.claude/settings.local.json`
- 読み取り専用のサブエージェント 3 体による grep と閲覧（`roms/` は対象外と指定した）
- `local_analysis/disc_hashes.txt` は開いていない
