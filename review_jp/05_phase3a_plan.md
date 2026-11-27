# Phase 3A 計画：profile の骨格と、北米版の値の移行（同一動作）

- 作成日：2026-11-22。対象ブランチ：`feature/japanese-disc-support`（`3a09fd0`、PCERecomp はフォークの `76ab827`）。
- 状態（2026-11-28）：**3A-1〜3A-7 は macOS で完了**（8〜14 章）。残りは Windows での確認（14.4）。1〜7 章は計画のときの記述のまま。
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
| 3A-1 | 骨格：`ys_profile.h`、`profile_usa.c`（`identity` と `disc`）、CMake の `ys_profile_usa`、リンクする実行ファイル（Q1）。`disc_id.c` の識別値と文言、`main.c` のタイトル・フォルダ選択の文言・ini 名・`host_paths_init` の app id。依存の規則（Q7）。値を比べる道具 T1 | `runtime/profile/`、`CMakeLists.txt`、`cmake/ys_edition.cmake`、`disc_id.c`、`main.c`、`tools/check_layers.py`、`tools/profile_values.py`（新規） |
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

**2026-11-22：ユーザーの回答は「Q1〜Q9 は全部推奨で」。** 9 問とも「案（推奨）」の列のとおりに決めた。

Q3 にはユーザーから補足があった。「用意した System Card は、北米版が 2.2、日本版が 3 で、メジャーアップデートなので中身は大幅に違うはず」。
- 版の番号について：`roms/bios/` にある北米版のファイルは `[BIOS] TurboGrafx CD System Card (USA) (v2.0).pce` で、
  中身の版の文字列は `VER. 2.00`。上流が前提とする CRC `FF2A5EC3` とも一致する（`03` の 8.6）[C]。2.2 ではない。
- 中身の違いについて：`03` の 8.6 の解析と合う。入口表の飛び先と割り込みの番地は、3.0 が 2.0 より −$19 ずれている。
  作業域を使う命令の数も一部違う（`$22E2`、`$221F`）[C]。作業域の番地の意味が同じかは [A] のまま。
- このため、`$22xx` を profile に入れず、日本版で確かめるまでコードに残す（Q3 の推奨）方針は、この違いとも合う。

- キャッシュのファイル名（`menu_font.bin`、`launcher_sfx.bin`）は profile に入れない（版ごとにフォルダが分かれるため。`01` の P4）。
- `launcher.c:12-14` の作品名と発売年は profile に入れない。日本版のランチャーは北米版と同じ表示にすると決めてある（`01` の P11）ので、
  版で変わらない。
- `edition_id`（`"usa"`）は 3A では足さない。使う場所（セッションの `# edition:`、起動のログ）は 3B で作るので、そのときに足す。
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

---

## 8. 3A-1 の結果（2026-11-22、macOS）

基準は `3956404`（本書の計画のコミット）。

### 8.1 変えたもの

| ファイル | 内容 |
|---|---|
| `runtime/profile/ys_profile.h`（新規） | `YsIdentity`（`app_id`、`ini_name`、`window_title`、`disc_name`）、`YsDiscIdentity`（トラック数、2 つの CRC、タイトルの文字列とセクタ、メッセージの中の名前）、`YsProfile`、`extern const YsProfile g_ys_profile` |
| `runtime/profile/profile_usa.c`（新規） | 北米版の値。`main.c` と `disc_id.c` から、値を変えずに移した |
| `runtime/app/disc_id.c`・`.h` | `YS_TRACK_COUNT`・`YS_TOC_CRC`・`YS_DATA_CRC` とタイトルの文字列をなくし、`g_ys_profile.disc` を読む。メッセージの中の名前とトラック数（「43」）も profile から。`.h` はコメントだけ |
| `runtime/main.c` | ウィンドウのタイトル、フォルダ選択の見出しと案内の文、`host_paths_init` の app id、ini の名前を `g_ys_profile.identity` から。F6 でタイトルに足す文は、文字列の連結から `snprintf` に替えた |
| `CMakeLists.txt`、`cmake/ys_edition.cmake` | `ys_profile_usa`（静的ライブラリ）を作り、`pce_runtime`・`pce_trace`・`app_tests`・`equip_check`・`ys_trace`・`YsRecomp` にリンク（Q1） |
| `tools/check_layers.py` | 規則 6（profile から `app/`・`host/`・SDL を include しない）と規則 7（版で分ける分岐を書かない）を足した（Q7）。試験 `host_layers` のまま |
| `tools/profile_values.py`（新規） | T1（4 章） |

### 8.2 確かめたこと

| 確かめたこと | 結果 |
|---|---|
| T1：`profile_values.py 3956404 --allow 1:2` | **OK** [C]。数字 4 個（43、2 つの CRC、タイトルのセクタ 1）が、コードから消えて profile に入った。文字列 10 個は、profile の値そのものか、新しい書式の `%s`・`%d` に profile の値を入れると元と同じになる。`--allow 1:2` は、`sizeof(title) - 1` の 2 個の `1` が `strlen` に替わって消えた分。わざと値を変えると、差として出ることも確かめた（トラック数を 44 にした場合と、`disc_name` を変えた場合） |
| `check_layers.py --pcerecomp external/PCERecomp` | **OK** [C]。新しい規則の正規表現は、違反の例に当たり、正しい書き方には当たらないことを確かめた |
| ビルド（`scripts/build.sh`） | 警告 0 [C]。リンク時の最適化（LTO）があるので、実行ファイルに `g_ys_profile` の名前は残らないが、値の文字列は入っている |
| 試験の名前と本数 | 前と同じ 100 本 [C] |
| `ctest -LE history -j 4 --repeat until-pass:2` | **90 本のうち 89 本合格、skip 1 本（`perf_ys`）、失敗 0**（1118 秒）[C]。`app_prep_ys` は 1 回目で合格（139 秒） |

- history は 3A-4・3A-5 の後と 3A-7 で回す（3.2 のとおり）。
- Windows での確認は 3A-7 でまとめて行う（Q9）。

### 8.3 3A-1 と一緒に行ったこと（ユーザーの指示、3A の範囲外）

- `build.bat`（新規）：Windows でダブルクリックすると `build.ps1 -Package` を実行し、最後に窓を止める。引数があれば
  そのまま `build.ps1` に渡す。改行は CRLF で、`.gitattributes`（新規）の `*.bat text eol=crlf` で保つ。
  README と CLAUDE.md に使い方を書いた。**Windows での動作は未確認**（この Mac では動かせない）。

---

## 9. 3A-2 の結果（2026-11-27、macOS）

基準は `3956404`。3A-1 がまだコミットされていないので、T1 は 3A-1 と 3A-2 を合わせて確かめた（3A-1 の `--allow 1:2` を含む）。

### 9.1 変えたもの

| ファイル | 内容 |
|---|---|
| `runtime/profile/ys_profile.h` | `YsMusicProfile`、`YsScreenProfile`、`YsScrollProfile`（と `YsScrollHook`、`YS_SCROLL_HOOK_*`）、`YsRewardProfile`（と `YsRewardHook`、`YS_REWARD_*`）。`YsProfile` に `music`・`screen`・`scroll`・`reward` を足した。表の形は各モジュールの今の形のまま（Q6） |
| `runtime/profile/profile_usa.c` | 4 つのモジュールの北米版の値。値は変えずに移した |
| `runtime/app/ys_music.c` | PC 2 つ、バンク、シグネチャ 6 バイトを `g_ys_profile.music` から |
| `runtime/app/ys_screen.c` | ローダとブートの PC・シグネチャ・バンク、プログラム番号を `g_ys_profile.screen` から。`ys_screen_layout` の `switch` は、case の順のまま `if` に替えた（K2） |
| `runtime/app/ys_scroll.c` | フック 3 行、バンク、RAM（`$2C2F/30`、`$2C31`、`$20F3`、`$2082` とその bit `$40`）を `g_ys_profile.scroll` から。`$220E/0F`・`$2212/13` は System Card の作業域なのでコードに残した（Q3） |
| `runtime/app/ys_reward.c` | フック 6 行と RAM（`$2022`、`$2736`、`$2C41`、`$2ABB`）を `g_ys_profile.reward` から。`w < 0x30` は `YS_WALKERS`（`ys_engine.h`、Q8 でコードに残す値）に替えた |
| `tools/profile_values.py` | `--new VALUE:COUNT` を足した（9.2）。文字列の組み立ては、目当ての文字列に現れる値だけを試すようにした（値が 140 個ほどに増え、組み合わせが多すぎて終わらなかったため。結果は変わらない） |

- 手で導いていた PC の範囲（`ys_scroll.c` の `0x4AD8`〜`0x4B05`、`ys_reward.c` の `0x707C`〜`0x7998`）は、profile の表の最小値と
  最大値から、初めてのフックのときに 1 回だけ計算する（2.2）。`ys_reward_init` はどこからも呼ばれていない（構造体は静的に 0）
  ので、範囲は状態の構造体ではなくモジュールの中に置いた。
- 2 バイト目以降の番地（`$2C30`、`$2C42`、`$2ABC`、`$2ABD`）は「先頭の番地 + 1、+ 2」で読む。profile には先頭だけを置く。
- プログラム番号 `0x04`（イントロ）は `YsScreenProfile.intro_program` に置いた。`ys_intro.c:6` の写しは 3A-3 でここを読むように替える。

### 9.2 T1 の調整と理由

T1 は全ファイルを合わせて数えるので、コードに新しく書いた数（ループの始まりの 0、「先頭 + 1」の 1 など）が、移した同じ数を隠す。
その分を `--new` で数える（コードに今ある個数より多くは書けない）。

| 引数 | 理由 |
|---|---|
| `--allow 1:2` | 3A-1（8.2） |
| `--allow 3:3`、`--allow 4:1` | シグネチャを比べるループの上限 `k < 3`（`ys_screen.c`、`ys_scroll.c`）、`k < 4`、`h < 3` を、`sizeof` と `YS_SCROLL_HOOKS` に替えた |
| `--allow 0x30:1` | `ys_reward.c` の `w < 0x30` を `YS_WALKERS` に替えた |
| `--allow 0x80:1` | `ys_music.c` でバンク `0x80` を 2 回書いていたのを、`bank` の 1 つにまとめた |
| `--allow 0x2C2F:1`、`0x2C30:2`、`0x2C42:1`、`0x2ABC:1`、`0x2ABD:1` | 同じ番地を 2 回読んでいたもの（`$2C2F/30`）と、2 バイト目以降を「先頭 + 1、+ 2」にしたもの |
| `--allow 0x4AD8:1`、`0x4B05:1`、`0x707C:1`、`0x7998:1` | 手で導いていた PC の範囲を、表から計算する形にした |
| `--new 0:3` | 範囲を計算するループの `h = 0` と最大値の初期値 `0`、`hooks[0]` |
| `--new 1:6` | `ys_scroll.c` の `h = 1` と `map_x + 1`（3 か所）、`ys_reward.c` の `gold + 1`・`exp + 1` |
| `--new 2:1` | `ys_reward.c` の `exp + 2` |

### 9.3 確かめたこと

| 確かめたこと | 結果 |
|---|---|
| T1（9.2 の引数） | **OK** [C]。数字 139 個が、コードから消えて profile に入った。文字列は 3A-1 の 10 個のまま |
| T1 がわざと変えた値を見つけるか | 見つけた [C]：フックの PC を `0x4AE1` → `0x4AE2`、`load_program` を `0x01` → `0x02`、`disc_name` を変えた 3 つの場合（それぞれ差 2、2、1 件）。確かめたあと、ファイルは元に戻した（`cmp` で同じ） |
| `check_layers.py --pcerecomp external/PCERecomp` | **OK** [C] |
| ビルド（`scripts/build.sh`） | 警告 0 [C] |
| 試験の名前と本数 | 前と同じ 100 本 [C] |
| `ctest -LE history -j 4 --repeat until-pass:2` | **90 本のうち 89 本合格、skip 1 本（`perf_ys`）、失敗 0**（1104 秒）[C]。やり直しになった試験はない。`app_prep_ys` は 1 回目で合格（139 秒）。実時間の試験も、しきい値を変えずに合格（K3） |

- history は 3A-4・3A-5 の後と 3A-7 で回す（3.2 のとおり）。3A-2 で触れたフックは、セッションの試験（`session_ys_*`）と枠・HUD の試験で通っている。
- Windows での確認は 3A-7 でまとめて行う（Q9）。

---

## 10. 3A-3 の結果（2026-11-27、macOS）

基準は `aaff255`（3A-1・3A-2 のコミット `6840786` の次の、`build.bat` のコミット。3A の値には触れていない）。

### 10.1 変えたもの

| ファイル | 内容 |
|---|---|
| `runtime/profile/ys_profile.h` | `YsWindowsProfile`（と `YsWindowsHook`、`YS_WIN_HOOK_*`）、`YsIntroProfile`（と `YsIntroOp`、`YS_INTRO_OP_*`）、`YsDialogProfile`、`YsFadeProfile`。`YsProfile` に `windows`・`intro`・`dialog`・`fade` を足した。表の形は各モジュールの今の形のまま（Q6） |
| `runtime/profile/profile_usa.c` | 4 つのモジュールの北米版の値。値は変えずに移した |
| `runtime/app/ys_windows.c` | フック 6 行とバンク `$81`、レイアウト切替のバンク `$82`・PC 4 つ・命令列 2 本、RAM（`$203E`、`$203F`〜`$2043`、カメラ `$2C37`〜`$2C3A`、スタック `$33C6`・`$33C7`）を `g_ys_profile.windows` から。`switch` の case（PC 4 つ）は、case の順のまま表を引く `for` に替えた（K2）。フックの enum `HOOK_*` は profile の `YS_WIN_HOOK_*` に（表の順を決めるため。値と順は同じ） |
| `runtime/app/ys_intro.c` | プログラム番号の写し `INTRO_PROGRAM 0x04` をやめ、`g_ys_profile.screen->intro_program` を読む。スクリプトとメニューの PC・シグネチャ・バンク、画面の操作 3 行、RAM（`$2062/63`、`$203A`、`$2024` とその bit 0、`$2EBF`）を `g_ys_profile.intro` から。列挙 `IntroScreen` はコードに残し、profile の行の順（`YS_INTRO_OP_*`）から列挙への対応表 `k_screen_of` を置いた（2.2） |
| `runtime/app/ys_dialog.c` | 速さの RAM `$27F6`、シグネチャ 26 バイト、試験と待ちの位置（10、23）、3 つのプログラムの PC、バンク `$80` を `g_ys_profile.dialog` から |
| `runtime/app/ys_fade.c` | 命令列 2 本（ワイルドカード付き）、その中の位置（段階 11、表 14、種類 16、閃光の zp 1）、種類を見分けるバイト `$65`、呼び手の命令（`$A5`、`$88 $91`）、色の RAM `$3C00` を `g_ys_profile.fade` から。命令列の先頭のバイトで分ける `op == 0x86`・`op == 0xB2`（2.2 の `ys_fade.c:188, 191`）は、`transform[0]`・`flash[0]` との比較に替えた。順は同じ |
| `tools/profile_values.py` | 数え方の不具合を直し、`--ram` と `--dim` を足した（10.2） |

- 手で導いていた PC の範囲（`ys_windows.c:65` の `0x6031`〜`0x605D`）は、3A-2 と同じく、profile の表の最小値と最大値から初めてのフックのときに 1 回だけ計算する。
- `0x13C7`（`0x33C7 - 0x2000` を手で計算した値。`ys_windows.c:216-217`）は、「RAM の終わりまでの残り」`sizeof(pce->ram) - (stack - 0x2000)` で profile の `stack` から導く。値は前と同じ `0x0C39`。
- 2 バイト目以降の番地（`$2040`〜`$2043`、`$2C38`、`$2C3A`、`$2063`）は「先頭 + n」で読む。profile には先頭だけを置く。
  `$2C39` は 16 ビットのカメラ X の次の値（カメラ Y）なので、`camera_y` として別に置いた。`$33C6`（長さ）と `$33C7`（中身）も、意味が違うので別に置いた [C: どちらも元のコードが別々に読んでいた番地]。
- `ys_intro.c` の RAM は、`$2000` からの差（`pce->ram[0x3A]` など）から、CPU から見た番地（`RAM(pce, 0x203A)`）に直した（Q5）。
- `ys_dialog.c` のシグネチャは CD RAM から直に読む。前は `& 0x1FFF` でバンク `$80`（CD RAM の先頭）を前提にしていたが、
  バンクを profile から読むようになったので、CD RAM の中の位置を `(bank - 0x80) * 0x2000` で足した。北米版ではバンクが `$80` なので 0 で、結果は同じ。
- コードに残したもの：`$2200-$22FF`（`ys_fade.c` の `$221F`、`$2220`、`$2222`。Q3）、CD RAM のバンク `$80`〜`$87`・RAM のバンク `$F8`・
  スタックのページ `$100`・色のマスク `$1FF`・VCE の 512 色（ハードウェア）、色の RAM の大きさ `0x400`（配列 `YsFade.ram[512]` の大きさ。Q2）、
  フェードの表の 8×8、マップの 64×32・アクターのフラグ `0x20`（Q8）、スプライトの SAT の差 32・88（ハードウェア）、`CROP_MAP_*`（3A-5）。
  `tools/trace/ys_trace_ext.c` の `$203E`・`$27F6` の写し（2.5）は 3A-6 で直す。

### 10.2 T1 の直しと、引数の理由

**直したこと**：コードから消えた数を `Counter` の引き算で数えていたため、ある数がコードで「消えた数より新しく書いた数のほうが多い」とき、
差が 0 に切り捨てられ、そのあとで `--new` を足していた。このため `--new` が実際より少なくても OK になることがあった。
符号付きで数え、`--new`・`--allow` を引いたあとで負の数（コードに新しく書いた数。前と同じく一覧に出すだけ）を 0 にするように直した。

- この直しで 3A-1・3A-2 を数え直すと（`HEAD` の木を取り出し、基準 `3956404`）、9.2 の引数のうち `--new 1:6` は **`--new 1:7`** が正しい。
  7 個目は `ys_reward.c` の `g_pc_range = 1`（9.2 では `ys_scroll.c` の `map_x + 1` を 3 か所と数えていたが、実際は 2 か所と `g_pc_range = 1`）。
  `--new 1:7` で OK になる [C]。値の移し方に誤りはなく、引数の説明だけの誤り。
- 足した規則（4 章の「変換の規則を道具に持たせる」）：
  - `--ram OFFSET:COUNT`：`$2000` からの差（`ram[OFFSET]`）でコードから消え、CPU の番地 `OFFSET + 0x2000` で profile に入った（Q5）。
  - `--dim VALUE:COUNT`：別々に書いていた値をまとめた、profile の新しい配列の大きさ（3 つの比較が `test_pc[3]` になる、など）。コードからは消えずに profile に入る。

3A-3 の引数（基準 `aaff255`）：

| 引数 | 理由 |
|---|---|
| `--allow 0x6031:1`、`0x605D:1` | 手で導いていた PC の範囲を、表から計算する形にした |
| `--allow 0x13C7:2` | 手で計算した値を、`stack` から導く形にした |
| `--allow 0x203E:1`、`0x203F:1`、`0x2040:3`、`0x2041:2`、`0x2042:2`、`0x2043:2` | 同じ番地を何度も読んでいたものと、2 バイト目以降を「`rect` + n」にしたもの |
| `--allow 0x2C38:1`、`0x2C3A:1` | カメラの上のバイトを「+ 1」にした |
| `--allow 0x63:1` | スクリプトのポインタの上のバイトを「`script` + 1」にした |
| `--allow 0x04:1` | `ys_intro.c` のプログラム番号の写し（profile の `intro_program` は 3A-2 で入っている） |
| `--allow 0x86:1`、`0xB2:1` | `op == 0x86`・`op == 0xB2` を、命令列の先頭のバイトとの比較にした |
| `--allow 5:1` | シグネチャを比べるループの上限 `k < 5` を `sizeof` に替えた |
| `--allow 15:1`、`17:1` | `pc + 15`・`pc + 17` を `table_at + 1`・`family_at + 1` にした |
| `--allow 0:2` | 表の大きさの `sizeof(k_hooks[0])`・`sizeof(k_screens[0])` をなくした |
| `--new 0:13` | `ys_windows.c` の 7（`one_scroll_pc[0]`・`split_pc[0]`、ループの `k = 0` 2 つ、`hooks[0]`、`rect_byte(pce, 0)` 2 つ）、`ys_dialog.c` の `test_pc[0]`、`ys_fade.c` の 5（`transform[0]` 2 つ、`flash[0]` 2 つ、`caller_op[0]`） |
| `--new 1:12` | `ys_windows.c` の 7（`h = 1`、`g_pc_range = 1`、カメラの `+ 1` 2 つ、`rect_byte(pce, 1)` 3 つ）、`ys_intro.c` の `script + 1`、`ys_dialog.c` の `test_pc[1]`、`ys_fade.c` の 3（`table_at + 1`、`family_at + 1`、`caller_op[1]`） |
| `--new 2:3`、`3:2`、`4:2` | `rect_byte(pce, 2)` 2 つと `test_pc[2]`、`rect_byte(pce, 3)` 2 つ、`rect_byte(pce, 4)` 2 つ |
| `--new 0x80:1` | `ys_dialog.c` の CD RAM の先頭のバンク（ハードウェア）。北米版のバンク `$80` は profile に移った |
| `--ram 0x24:1`、`0x3A:1`、`0xEBF:1`、`0x62:1` | `ys_intro.c` の RAM を CPU の番地に直した（`$2024`、`$203A`、`$2EBF`、`$2062`） |
| `--dim 2:3`、`3:1` | `one_scroll_pc[2]`・`split_pc[2]`（case 2 つずつ）、`caller_op[2]`（比較 2 つ）、`test_pc[3]`（比較 3 つ） |

- コードに新しく書いた数（一覧に出るだけ）：`0x2000` が 1 個（`ys_intro.c` と `ys_dialog.c` に 1 個ずつ足し、`ys_windows.c` の `0x2000 - 0x13C7` 2 個を 1 個にした）。

### 10.3 確かめたこと

| 確かめたこと | 結果 |
|---|---|
| T1（10.2 の引数） | **OK** [C]。数字 217 個が、コードから消えて profile に入った（`--ram` 4、`--dim` 4 を含む）。文字列は 0 |
| T1 がわざと変えた値を見つけるか | 見つけた [C]：フックの PC `0x604D` → `0x604E`、`mask` `0x2EBF` → `0x2EBE`、`sig_wait` 23 → 24、`colors` `0x3C00` → `0x3C01` の 4 つ（それぞれ差 2 件）。確かめたあと、ファイルは元に戻した（`cmp` で同じ） |
| `check_layers.py --pcerecomp external/PCERecomp` | **OK** [C] |
| ビルド（`scripts/build.sh`） | 警告 0 [C] |
| 試験の名前と本数 | 前と同じ 100 本 [C] |
| `ctest -LE history -j 4 --repeat until-pass:2` | **90 本のうち 89 本合格、skip 1 本（`perf_ys`）、失敗 0**（1118 秒）[C]。やり直しになった試験はない。`app_prep_ys` は 1 回目で合格（142 秒）。実時間の試験も、しきい値を変えずに合格（K3） |

- history は 3A-4・3A-5 の後と 3A-7 で回す（3.2 のとおり）。3A-3 で触れたフックは、セッションの試験（`session_ys_*`）、
  ウィンドウの試験（`windows_ys`、`windows_ys1`、`windows_crop_ys`、`window_ys`、`window_ring_ys`）、`intro_ys`、`dialog_ys`、`equip_c_ys`（`ys_fade.c`）で通っている。
- Windows での確認は 3A-7 でまとめて行う（Q9）。

---

## 11. 3A-4 の結果（2026-11-28、macOS）

基準は `f2caa50`（3A-3 のコミット）。

### 11.1 変えたもの

| ファイル | 内容 |
|---|---|
| `runtime/profile/ys_profile.h` | `YsCommonRam`、`YsEngineProfile`（と `YsEngineHook`、`YS_ENGINE_HOOK_*`）、`YsPanelProfile`、`YsEquipProfile`、`YsMenuFontProfile`、`YsLauncherSfxProfile`、`YsIntroSkipProfile`。`YsProfile` に 7 つのポインタを足した。`YsRewardProfile` から `actor`・`gold`・`exp` を `YsCommonRam` に移した（下の「同じ値の写し」） |
| `runtime/profile/profile_usa.c` | 北米版の値。値は変えずに移した |
| `runtime/app/ys_engine.c` | フック 4 行とバンク `$80`、鉱山の明かりの写し（`$86:7108`、シグネチャ、`$7673`）、Adol の誕生（`$75A5`・`$75A7`、シグネチャ）、キャラクターの表 7 つ、リストのカメラ、歩く者の表 7 つと `$2092`、前景のセルの RAM 6 つを `g_ys_profile.engine` から。歩く者のキャラクター・EXP・GOLD は `g_ys_profile.common` から。16 ビットの読み方 8 か所は `ram16` 1 つにまとめた |
| `runtime/app/ys_reward.c` | 歩く者のキャラクター・GOLD・EXP を `g_ys_profile.common` から（値は同じ） |
| `runtime/app/ys_panel.c`・`.h` | 次のレベルの EXP `$2C4A`（3 バイト）と最大 H.P `$2C48`。タイルのマクロ `YS_TILE_*`（13 個）は名前を残し、中身を `g_ys_profile.panel` を読む式にした（`crop.c`・`ys_trace_ext.c`・`app_tests.c` は変えずに済む）。`YS_PANEL_X/Y/ROWS/COLS`・`YS_BAR_FIRST_COL`（パネルの位置と配列の大きさ）は 3A-5 |
| `runtime/app/equip.c` | 品目の表（`$82:6321`）と Ys I・Ys II の先頭の並び、装備の RAM `$2C71`（3 バイト）、Adol のパレット 13 と氷のパレット 14 を `g_ys_profile.equip` から。並びの比較は `memcmp` にした（同じ結果） |
| `runtime/app/fade.c` | 装備の直接の色を読むパレット 13（`256 + 13 * 16 + 1` から `256 + 14 * 16` まで）を `adol_pal` と `adol_pal + 1` から |
| `runtime/app/menu_font.h` | `MENU_FONT_FIRST`（449）・`MENU_FONT_ARROW`（504）・`MENU_FONT_CRC` を `g_ys_profile.menu_font` から。`MENU_FONT_TILES`・`MENU_FONT_WORDS` は配列とキャッシュのファイルの大きさなので 3A-5（Q2） |
| `runtime/app/launcher_sound.c`・`.h` | 効果音の番号 3 つ、隠しスクリプトのフレーム（決まった押下 3 組、最初の連打、40 フレームごとに 6 フレーム、上限 7000、落ち着くまで 150）、音源ドライバのフラグ `$20E7` とマスク `$7F` を `g_ys_profile.launcher_sfx` から。`launcher_sfx_numbers` は同じ名前のマクロにした（`main.c`・`app_tests.c` は変えずに済む） |
| `runtime/app/intro_skip.h` | `INTRO_MAX_FRAMES`（4000）を `g_ys_profile.intro_skip` から |
| `tests/app/app_tests.c` | 2038 行の表 `k_kind1`・`k_kind2` を `static const int` から `const int` にした。タイルが定数でなくなり、静的な初期化に書けないため。値も期待も変えていない（作業規則 v2 の 6 に当たらない） |
| `tools/profile_values.py` | `--as OLD:NEW:COUNT` を足した（11.2） |

- 手で導いていた PC の範囲（`ys_engine.c:65` の `0x4000`〜`0x6000`。窓 2 の範囲）は、3A-2・3A-3 と同じく、profile の表の最小値と最大値から計算する形にした。
  範囲は `$4D58`〜`$4E60` と狭くなるが、範囲は表を引く前の速い試しにすぎず、表の PC はすべてその中にあるので、結果は同じ。
- **同じ値の写し**：歩く者のキャラクター `$2736`、EXP `$2ABB`、GOLD `$2C41` は、3A-2 で `YsRewardProfile` に入れていたが、`ys_engine.c` も同じ番地を読む。
  2 か所に置かないように、両方が読む `YsCommonRam`（`g_ys_profile.common`）に移した。`tools/trace/` の写し（2.5）は 3A-6 でここを読むようにする。
- `equip.c` の表の位置は、前は CD RAM の中の位置 `2 * 0x2000 + 0x0321` と書いていた。profile には文書と同じバンクと論理番地（`$82`、`$6321`）を置き、
  `(items_bank - 0x80) * 0x2000 + (items & 0x1FFF)` で同じ位置 `0x4321` を求める。
- パネルのタイルの `tile_h`（257）と `tile_label_first`、`tile_m`（266）と `tile_label_last`、`tile_slash`（460）と `tile_digit_last`、
  メニューのフォントの `first`（449）とパネルの `tile_digit_first` は、北米版では同じ値だが意味が違う（文字の範囲と、ラベルの 1 文字目など）。
  日本版で別の値になりうるので、別の項目のままにした。
- コードに残したもの：`$2216`（`ys_engine.c`。System Card の作業域、Q3）、`$220E/0F/12/13`（`ys_panel.c`、Q3）、`$22D0/D1`・`$22E1/E2`（`launcher_sound.c`、Q3）、
  CD RAM のバンク `$80`〜`$87`（ハードウェア）、`YS_ACTORS`・`YS_WALKERS`・状態 `0x50`（Q8）、前景の表の大きさ 256（配列、Q2）、
  装備の絵のポーズ `$10`〜`$1F` と絵から導いた値（`equip.c`、`01` の 3.3 でも絵の側として扱う）、文字からタイル番号への対応（`menu_font.c:176-179`、2.3）、
  効果音を取り出すときの一般の上限（`CAP_SFX_MAX` 240、`CAP_SFX_QUIET` 20、無音の判定 8、1 フレームの標本 735、次の効果音までの 30）。
  最後のものは音を切り出す規則で、ディスクの中身ではない [A]。日本版で効果音が長ければ変える。

### 11.2 T1 の引数と理由

`--as OLD:NEW:COUNT` を足した：`--ram` と同じ「書き方を変えた値」の一般の形。`OLD` がコードから消え、`NEW` が profile に入った（コードは `NEW` から `OLD` を求める）。

3A-4 の引数（基準 `f2caa50`）：

| 引数 | 理由 |
|---|---|
| `--allow 0x4000:1`、`0x6000:1` | 手で書いていた PC の範囲を、表から計算する形にした |
| `--allow 0x7108:1` | 写しの PC を 2 回書いていた（比較と読む先） |
| `--allow 0x2053:1`、`0x205E:1`、`0x2060:1`、`0x2C34:1`、`0x2C36:1`、`0x2C56:1`、`0x2ABC:1`、`0x2C42:1`、`0x2C4B:1`、`0x2C4C:1`、`0x2C72:1`、`0x2C73:1` | 2 バイト目以降を「先頭 + n」（`ram16`、`slots + 1` など）にした |
| `--allow 0x20E7:1` | 同じ番地を読んで書いていた |
| `--allow 0x2736:1`、`0x2ABB:1`、`0x2C41:1` | `ys_engine.c` の写し。値は 3A-2 から profile にある（`YsCommonRam` に移した） |
| `--allow 0:3`、`1:2`、`2:2`、`3:2` | `equip.c` の `t[0]`〜`t[3]`（並びの比較を `memcmp` にした。`3` は 2 個）と、`ys_engine.c` の `sizeof(k_hooks[0])`（`0` の 3 個目） |
| `--allow 3:2` の残り（計 `--allow 3:4`） | `launcher_sound.c` のループの上限 `i < 3`（`sizeof` にした）と、`launcher_sfx_numbers[3]` の大きさを `.h` と `.c` の 2 か所に書いていたもの |
| `--allow 5:1`、`6:1`、`7:1` | シグネチャを比べるループの上限 `k < 5`、`k < 6`、`k < 7` を `sizeof` にした |
| `--allow 8:8`、`--new 8:1` | `ys_engine.c` の 16 ビットの読み方 `<< 8` の 8 か所を、`ram16` の 1 か所にまとめた |
| `--allow 13:2`、`14:3` | パレット 13・14 を何度も書いていたもの（`equip.c`）と、`fade.c` の `14 * 16`（`adol_pal + 1` にした。その `1` は `--new 1` に数えた） |
| `--new 0:5` | `ys_engine.c` の `hooks[0]`・`hero_pc[0]`、`launcher_sound.c` の `sizeof(p->fixed[0])`、`equip.c` の `memcmp(...) == 0` 2 つ |
| `--new 1:7` | `ys_engine.c` の 4（`h = 1`、`g_pc_range = 1`、`hero_pc[1]`、`ram16` の `a + 1`）、`fade.c` の `adol + 1`、`ys_panel.c` の `next_level + 1`、`equip.c` の `slots + 1` |
| `--new 2:2` | `ys_panel.c` の `next_level + 2`、`equip.c` の `slots + 2` |
| `--new 0x80:1` | `equip.c` の CD RAM の先頭のバンク（ハードウェア）。北米版のエンジンのバンク `$80` は profile に移った |
| `--dim 2:1`、`4:2` | `hero_pc[2]`（比較 2 つ）、`ys1_items[4]`・`ys2_items[4]`（比較 4 つずつ） |
| `--as 2:0x82:1`、`0x321:0x6321:1` | `equip.c` の表の位置 `2 * 0x2000 + 0x0321` を、バンク `$82` と論理番地 `$6321` から求める形にした |

- コードに新しく書いた数（一覧に出るだけ）：`0x1FFF` が 1 個（`equip.c` の `items & 0x1FFF`）。

### 11.3 確かめたこと

| 確かめたこと | 結果 |
|---|---|
| T1（11.2 の引数） | **OK** [C]。数字 123 個が、コードから消えて profile に入った（`--dim` 3、`--as` 2 を含む）。文字列は 0 |
| T1 がわざと変えた値を見つけるか | 見つけた [C]：`walker_hpmax` `0x28E6` → `0x28E7`、`tile_bar_red` 507 → 508、`crc` の最後の桁、`ys2_items` の 18 → 19、`intro_skip` の 4000 → 4001 の 5 つ（それぞれ差 2 件）。確かめたあと、ファイルは元に戻した（`cmp` で同じ） |
| `check_layers.py --pcerecomp external/PCERecomp` | **OK** [C] |
| ビルド（`scripts/build.sh`） | 警告 0 [C] |
| K4：装備の絵 | 新しい `pce_trace` で `tools/equip/equip_frames.py` を実行し直した `equip_frames.c` は、前のものとバイト単位で同じ（`cmp`、SHA-1 `4c4759d9…`）[C]。36 フレーム |
| 試験の名前と本数 | 前と同じ 100 本 [C] |
| `ctest -LE history -j 4 --repeat until-pass:2` | **90 本のうち 89 本合格、skip 1 本（`perf_ys`）、失敗 0**（1107 秒）[C]。やり直しになった試験はない。`app_prep_ys` は 1 回目で合格（142 秒）。実時間の試験も、しきい値を変えずに合格（K3） |
| history（`scripts/build.sh --test-hist`） | **10 本すべて合格**、失敗 0 [C]（`motion_cave_ys`、`cd_pop_ys`、`crop_flash_ys`、`motion_intro2_ys`、`motion_light_ys`、`motion_save_ys`、`motion_ys`、`motion_temple_ys`、`gpu_window_ys`、`item_window_ys`）。`-j` はコア数の半分で、試験の時間の合計は約 734 秒、経過は約 3 分 |

- 3A-4 で触れた値は、セッションの試験（`session_ys_*`）、HUD・枠の試験、装備の試験（`equip_*`）、イントロの試験（`intro_ys`、`app_tests` の
  `INTRO_MAX_FRAMES`）、ランチャーの試験（`launcher`、`launcher_ys`）、history の滑らかな動き・鉱山の明かり・ウィンドウの試験で通っている。
- 次に history を回すのは、3A-5 の後と 3A-7（3.2 のとおり）。
- Windows での確認は 3A-7 でまとめて行う（Q9）。

---

## 12. 3A-5 の結果（2026-11-28、macOS）

基準は `74019a8`（3A-4 のコミット）。Q2 のとおり、画面の幾何と配列の大きさは**コンパイル時の定数のまま**、`runtime/profile/ys_profile.h` の末尾の 1 か所に集めた。
値は北米版のもので、「両方の版で同じと仮定している [A]」と注を付けた（`01` の U12。日本版で違うと分かったら、そのときに profile の構造体へ移す）。

### 12.1 変えたもの

| ファイル | 内容 |
|---|---|
| `runtime/profile/ys_profile.h` | 定数 `YS_IMG_*`（枠の線 6、地図 4、パネルの位置と大きさ 4、ロード画面の線 2、ゲームの画面の最小の大きさ 2）、`YS_PANEL_ROWS`・`YS_PANEL_COLS`・`YS_BAR_FIRST_COL`、`YS_EQUIP_FRAMES`・`YS_EQUIP_W`・`YS_EQUIP_H`、`YS_MENU_FONT_TILES`。冒頭の注を「型と定数だけ」に直した |
| `runtime/app/crop.c` | 画像の寸法のマクロを `YS_IMG_*` と `CROP_*` の別名にした。`MAP_X/Y/W/H` と `PANEL_H` は `crop.h` の `CROP_MAP_*`・`CROP_PANEL_H` と同じ値を 2 度書いていたので、そちらの別名にした |
| `runtime/app/crop.h` | `CROP_PANEL_W/H`・`CROP_MAP_W/H/X/Y` を `YS_IMG_*` の別名にした（約 22 か所の使う側は変えない） |
| `runtime/app/ys_panel.h` | `YS_PANEL_ROWS/COLS`・`YS_BAR_FIRST_COL` を profile に移した。`YS_PANEL_X`（枠の左の列と同じと注にあった）と `YS_PANEL_Y`（パネルの 1 行目）は `YS_IMG_FRAME_LEFT`・`YS_IMG_PANEL_Y` から |
| `runtime/app/ys_scroll.h` | `YS_SCROLL_MAP_ROW`・`YS_SCROLL_LOW_ROW` を、注にあったとおり `YS_IMG_MAP_Y` と `YS_IMG_MAP_Y + YS_IMG_MAP_H` から（27 と 155 のまま） |
| `runtime/app/equip.h` | `EQUIP_FRAMES/W/H` を `YS_EQUIP_*` の別名にした |
| `runtime/app/menu_font.h` | `MENU_FONT_TILES` を `YS_MENU_FONT_TILES` の別名にした（`MENU_FONT_WORDS` とキャッシュのファイルの大きさはそこから） |
| `tools/trace/hud_old.c` | 写していた `FRAME_LEFT`・`FRAME_RIGHT`・`PANEL_Y`・`PANEL_H`・`PANEL_W` をなくし、`YS_IMG_*` を使う（2.5 の「画面の枠の幾何」の写し） |

- 名前は各モジュールの今のもの（`CROP_MAP_X`、`EQUIP_FRAMES` など）を別名として残したので、使う側（`motion.c`・`gpu_draw.c`・`battle.c`・`redraw.c`・`ys_windows.c`・`main.c`・`ys_trace_ext.c`・試験）は変えていない。
- コードに残したもの（PC の側の配置の決めごとで、ディスクの値ではない）：`crop.c` の `LOAD_MID_Y` 37・`FRAME_BELOW` 4、`crop.h` の `CROP_FRAME_VIEW_H` 213
  （枠の上の線 11 から、パネルの下 219 + `FRAME_BELOW` 4 = 223 まで）、`CROP_HUD_*`・`CROP_BAND_*` などの HUD の帯の大きさ。タイルの 8 ピクセル（ハードウェア）。
- `tests/app/app_tests.c` の数字を直に書いた幾何（2.7）は変えていない。移した後もこれらの試験が北米版の値を確かめる。

### 12.2 T1 の引数と理由

3A-5 の引数（基準 `74019a8`）：

| 引数 | 理由 |
|---|---|
| `--allow 8:2` | 枠の左の列 8 を 3 か所（`crop.c`、`hud_old.c`、`ys_panel.h` の `YS_PANEL_X`）に書いていた。profile には `YS_IMG_FRAME_LEFT` と `YS_BAR_FIRST_COL`（別の意味の 8）の 2 個 |
| `--allow 24:1`、`27:2`、`128:1`、`288:1` | 地図の位置と大きさを `crop.c` と `crop.h` に 2 度書いていたもの。27 はほかに `YS_SCROLL_MAP_ROW` |
| `--allow 155:1` | `YS_SCROLL_LOW_ROW` を `YS_IMG_MAP_Y + YS_IMG_MAP_H` にした |
| `--allow 41:2`、`179:2`、`320:1`、`327:1` | パネルと枠の寸法を `crop.c`・`crop.h`・`ys_panel.h`・`hud_old.c` に何度も書いていたもの |

### 12.3 確かめたこと

| 確かめたこと | 結果 |
|---|---|
| T1（12.2 の引数） | **OK** [C]。数字 25 個が、コードから消えて profile に入った |
| T1 がわざと変えた値を見つけるか | 見つけた [C]：`YS_IMG_PANEL_Y` 179 → 180、`YS_EQUIP_FRAMES` 36 → 37、`YS_IMG_MAP_H` 128 → 129 の 3 つ（それぞれ差 2 件）。確かめたあと、ファイルは元に戻した（`cmp` で同じ） |
| `check_layers.py --pcerecomp external/PCERecomp` | **OK** [C] |
| ビルド（`scripts/build.sh`） | 警告 0 [C] |
| 試験の名前と本数 | 前と同じ 100 本 [C] |
| `ctest -LE history -j 4 --repeat until-pass:2` | **90 本のうち 89 本合格、skip 1 本（`perf_ys`）、失敗 0**（1209 秒）[C]。やり直しになった試験はない。`app_prep_ys` は 1 回目で合格（139 秒）。実時間の試験も、しきい値を変えずに合格（K3） |
| history（`scripts/build.sh --test-hist`） | **10 本すべて合格**、失敗 0 [C] |

- 3A-5 で触れた定数は、枠と HUD の試験（`app_tests` の数字を直に書いた幾何、2.7）、`windows_crop_ys`、装備の試験（`equip_*`）、
  `hud_old.c` を使う `--crop-log` の試験（`save_window_ring_ys`、`crop_flash_ys`、`equipment_ys` ほか）、history の滑らかな動き・切り抜きの試験で通っている。
- 次に history を回すのは 3A-7（3.2 のとおり）。
- Windows での確認は 3A-7 でまとめて行う（Q9）。

---

## 13. 3A-6 の結果（2026-11-28、macOS）

基準は `8290eb3`（3A-5 のコミット）。Q4 のとおり、トレースの拡張も `g_ys_profile` を読み、`tools/trace/` の写し（2.5）をなくした。
トレースだけが読む値は `YsTraceProfile`（`g_ys_profile.trace`）に入れた。

### 13.1 変えたもの

| ファイル | 内容 |
|---|---|
| `runtime/profile/ys_profile.h`・`profile_usa.c` | `YsTraceProfile`（と `YS_TRACE_HOOK_*`）：トレースだけのフック 2 行（次のフレームを待つ終わり `$4B1B`、VBlank `$4AA3`）、組み立ての最初のキャラクター `$2E4A`、書いている部品のポインタ `$2010`、VBlank のフラグ `$2080` の bit `$02`、プレイヤーの H.P `$2C43`、パネルのタイルを文字にする 2 つの文字列。`YsProfile` に `trace` を足した |
| `tools/trace/ys_actors.c` | エンジンのフック 4 行（`ys_engine.c:16-19` とまったく同じ写し）をやめ、`g_ys_profile.engine->hooks` と `g_ys_profile.trace->hooks` を引く。表の順（START、ACTOR、PIECE、END、WAIT、VBLANK）は前と同じ。窓 2 の範囲 `0x4000`〜`0x6000`（`ys_actors.c:39`）は、2 つの表の最小値と最大値から計算する形にした（3A-4 の `ys_engine.c` と同じ）。キャラクターの表とリストのカメラは `g_ys_profile.engine` から |
| `tools/trace/ys_trace_ext.c` | ウィンドウの数 `$3E`（`windows->count`）、Adol のパレット 13 の 3 か所（`equip->adol_pal`。`0x8D` は「前に出る bit `0x80` | パレット」なので `0x80 | adol_pal`）、`--panel-log` の EXP・GOLD（`common`）、次のレベル・最大 H.P（`panel`）、H.P（`trace->hp`）、パネルの文字（`trace->label_chars`・`digit_chars`）、会話の速さ `$27F6`（`dialog->speed`）と比べる値 `$92`（`engine->unit` と同じ `$2092`。会話の命令列の `SBC $92`） |

- `$2241`（`ys_actors.c` の待ちのフック。System Card の作業域）と `$2216`（部品の数）はコードに残した（Q3）。
- 行の数の区切り（132、210、263）、スプライトの SAT の差 `0x58`・`0x20`、VCE の 1 行 1365 サイクル、`YS_SLOTS` 64 はハードウェアか解析の区切りなので、コードに残した。
- `hud_old.c` の写しは 3A-5 でなくした。2.5 の表の写しは、これで 7 行すべてなくなった。

### 13.2 T1 の引数と理由

3A-6 の引数（基準 `8290eb3`）：

| 引数 | 理由 |
|---|---|
| `--allow 0x4D58:1`、`0x4DAC:1`、`0x4E60:1`、`0x4DAA:1` と、シグネチャの 20 バイト（`0xA9`、`0x01`、`0x4D`、`0x15`、`0x22`、`0xBD:2`、`0x38:2`、`0x2F:2`、`0x89`、`0x08`、`0x8C`、`0x02`、`0x00`、`0x8D`、`0x03`、`0x58`、`0x60`） | `ys_actors.c` のエンジンのフック 4 行の写し。値は 3A-4 から `YsEngineProfile` にある |
| `--allow 0x8D` の 2 個目 | `ys_trace_ext.c` の `== 0x8D` を `0x80 | adol_pal` にした |
| `--allow 5:2` | 写しの表の `sig[5]` とループの上限 `k < 5`（`sizeof` にした） |
| `--allow 0x4000:1`、`0x6000:1` | 窓 2 の範囲を、表から計算する形にした |
| `--allow 0x2EF8:1`、`0x2F38:1`、`0x2F78:1`、`0x2FB8:1`、`0x2FF8:1`、`0x3038:1`、`0x2C33:1`、`0x2C34:1`、`0x2C35:1`、`0x2C36:1` | キャラクターの表とカメラの写し（`YsEngineProfile`） |
| `--allow 0x2011:1` | 部品のポインタの上のバイトを「`piece` + 1」にした |
| `--allow 13:2` | パレット 13 の写し（`YsEquipProfile.adol_pal`） |
| `--allow 0x3E:1`、`0x92:1`、`0x27F6:1` | ウィンドウの数・`$2092`・会話の速さの写し（`$2000` からの差で書いていたもの 2 つを含む） |
| `--allow 0xABB:1`、`0xABC:1`、`0xABD:1`、`0xC41:1`、`0xC42:1`、`0xC44:1`、`0xC48:1`、`0xC49:1`、`0xC4A:1`、`0xC4B:1`、`0xC4C:1` | `--panel-log` の EXP・GOLD・次のレベル・最大 H.P の写しと、H.P の上のバイト（`$2000` からの差で書いていた） |
| `--ram 0xC43:1` | H.P は profile に初めて入る値：差 `0xC43` を CPU の番地 `0x2C43` に直した |
| `--new 1:10` | `ys_actors.c` の 5（`h = 1`、`g_pc_range = 1`、`piece + 1`、カメラの `+ 1` 2 つ）と、`ys_trace_ext.c` の 5（EXP・次のレベル・H.P・最大 H.P・GOLD の `+ 1`） |
| `--new 2:2` | `ys_trace_ext.c` の EXP と次のレベルの `+ 2` |

- コードに新しく書いた数（一覧に出るだけ）：`0x2000` が 3 個（`--panel-log` の `R` マクロ、ウィンドウの数と `$2092` の `- 0x2000`）。
- 文字列 2 つ（`"H.PEXGOLDM"`、`"0123456789:/"`）は、コードから消えて、同じ文字列のまま profile に入った。

### 13.3 確かめたこと

| 確かめたこと | 結果 |
|---|---|
| T1（13.2 の引数） | **OK** [C]。数字 17 個と文字列 2 個が、コードから消えて profile に入った（`--ram` 1 を含む） |
| T1 がわざと変えた値を見つけるか | 見つけた [C]：待ちのフックの PC `0x4B1B` → `0x4B1C`、`hp` `0x2C43` → `0x2C44`、`label_chars` の最後の文字の 3 つ（それぞれ差 2 件）。確かめたあと、ファイルは元に戻した（`cmp` で同じ） |
| `check_layers.py --pcerecomp external/PCERecomp` | **OK** [C] |
| ビルド（`scripts/build.sh`） | 警告 0 [C] |
| K4：装備の絵 | 新しい `pce_trace` で作り直した `equip_frames.c` は、前のものとバイト単位で同じ（`cmp`、SHA-1 `4c4759d9…`、36 フレーム）[C] |
| 試験の名前と本数 | 前と同じ 100 本 [C] |
| `ctest -LE history -j 4 --repeat until-pass:2` | **90 本のうち 89 本合格、skip 1 本（`perf_ys`）、失敗 0**（1109 秒）[C]。やり直しになった試験はない。`app_prep_ys` は 1 回目で合格（142 秒）。実時間の試験も、しきい値を変えずに合格（K3） |

- 3A-6 で触れたのはトレースの道具だけ（`pce_trace`・`ys_trace`）。それを使う試験（`windows_ys`・`windows_ys1`・`panel_ys`・`dialog_ys`・
  `equip_*`・`session_ys_*` など）で通っている。history は 3.2 のとおり 3A-7 で回す（3A-6 はプレイヤーの実行ファイルの動作に触れない）。
- Windows での確認は 3A-7 でまとめて行う（Q9）。

---

## 14. 3A-7：まとめ（2026-11-28、macOS）

3A-1〜3A-6 のコミット：`6840786`（3A-1・3A-2）、`f2caa50`（3A-3）、`74019a8`（3A-4）、`8290eb3`（3A-5）、`e9a5a29`（3A-6）。
3A-7 はコードを変えない（文書だけ）。3A の前の基準は `3956404`（本書の計画のコミット）。

### 14.1 終了条件（1 章）

| # | 結果 | 判定 |
|---|---|---|
| E1 | `runtime/profile/` に `ys_profile.h` と `profile_usa.c`。`g_ys_profile` を定義するのは `profile_usa.c:292` だけ（`grep`）。`ys_profile_usa` をリンクする実行ファイルは `pce_runtime`・`pce_trace`・`app_tests`・`equip_check`・`ys_trace`・`YsRecomp` の 6 つで、それぞれ 1 回だけ（`CMakeLists.txt:113, 121, 130`、`cmake/ys_edition.cmake:189, 198, 207`）。ほかの profile はまだない | 合格 [C] |
| E2 | 段ごとの T1 は 6 回とも差 0（8.2、9.3、10.3、11.3、12.3、13.3）。3A の全体（`3956404` → `e9a5a29`）でも、段の引数を合わせ、14.3 の 2 つを足すと差 0 | 合格 [C] |
| E3 | macOS：段ごとに 100 本、`-LE history` で 89 本合格・`perf_ys` skip・失敗 0。history は 3A-4・3A-5・3A-7 で 10 本合格（14.5）。期待値は変えていない（`app_tests.c` は 3A-4 で表 1 つの `static` を外しただけ、11.1）。**Windows は 14.4 の手順で確かめる（保留：2026-11-28 の方針で、後日ユーザーが Windows の実機でまとめて行う）** | macOS は合格 [C]、Windows は保留 |
| E4 | `tools/check_layers.py --pcerecomp external/PCERecomp`（試験 `host_layers`）が段ごとに OK。規則 6（profile から `app/`・`host/`・SDL を include しない）と規則 7（版の分岐を書かない）を 3A-1 で足した | 合格 [C] |
| E5 | 規則 7 が `runtime/app/`・`main.c`・`tools/trace/` で版の分岐がないことを確かめている。3A で足したコードにも `#ifdef` や `edition_id` の比較はない | 合格 [C] |
| E6 | サブモジュールの指す先は `3956404` と同じ `76ab827`（`git ls-tree`）。`external/PCERecomp` の作業ツリーに変更はない | 合格 [C] |
| E7 | 14.2 の表 | 合格 [C] |

### 14.2 コードに残したディスク由来の値（E7）

日本版（3B 以降）で確かめるための一覧。「確かめ方」は、日本版のディスクと System Card 3.0 で何を見ればよいか。

| 種類 | 値と場所 | 残した理由 | 日本版での確かめ方 |
|---|---|---|---|
| System Card の作業域（Q3） | `$2216`（`ys_engine.c`、`tools/trace/ys_actors.c`）、`$220E/0F`・`$2212/13`（`ys_scroll.c`、`ys_panel.c`）、`$221F`・`$2220`・`$2222`（`ys_fade.c`）、`$22D0/D1`・`$22E1/E2`（`launcher_sound.c`）、`$2241`（`ys_actors.c`） | 番地はディスクではなく System Card 2.0 に由来する [C] | System Card 3.0 で同じ意味か（`04` の J10。`03` の 8.6 では、作業域を使う命令の数が一部違う） |
| エンジンの特性（Q8） | `YS_ACTORS` 64、`YS_WALKERS` 48、`YS_NO_ACTOR` 0xFF、状態 `0x50`（`ys_engine.h`）、`ys_masks.c`・`.h` の規則（フラグ `0x20`・`0x08`・`&0x27 == 0x22`、ポーズの範囲、足の位置 `y + 8 + 24`、`YS_MASK_H` 32）、`MOTION_STEP` 8（`motion.h`）、`motion.c`・`battle.c` のフラグ（2.3）、`RING` 256（`motion.c:15`、`gpu_draw.c:15`、`redraw.h`）、`RING_X` 512・`RING_Y` 256（`battle.c:16-17`）、マップ 64×32（`game_fixes.c`、`ys_windows.c` の `set_bat`・`ys_windows_cells`）、ウィンドウのスプライトのフラグ `0x20`（`ys_windows.c`）、`MENU_CHIP_RET` 0x3FFE（`menu_chip.c:7`） | 両作に共通のエンジンの性質と見ている [A] | Phase 2 の残りで、日本版のエンジンの表と命令を読む。違えば profile に移す |
| 画面の幾何と配列の大きさ（Q2） | `YS_IMG_*`、`YS_PANEL_ROWS/COLS`、`YS_BAR_FIRST_COL`、`YS_EQUIP_FRAMES/W/H`、`YS_MENU_FONT_TILES`（`ys_profile.h` の末尾、3A-5） | コンパイル時の定数。両方の版で同じと仮定 [A] | `01` の U12。日本版の画面（枠・パネル・ロード画面）を測る。違えば profile の構造体に移す |
| 配列の大きさ（ほか） | 色の RAM の大きさ `0x400`（`YsFade.ram[512]`）、前景の表 256（`YsFront`）、フェードの表 8×8 | 配列の大きさ。ゲームの表の形でもある [A] | 日本版のフェードの表と前景の表の大きさ |
| 文字とタイルの対応 | `menu_font.c:176-179`（`:` 10、`/` 11、A〜C 12、D〜Z 16 から） | 北米版のフォントの並び。日本版はフォントが違う見込み [A] | 3B のフォントの扱い（`04` の計画）で決める |
| 装備の絵 | `equip.c` のポーズ `$10`〜`$1F` と、絵から導いた値（約 50）、何もないを表す `0xFF` | 絵の側の値（`01` の 3.3 でも絵の側） | 日本版の Adol の絵とポーズ |
| 効果音を切り出す規則 | `launcher_sound.c` の `CAP_SFX_MAX` 240、`CAP_SFX_QUIET` 20、無音の判定 8、次の効果音までの 30 | 音を切り出す一般の規則で、ディスクの中身ではない [A] | 日本版の効果音の長さ |
| `--launcher-wav` の効果音の範囲 | `main.c:2370` の付近 | 開発用の道具（2.3） | 3B で道具を使うとき |
| ハードウェア | CD RAM のバンク `$80`〜`$87`、RAM のバンク `$F8`、スタックのページ `$100`、色のマスク `$1FF`、VCE の 512 色、SAT の差 `0x20`・`0x58`（32・88）、タイルの 8 ピクセル、1 行 1365 サイクル、1 フレーム 735 標本 | 版によらない [C] | 不要 |
| PC の側の決めごと | `crop.c` の `LOAD_MID_Y` 37・`FRAME_BELOW` 4、`CROP_FRAME_VIEW_H` 213、`CROP_HUD_*`・`CROP_BAND_*`、`tools/trace/ys_actors.c` の行の区切り 132・210・263 | ディスクの値ではない | 不要（日本版の幾何が違えば見直す） |
| ファイル名と作品名 | `menu_font.bin`・`launcher_sfx.bin`（`01` の P4）、`launcher.c:12-14` の作品名と発売年（`01` の P11） | 版で変わらないと決めてある | 不要 |

### 14.3 T1：3A の全体

段の引数（9.2 を `--new 1:7` に直したもの、10.2、11.2、12.2、13.2）を合わせて、基準 `3956404` で実行した：

- 差は 2 つだけ出た。どちらも 3A-1・3A-2 でコードに新しく書いた数で、その段では「一覧に出るだけ」だった。全体で数えると、
  あとの段で移した同じ数を隠すので、`--new` で数える：`--new 256:2`（`main.c` の `char title[256]` 2 つ）、`--new 0xFFFF:1`（`ys_reward.c` の `g_pc_min = 0xFFFF`）。
- これを足すと **OK** [C]。数字 521 個が、コードから消えて profile に入った（`--ram` 5、`--dim` 7、`--as` 2 を含む）。
  文字列 12 個がコードから消え、profile の 9 個の文字列と、それを入れる書式で元と同じになる。

### 14.4 Windows での 3A 全体の確認手順（Q9）

状態：**保留**（2026-11-28）。開発は macOS の実機で先に進め、この確認は後日ユーザーが Windows の実機で行う（CLAUDE.md の
「開発環境」）。3B 以降で確認待ちが増えても、before は `3956404` のまま。after に実行する日のブランチの先頭を使うと、3A より後の
差も含むので、そのことを結果に書く（3A だけを判定するなら、after を 3A の最後のコミットにして別に流す）。

M3 と同じ実機（`review_portability/11_m3_summary.md` の 5 章と同じ形）。`reference/` と `gg_trace.exe` は M3-8 で入れたものをそのまま使う。

- before：`3956404`（3A-1 の直前）。サブモジュールは `76ab827`。
- after：`feature/japanese-disc-support` の先頭（本書のコミットを含む）。サブモジュールは同じ `76ab827`。
- 所要時間：1 時間半ほど（生成 C を含むビルド 2 回、全試験 2 回、history 2 回）。

#### 14.4.1 before（`3956404`）

```powershell
cd C:\work\YsRecomp
$env:Path = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;" + $env:Path
git fetch; git checkout 3956404; git submodule update
git log --oneline -1; git -C external/PCERecomp log --oneline -1      # -> 3956404 / 76ab827
Remove-Item -Recurse -Force build
.\build.ps1 *> p3a_before_build.txt
ctest --test-dir build -C Release -N | Select-String "Total Tests"     # -> 100
ctest --test-dir build -C Release -N | Select-String "Test +#" | ForEach-Object { ($_ -split '\s+')[3] } | Set-Content p3a_before_names.txt
ctest --test-dir build -C Release -LE history -j 2 --repeat until-pass:2 *> p3a_before_testall.txt
ctest --test-dir build -C Release -L history *> p3a_before_hist.txt
.\build.ps1 -Package *> p3a_before_package.txt
python -c "import zipfile;[print(i.filename,i.file_size) for i in zipfile.ZipFile('dist/YsRecomp.zip').infolist()]" *> p3a_before_zip.txt
```

#### 14.4.2 after（ブランチの先頭）

```powershell
git checkout feature/japanese-disc-support; git pull
git submodule sync; git submodule update
git log --oneline -1; git -C external/PCERecomp log --oneline -1      # -> the branch tip / 76ab827
Remove-Item -Recurse -Force build
.\build.ps1 *> p3a_after_build.txt
ctest --test-dir build -C Release -N | Select-String "Total Tests"     # -> 100
ctest --test-dir build -C Release -N | Select-String "Test +#" | ForEach-Object { ($_ -split '\s+')[3] } | Set-Content p3a_after_names.txt
Compare-Object (Get-Content p3a_before_names.txt) (Get-Content p3a_after_names.txt)
ctest --test-dir build -C Release -LE history -j 2 --repeat until-pass:2 *> p3a_after_testall.txt
ctest --test-dir build -C Release -L history *> p3a_after_hist.txt
.\build.ps1 -Package *> p3a_after_package.txt
python -c "import zipfile;[print(i.filename,i.file_size) for i in zipfile.ZipFile('dist/YsRecomp.zip').infolist()]" *> p3a_after_zip.txt
Compare-Object (Get-Content p3a_before_zip.txt) (Get-Content p3a_after_zip.txt)
python tools\profile_values.py 6840786 --allow 1:2 --allow 3:3 --allow 4:1 --allow 0x30:1 --allow 0x80:1 --allow 0x2C2F:1 --allow 0x2C30:2 --allow 0x2C42:1 --allow 0x2ABC:1 --allow 0x2ABD:1 --allow 0x4AD8:1 --allow 0x4B05:1 --allow 0x707C:1 --allow 0x7998:1 --new 0:3 --new 1:7 --new 2:1 *> p3a_after_t1.txt
git status --short
```

- 最後から 2 行目は、T1 の道具が Windows の Python でも動くことの確かめ（W8）。基準 `6840786` は 3A-1・3A-2 のコミットで、
  その後の段の値を数えるので、**差が出るのが正しい**（`DIFFERENCE` の行が出て、終わりが `profile_values: N difference(s)`）。
  見るのは、道具が最後まで動いて `profile_values:` の行を 2 つ出すことだけ。

#### 14.4.3 まとめ（貼るためのファイル 2 つ）

```powershell
Select-String -Path p3a_*.txt -Pattern "warning|tests passed|\*\*\*Failed|Not Run|Skipped|Total Tests|Total Test time|missing in the|Package|Zip|Built|profile_values:" |
  ForEach-Object { "$($_.Filename): $($_.Line.Trim())" } | Set-Content -Encoding utf8 p3a_results.txt
Select-String -Path p3a_after_testall.txt, p3a_after_hist.txt -Pattern "Test +#\d+:" |
  ForEach-Object { $_.Line.Trim() } | Set-Content -Encoding utf8 p3a_after_tests.txt
```

#### 14.4.4 チェックポイント

| ID | 確かめること | 元 |
|---|---|---|
| W1 | before・after とも `Built.`。`Total Tests` がどちらも 100。名前の `Compare-Object` は何も出ない | E3 |
| W2 | `p3a_after_build.txt` に `warning` の行がない（MSVC は Apple Clang と警告が違うので、profile を読む新しいコードの型の変換が出ないか） | E3 |
| W3 | history 以外：before・after とも 90 本で、skip は `perf_ys` の 1 本だけ、失敗 0（`session_ys_*`・`windows_*`・`equip_*`・`intro_ys`・`dialog_ys`・`panel_ys`・`launcher_ys`・`host_layers` が合格） | E3、E4 |
| W4 | `app_prep_ys` が合格。`***Failed` の行があれば（2 回目で合格していても）、その行と `missing in the` の後の記録を貼る | K5 |
| W5 | history：before・after とも 10 本合格 | E3 |
| W6 | `app_package_ys`・`app_launcher_ys` が合格。zip の `Compare-Object` は何も出ないか、`YsRecomp.exe` の大きさの行だけ | E3 |
| W7 | `git status --short` に、`??` のログ以外が出ない | E6 |
| W8 | `p3a_after_t1.txt` に `profile_values: base 6840786` と `profile_values: ... difference(s)` の 2 行がある（Python の例外が出ていない） | T1 |

#### 14.4.5 貼っていただくもの

`p3a_results.txt` の中身、`p3a_after_tests.txt` の中身、2 つの `Compare-Object` の出力、`git status --short` の出力。

### 14.5 macOS で確かめたこと（3A-7）

| 確かめたこと | 結果 |
|---|---|
| コード | 3A-6 のコミット `e9a5a29` から変えていない（3A-7 は `review_jp/05`・`01`、CLAUDE.md だけ） |
| T1（3A の全体、14.3） | **OK** [C] |
| `check_layers.py --pcerecomp external/PCERecomp` | **OK** [C] |
| ビルド（`scripts/build.sh`） | 警告 0 [C] |
| `ctest -LE history -j 4 --repeat until-pass:2` | **90 本のうち 89 本合格、skip 1 本（`perf_ys`）、失敗 0**（1309 秒）[C]。やり直しになった試験はない。`app_prep_ys` は 1 回目で合格（139 秒） |
| history（`scripts/build.sh --test-hist`） | **10 本すべて合格**、失敗 0 [C] |

- `01_profile_design.md` の 3.3 の表を 2 行直した：`YsLauncherSfxProfile` から System Card の作業域の RAM を外した（Q3 の決定。`$20E7` だけ入れた）。
  `YsCommonRam`（3A-4 で足した）の行を足した。
- 3B に残るもの（7 章のとおり）：`profile_jp.c`、`profile_report.c`、NULL による機能の無効化、`YsIandII_JP`、BIOS の流れ、`edition_id`。
  `04` の「`05_jp_disabled_features.md`」は `06` に直す（本書が 05 を使うため。冒頭の注）。
