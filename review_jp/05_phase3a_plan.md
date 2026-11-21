# Phase 3A 計画：profile の骨格と、北米版の値の移行（同一動作）

- 作成日：2026-11-22。対象ブランチ：`feature/japanese-disc-support`（`3a09fd0`、PCERecomp はフォークの `76ab827`）。
- **本書は計画である。** コード・CMake・試験は変更していない。実装は、5 章の質問への回答を得てから、段ごとに行う。
- 元にした文書：
  - `01_profile_design.md`：profile 層の設計（2〜5 章、9 章）。
  - `02_build_layout.md`：6 章の段階表と 7 章の B5。
  - `04_phase3b_plan.md`：3A の上に作る 3B の計画。5 章の Q8 で「`pce_runtime` の profile は 3A で決める」とした。
  - `review_portability/11_m3_summary.md`：基準 M-US。100 本のうち 99 本が合格し、`perf_ys` が skip。
- 表記：[C] 確認済み、[A] 推測、[U] 不明。行番号は `3a09fd0` の時点のもの。
- **日本版の値は本書に書かない。** 3A では日本版の値を 1 つも使わない。
- 文書の番号：`04` は 3B で作る無効な機能の表を `05_jp_disabled_features.md` と呼んでいる。本書が 05 を使うので、その表は
  `06` にする（3B で作るときに `04` も直す）。

---

## 1. 目的と終了条件

`02` の 6 章で、3A は「profile の骨格と北米版の値の移行（`01` の 5 章。同一動作）。版は USA、OS は両方」と決まっている。
3A では日本版のものは何も作らない。`profile_jp.c` は 3B で作る。

| # | 条件 |
|---|---|
| E1 | `runtime/profile/` があり、`g_ys_profile` を定義するのは `profile_usa.c` だけ。どの実行ファイルも、profile をちょうど 1 つリンクする |
| E2 | 移した値は北米版の今の値と同じ。値を比べる道具（4 章の T1）で、段ごとに差が 0 |
| E3 | 試験の名前と本数は M3 の後と同じ（100 本）。macOS と Windows の両方で、99 本合格・`perf_ys` だけ skip・失敗 0。期待値は 1 つも変えない |
| E4 | 依存の規則（`01` の 9 章）が `tools/check_layers.py`（試験 `host_layers`）で確かめられ、合格する |
| E5 | `runtime/app/`・`main.c`・`tools/trace/` に、版で分ける分岐（`#ifdef` や `edition_id` の比較）がない |
| E6 | PCERecomp は変えない。Ys のものも入れない |
| E7 | コードに残したディスク由来の値（ハードウェア、System Card の作業域、エンジンの特性）を、残した理由と一緒に表にする（3B 以降で日本版を確かめるための一覧） |

---

## 2. 調べたこと（2026-11-22、ソースの読み取りだけ）

### 2.1 移す値の量 [C]

`runtime/app/`、`tools/trace/`、`main.c` を、`01` の 3.3 の表と照らして調べ直した。
**北米版のディスクに由来するリテラルは約 750 個**ある。大部分は、命令のシグネチャのバイト。

| ファイル | およその数 | `01` の 3.3 のサブ構造体 |
|---|---|---|
| `ys_engine.c`・`.h` | 86 | `YsEngineProfile` |
| `ys_windows.c` | 95 | `YsWindowsProfile` |
| `ys_reward.c` | 85 | `YsRewardProfile` |
| `equip.c`・`.h` | 85（うち約 50 は絵から導いた色番号など） | `YsEquipProfile` |
| `ys_intro.c` | 47 | `YsIntroProfile` |
| `ys_fade.c` | 43 | `YsFadeProfile` |
| `ys_dialog.c` | 33 | `YsDialogProfile` |
| `ys_scroll.c`・`.h` | 27 | `YsScrollProfile` |
| `crop.c`・`.h` | 26 | `YsCropProfile` |
| `ys_panel.h`・`.c` | 24 | `YsPanelProfile` |
| `launcher_sound.c` | 16 | `YsLauncherSfxProfile` |
| `ys_screen.c` | 14 | `YsScreenProfile` |
| `menu_font.h`・`.c` | 12 | `YsMenuFontProfile` |
| `ys_music.c` | 10 | `YsMusicProfile` |
| `disc_id.c` | 8 | `YsDiscIdentity` |
| `main.c`・`launcher.c` の文言 | 14 | `YsIdentity` |
| `intro_skip.h` | 1 と、画面の列挙 | `YsIntroSkipProfile` |
| `tools/trace/`（`ys_actors.c`、`ys_trace_ext.c`、`hud_old.c`） | 90 | `YsTraceProfile` と、ほかのサブ構造体の共有 |
| `ys_masks.c`・`motion.*`・`battle.c`・`redraw.h`・`gpu_draw.c`・`game_fixes.c`・`fade.c` | 約 36 | **`01` の表にない**（2.3） |

### 2.2 そのまま移せないところ [C]

| 種類 | 場所 | 必要な書き換え |
|---|---|---|
| `switch` の case ラベル | `ys_screen.c:52-55`（プログラム番号）、`ys_windows.c:51-55`（PC）、`ys_fade.c:188, 191`（シグネチャの先頭のバイト） | case には定数しか書けないので、`if` か表を引く形にする |
| 表から手で導いた範囲 | `ys_engine.c:65`、`ys_windows.c:65`、`ys_scroll.c:29`、`ys_reward.c:37`、`tools/trace/ys_actors.c:39` | 先に PC を絞り込む範囲。profile の表の最小値と最大値から計算する形にする |
| 配列の大きさ | `ys_panel.h:53-54`（`YS_PANEL_ROWS/COLS/H/W`）、`crop.c:810`、`hud_old.c:77`、`equip.h:41`（`EQUIP_FRAMES`）、`menu_font.c:8` ほか（`MENU_FONT_WORDS`。キャッシュのファイルの大きさにもなる） | 実行時の値にはできない。5 章 Q2 |
| 多くのファイルで使うマクロ | `CROP_MAP_X/Y/W/H`（`motion.c`・`gpu_draw.c`・`battle.c`・`redraw.c`・`ys_windows.c`・`main.c`・`ys_trace_ext.c` の約 22 か所）、`YS_SCROLL_*_ROW`、`YS_TILE_*`、`INTRO_MAX_FRAMES` | 5 章 Q2 |
| 画面の列挙 | `intro_skip.h:24-30` の `IntroScreen` と、`ys_intro.c:21-23` の `k_screens` が対応している | 3A では列挙をそのまま残し、表の値だけを移す |
| 同じ値を別々に書いたもの | `0x13C7`（`ys_windows.c:216-217`。`0x33C7 - 0x2000` を手で計算した値）、プログラム番号 `0x04` が 2 か所（`ys_intro.c:6`、`ys_screen.c:46`）、`w < 0x30`（`ys_reward.c:66`。`YS_WALKERS` と同じ値）、`disc_id.c:127, 135` の文中の「43」（トラックの数） | profile では 1 か所にまとめ、ほかはそこから導く |
| RAM の番地の書き方が 4 通り | `RAM(p, 0x2C41)`、`ram[0x2C41 - 0x2000]`、`$2000` からの差（`ys_intro.c:27-29` の `0x3A` など）、物理番地 `0x1F0000 +`（`launcher_sound.c:22`） | 5 章 Q5 |

### 2.3 `01` の 3.3 の表にないもの [C]

- `ys_masks.c`・`ys_masks.h`：マスクと前後関係の、ゲームの規則。アクターのフラグ `0x20`・`0x08`・`&0x27 == 0x22`、
  ポーズの範囲、足の位置 `y + 8 + 24`、ブロックの高さ `YS_MASK_H 32`。
- `motion.h:37` の `MOTION_STEP 8`（ゲームの歩幅）、`motion.c:406`・`battle.c:361` のフラグ。
- `ys_scroll.h:23-24` の `YS_SCROLL_MAP_ROW 27`・`YS_SCROLL_LOW_ROW 155`（ゲームのラスタ分割の行）。
- `ys_engine.h:69` の `state == 0x50`（ウィンドウとして使われるアクター）。
- `menu_font.c:183-187`：文字からタイル番号への対応。
- `main.c:2372-2374`：`--launcher-wav` の効果音の範囲。
- `fade.c:67`：Adol のパレット 13。
- `RING 256` が 3 か所（`motion.c:15`、`gpu_draw.c:15`、`redraw.h:29`）、`RING_X 512`・`RING_Y 256`（`battle.c:16-17`）、
  マップの大きさ 64×32（`game_fixes.c:26`、`ys_windows.c:90-93`）。

### 2.4 `01` の中の食い違い [C]

- `01` の 3.3 は、ランチャーの効果音の取り出しが使う音源ドライバの RAM（`launcher_sound.c:61-65, 76-78` の `$22D0/D1`、
  `$22E1/E2`）を `YsLauncherSfxProfile` に入れるとしている。一方 3.4 は、`$2200-$22FF` を System Card の作業域として
  profile から除くとしている。
- 同じ範囲の番地はほかにもある：`ys_engine.c:160, 170` の `$2216`、`ys_scroll.c:85-86` と `ys_panel.c:24` の
  `$220E/0F/12/13`、`ys_fade.c:154-155, 180` の `$2220`・`$221F`・`$2222`。5 章 Q3 で決める。

### 2.5 `runtime/app/` と `tools/trace/` で同じ値を写しているもの [C]

| 値 | 場所 |
|---|---|
| 画面の部品を組み立てるフック 4 行 | `ys_engine.c:16-19` と `ys_actors.c:16-19`（まったく同じ）。範囲も `ys_engine.c:65` と `ys_actors.c:39` で同じ |
| アクターの RAM、カメラの RAM | `ys_engine.c:152-172` と `ys_actors.c:106-139` |
| ウィンドウの RAM `$203E` | `ys_windows.c` と `ys_trace_ext.c:176` |
| 文字の速さ `$27F6`、`$2092` | `ys_dialog.c:6`・`ys_engine.c:188` と `ys_trace_ext.c:1374, 1389` |
| EXP・GOLD・HP の RAM | `ys_engine.c`・`ys_reward.c`・`ys_panel.c` と `ys_trace_ext.c:1324-1333` |
| パレット 13 | `equip.c`・`fade.c:67` と `ys_trace_ext.c:926, 1194, 1240` |
| 画面の枠の幾何 | `crop.c:14-27`・`crop.h:79` と `hud_old.c:5-9` |

### 2.6 どの実行ファイルが profile を必要とするか [C]

`nm` で、今の実行ファイルにどのモジュールが入っているかを見た（macOS の `build/Release`）。

| 実行ファイル | profile を使うモジュール | 備考 |
|---|---|---|
| `YsRecomp`、`ys_trace` | すべて | 北米版の実行ファイル |
| `pce_trace` | `ys_engine`、`ys_windows`、`crop`、`menu_font` ほか 30 個の `ys_` の関数 | 汎用の名前だが、Ys の拡張（`ys_trace_ext.c`）を含む。`windows_ys1`・`panel_ys`・`windows_crop_ys` などの試験と、装備の絵の生成（`equip_frames.py`）がこれを使う |
| `pce_runtime` | `menu_chip`、`ys_panel_read` | `PCE_APP_YS` なしでビルドした `main.c`（試験のパターンとインタプリタ）。試験には使われていない |
| `app_tests` | `crop`、`disc_id`、`ys_windows`、`ys_masks`、`menu_font`、`launcher_sound` | quick の試験 |
| `equip_check` | `equip.c`、`ys_fade.c` | `equip_c_ys` の試験 |

### 2.7 試験の中の北米版の値 [C]

`tests/app/app_tests.c` は、枠と HUD の幾何（8、327、11、170、179、219、234 など）、マップの原点、タイル 449 を数字で直に書いている。
**これは変えない**（作業規則 v2 の 6）。値を profile に移したあとも、これらの試験は北米版の値を数字で確かめることになるので、
移す前と後の値が同じことの証拠にもなる。

### 2.8 依存の規則の試験 [C]

`tools/check_layers.py`（試験 `host_layers`、quick）は、すでに `runtime/profile/` を「OS に依存しない共通コード」として
見ており（規則 1）、`runtime/host/` から `profile/` と `g_ys_profile` を見ることを禁じている（規則 2）。
`01` の 9 章の残りの規則（profile → `app/`・`host/`・SDL の禁止、版の分岐の禁止）は、ここに足せる。

---

## 3. 作り方（案。5 章の回答で決める）

### 3.1 ファイル

```
runtime/profile/
  ys_profile.h      types only (stdint.h): YsProfile and one struct per module, no SDL, no app/, no host/
  profile_usa.c     const YsProfile g_ys_profile = { ... }: the USA values, moved unchanged
```

- `profile_report.c`（無効な機能のログ）は 3B で作る。3A の北米版は、無効な機能を 1 つも持たないため。
- CMake：`add_library(ys_profile_usa STATIC runtime/profile/profile_usa.c)`。`pce_app` には入れない。
  profile は実行ファイルごとにリンクで選ぶ（`01` の 2 章の 1）。どれにリンクするかは Q1。
- 各モジュールは `g_ys_profile.<module>` を読む。NULL のときにフックを登録しない処理（`01` の 4 章）は 3B で足す。
  3A の北米版では NULL がないので、動作は変わらない。

### 3.2 段（各段の終わりに macOS の全試験を回して止まる）

| 段 | 内容 | 主なファイル |
|---|---|---|
| 3A-1 | 骨格：`ys_profile.h`、`profile_usa.c`（`identity` と `disc`）、CMake の `ys_profile_usa`、リンクする実行ファイル（Q1）。`disc_id.c` の識別値と文言、`main.c` のタイトル・フォルダ選択の文言・ini 名・`host_paths_init` の app id、`launcher.c` の作品名。依存の規則（Q7）。値を比べる道具 T1 | `runtime/profile/`、`CMakeLists.txt`、`cmake/ys_edition.cmake`、`disc_id.c`、`main.c`、`launcher.c`、`tools/check_layers.py`、`tools/profile_values.py`（新規） |
| 3A-2 | 単純なフック表：`ys_music`、`ys_screen`（`switch` を表に）、`ys_scroll`、`ys_reward` | `runtime/app/ys_*.c` |
| 3A-3 | `ys_windows`、`ys_intro`、`ys_dialog`、`ys_fade` | 同上 |
| 3A-4 | `ys_engine`、`ys_panel`（RAM と、配列の大きさ以外のタイル）、`equip`（RAM・CD RAM・パレット）、`menu_font`（タイルと CRC）、`launcher_sound`（効果音の番号とフレーム）、`intro_skip` | 同上 |
| 3A-5 | 画面の幾何と配列の大きさ（Q2 の決定に従う） | `crop.*`、`ys_panel.h`、`equip.h`、`menu_font.*`、`hud_old.c` |
| 3A-6 | トレースの拡張：`tools/trace/` の写し（2.5）を profile の共有に替え、トレースだけの値を `YsTraceProfile` に | `tools/trace/ys_actors.c`、`ys_trace_ext.c`、`hud_old.c` |
| 3A-7 | まとめ：E1〜E7、コードに残した値の表（E7）、CLAUDE.md、Windows での before／after の確認手順 | `review_jp/05`（本書）、CLAUDE.md |

- 各段で確かめること（macOS）：ビルドの警告 0、`ctest -LE history -j 4 --repeat until-pass:2` で 90 本（89 合格、`perf_ys` skip）、
  T1 の差が 0。`history` は、`ys_engine`・`equip`・`crop` に触れる 3A-4・3A-5 の後と、3A-7 で回す。
- 1 段は、ビルドと試験を合わせて 30〜40 分ほど（全試験が約 19 分、history が約 12 分）[A]。

---

## 4. 同じ値であることの確かめ方

| ID | 方法 | 確かめること |
|---|---|---|
| T1 | `tools/profile_values.py`（新規、ROM は使わない）：その段の基準のコミット（`git diff <base>`）で、`runtime/app/`・`tools/trace/`・`main.c` から**消えた**数値と文字列のリテラルの並びと、`profile_usa.c` に**足した**リテラルの並びを、表ごとに突き合わせる | 移す途中で値を書き換えていない。書き方だけを変えたもの（Q5 の番地の書き方、`0x13C7` のような導いた値）は、変換の規則を道具に持たせ、規則に当てはまったものを別に一覧にする |
| T2 | 全試験（2.7 の、数字を直に書いた試験を含む） | 動作が同じ |
| T3 | `check_layers.py` | 依存の向き（E4、E5） |

- T1 は ctest には入れない（git の履歴が要るため）。段ごとに手で実行し、結果を本書に書く。

---

## 5. 決めていただくこと

| ID | 決めること | 案（推奨） | ほかの案 |
|---|---|---|---|
| Q1 | どの実行ファイルに、どの profile をリンクするか（`02` の B5、`04` の Q8） | **北米版の profile を、使う実行ファイルすべてにリンクする**（`YsRecomp`、`ys_trace`、`pce_trace`、`pce_runtime`、`app_tests`、`equip_check`）。今と同じ動作になる。`pce_trace` の試験（2.6）は Ys のフックを使うので、外すと動作が変わる | `pce_runtime` だけに、フックを外した `profile_none` を作る。動作が変わる（`menu_chip`・`ys_panel_read` が働かなくなる）うえ、試験はない。日本版に向けても要らない |
| Q2 | 配列の大きさになる値と、多くのファイルで使う画面の幾何のマクロ（2.2） | **3A では、コンパイル時の定数のまま `runtime/profile/ys_profile.h` に集める**。値は変えず、「両方の版で同じと仮定している [A]」と注を付ける。日本版で違うと分かったら（`01` の U12、Phase 2 の残り）、そのときに profile へ移す。3A-5 は、マクロを 1 か所に集めて、`hud_old.c` の写しをなくすだけの段になる | すべて profile の実行時の値にする。配列は最大の大きさで取り、profile の値がそれ以下であることを確かめる。約 35 か所の書き換えになり、3A で動作を変えるおそれが大きい |
| Q3 | System Card の作業域 `$2200-$22FF`（2.4）を profile に入れるか | **入れない**（`01` の 3.4 のとおり）。番地はディスクではなく System Card 2.0 に由来する [C]。E7 の表に「日本版の System Card 3.0 で同じかを確かめる（J10）」と書く。`01` の 3.3 の `YsLauncherSfxProfile` の行から RAM を外す | `launcher_sound` の RAM だけ profile に入れる（`01` の 3.3 のとおり） |
| Q4 | `tools/trace/` の写し（2.5） | **profile を共有する**：トレースの拡張も `g_ys_profile` を読み、写しをなくす。トレースだけが使う値は `YsTraceProfile` に入れる | 写しのまま、トレース用の profile を別に作る |
| Q5 | RAM の番地の書き方（2.2） | **profile では CPU から見た番地（`0x2C41` の形）にそろえる**。`$2000` からの差で書いてあるもの（`ys_intro.c:27-29` など）も、この形に直して移す。T1 は「差 + 0x2000 = 番地」の規則で同じことを確かめ、直したものを一覧にする | 今の書き方のまま、モジュールごとに移す（profile の中に 4 通りの書き方が残る） |
| Q6 | フック表の型（`01` の 3.2 の `YsHookRow` にそろえるか） | **3A では今の表の形のまま `ys_profile.h` に移す**（モジュールごとの構造体）。照合のコードを書き換えずに済み、同一動作を保ちやすい。そろえるのは、日本版で必要になったとき（3C） | 3A で `YsHookRow` にそろえ、照合のコードも共通にする |
| Q7 | 依存の規則の試験 | **`tools/check_layers.py` に規則を足す**（試験 `host_layers` のまま。本数は 100 のまま）：`runtime/profile/` から `app/`・`host/`・SDL を include しない。`runtime/app/`・`main.c`・`tools/trace/` に、`edition_id` の比較と `PCE_EDITION` を書かない | 新しい試験 `profile_layers` を足す（101 本になる） |
| Q8 | `ys_masks.c` などのゲームの規則（2.3。アクターのフラグ、歩幅 8、ブロックの高さ 32、マップ 64×32、`RING`） | **3A では移さない**。`01` の 3.4 の「両作に共通のエンジンの特性」（`YS_ACTORS` など）と同じ扱いにし、E7 の表に「Phase 2 の残りで日本版を確かめ、違えば移す」と書く | 新しいサブ構造体 `YsActorRules` を作って移す |
| Q9 | Windows での確認の頻度 | **3A-7 で 1 回**（M3 と同じ before／after。before は 3A-1 の直前、after は 3A-7）。各段は macOS の全試験で確かめる | 段ごとに Windows でも確かめる（7 回） |

- キャッシュのファイル名（`menu_font.bin`、`launcher_sfx.bin`）は profile に入れない（版ごとにフォルダが分かれるため。`01` の P4）。
- `menu_chip.c:7` の `MENU_CHIP_RET 0x3FFE`（ゲームが実行しない番地という前提）は Q8 と同じ扱いにし、E7 の表に載せる。

---

## 6. 危険と、起きたときの扱い

| ID | 危険 | 扱い |
|---|---|---|
| K1 | 移す途中で値を書き間違える | T1 で段ごとに差が 0 であることを確かめる。試験も回す |
| K2 | `switch` を表に替えたことで、一致の順序が変わる | 今の case の順に表を引く。同じ値の行がないことを T1 の一覧で確かめる |
| K3 | `const` の構造体から読むことで、生成されるコードの速さが変わる | `perf_ys` は skip なので測れない。実時間の試験（`TESTS_TEMPO_REAL`）としきい値はそのまま。遅くなって落ちたら止めて報告する（しきい値は緩めない） |
| K4 | `pce_trace` で装備の絵を生成する処理（`equip_frames.py`）が変わる | 生成した `equip_frames.c` が前と同じことを確かめる（3A-4 で、前後のファイルを比べる） |
| K5 | `app_prep_ys` のまれな失敗（M3-5-1） | 起きたら、前後の記録を貼って報告する。3A のせいかどうかは、前のコミットで同じ条件を回して確かめる |

---

## 7. 3A で閉じる項目

| ID | 内容 |
|---|---|
| B5（`02` の 7 章）、Q8（`04` の 5 章） | `pce_runtime` の profile：Q1 |
| `01` の 5 章の 3 | 値を機械的に突き合わせる道具：T1 |
| `01` の 9 章 | 依存の規則の試験：Q7 |

3B に残るもの：`profile_jp.c`、`profile_report.c`、NULL による機能の無効化、`YsIandII_JP`、BIOS の流れ（`04` の計画どおり）。
