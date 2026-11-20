# Phase 3B 計画：日本版の骨格（`YsIandII_JP`、実 BIOS 必須の流れ、機能はすべて無効）

- 作成日：2026-11-20。対象ブランチ：`feature/japanese-disc-support`（`910d89c`、PCERecomp はフォークの `2e86f2c`）
- **本書は計画である。** コード・CMake・試験は変更していない。実装は、5 章の質問への回答とユーザーの承認を得てから行う。
- 元にした文書：`01_profile_design.md`（profile 層、決定 P1〜P13）、`02_build_layout.md`（6 章の段階表、4.3 の試験）、
  `03_disc_analysis.md`（日本版の値、12 章の R2-1〜R2-7）、`review_portability/05_host_layer_design.md`（3 章の app_id、5 章の `.rc`）、
  `review_portability/09_m2_rom_paths_editions.md`（M2-3、M2-4）、`review_portability/06_m1_host_skeleton.md`（M1-5）。
- 表記：[C] 確認済み、[A] 推測、[U] 不明。**日本版の値は本書に書かない**（`03` の章を参照する）。

---

## 1. 目的と終了条件

`02` の 6 章の定義：**`profile_jp.c`（identity・BIOS の方針のみ、機能はすべて NULL）、`YsIandII_JP`、BIOS 必須の流れ、
無効な機能のログとメニュー、ROM 不要の試験**。

終了条件（すべて満たしたら止まる）：

| # | 条件 |
|---|---|
| E1 | 日本版の実行ファイル `YsIandII_JP` が macOS と Windows の両方でビルドできる（ROM がない構成では定義されない） |
| E2 | `YsIandII_JP` は起動時に、版が `jp` であることと、無効な機能を 1 行ずつログに出す。オプションメニューでは、それらの項目が灰色になる |
| E3 | 日本版の起動には実 BIOS が要る：ini の `bios_file`。なければファイル選択。BIOS がないときは、はっきりした文で止まる |
| E4 | 日本版のディスクの扱いは、5 章 Q1 の決定どおり（受け付ける／明示の文で拒否する）。北米版の流れは何も変わらない |
| E5 | 北米版の試験：本数、名前、合格と skip の件数が 3B の前と同じ（macOS と Windows） |
| E6 | 日本版の ROM 不要の試験（4 章 3B-7）が合格する。ROM がある環境では、起動の煙試験（3B-0）が合格する |
| E7 | 無効な機能の表（`review_jp/05_jp_disabled_features.md`、新設）が、ログの出力と一致する |
| E8 | PCERecomp に Ys のものが入っていない（上流の規則）。版の分岐が `runtime/app`・`main.c` に散らばっていない（作業規則 v2 の 8） |

**3B でしないこと**（3C・3D の範囲）：
- `games/ys_jp/disc.toml` と生成 C（再コンパイル）
- フック・RAM・装備の値を入れて機能を有効にすること
- 日本版のセッション、カバレッジ、Geargrafx との比較

---

## 2. 前提と今の状態

| 項目 | 状態 | 3B への影響 |
|---|---|---|
| M2（北米版だけの `ys_edition()`） | macOS で確認済み。Windows の確認（M2-1、M2-2、W1〜W8）は**待ち** | 3B の CMake は M2 の形の上に足す。先に Windows の確認を終えたい |
| M3（macOS で北米版のゲーム本体をビルド・試験） | **未着手** | フェーズ順（CLAUDE.md）では、M3 → 3A → 3B。`YsIandII_JP` を macOS で動かす（E1）には、M3 で `main.c` と SDL の macOS ビルドが通っている必要がある |
| Phase 3A（profile の骨格と北米版の値の移行） | **未着手**（`runtime/profile/` はまだない [C]） | 3B は 3A の `ys_profile.h`・`profile_usa.c`・`profile_report.c` の上に `profile_jp.c` を足す。**3A なしでは 3B の主要部は作れない** |
| `pce_trace` の macOS ビルド | できる [C]（M2） | R2-1（System Card 3.0 の起動）は M3 を待たずに確かめられる（3B-0） |
| `roms/jp`、`roms/bios/syscard3.pce` | 置いてある [C]（`03` の 3.1・8.1） | 日本版の ROM がある試験を macOS で動かせる |
| `main.c` の `host_paths_init("YsRecomp")` | 「版の app id は profile から（Phase 3B）」のコメントあり [C]（`runtime/main.c:2547`） | 3B-4 で profile の値に替える |

→ **順序の案**：
- 3B-0（R2-1 の確認）と 3B-1（CMake・ツールの版の受け皿）は、M3 と 3A の前にも進められる。
- 3B-2 以降は、3A の後に行う。

順序は 5 章 Q7 で決めていただきます。

---

## 3. 作るもの（設計は `01`・`02` のとおり。ここでは場所だけ）

| 区分 | ファイル・ターゲット | 内容 |
|---|---|---|
| profile | `runtime/profile/profile_jp.c`（新規） | `identity`（`edition_id "jp"`、`app_id`・ini 名 `YsIandII_JP`、タイトル `Ys I & II (Japan)`、英語の文言）。`bios`：`required = 1`。`disc`：Q1 による。ほかのサブ構造体はすべて NULL |
| CMake | `cmake/ys_edition.cmake` | `ys_rom_file(YS_JP_CUE …)`、`ys_rom_file(YS_JP_BIOS …)`（既定のファイル名は Q3）。`ys_edition(ID jp …)` の呼び出し。`disc.toml` がない版は生成 C を作らない（`02` の 2.2 の `EXISTS GAME_DIR/disc.toml`）。試験名の接尾辞（Q2）。`PROFILE`・`EQUIP none`・`EXE`・`TRACE`・`PTRACE` の引数（M2-3） |
| 実行ファイル | `YsIandII_JP` | `runtime/main.c` ＋ `pce_app` ＋ `ys_profile_jp` ＋ `equip_none.c`。生成 C がないので、ゲームのコードは**インタプリタで動く**（Q6） |
| 資源 | `runtime/host/win/app.rc.in`（`ysrecomp.rc` から）、`Info.plist.in`（macOS、M3 で作る形に合わせる） | 製品名と実行ファイル名を `configure_file` で版ごとに入れる（M1-5）。アイコンは Q4 |
| BIOS の流れ | `runtime/main.c` | `--bios` と「プレイヤーの流れ」の結び付きを切り離す（`01` の 6.2）。ini の `bios_file`、`SDL_ShowOpenFileDialog`。照合する値は Q5。北米版の動作（BIOS なしが既定）は変えない |
| ログとメニュー | `profile_report.c`（3A で作る）、`runtime/app/menu.c` | 無効な機能を 1 行ずつ。メニューの該当項目を灰色にして選べなくする（`menu.c:530` に灰色の行の描き方がある [C]） |
| セッション | `runtime/main.c:487-488` | ヘッダーに `# edition: <id>`。再生で版が違えば拒否（ツール側の `editions.session_edition` は M2 で済み [C]） |
| ツール | `tools/editions.py` に `jp` を足す。`build.ps1`・`scripts/build.sh` の `PLANNED` から `jp` を外す。M2-4 の 6 本（`equip_preview.py`、`equip_ice_preview.py`、`run_app_start_test.py`、`run_launcher_test.py`、`run_package_test.py`、`make_icon.py`）に版の引数 | 既定は `usa` のまま（今のコマンドがそのまま動く） |
| パッケージ | `make_package.py --edition jp`、`package/README_jp.txt` | `dist/YsIandII_JP/`、`.zip`。README に「System Card が必要」（P1） |
| 文書 | `review_jp/05_jp_disabled_features.md`（新規）、CLAUDE.md の版の節 | 無効な機能の表（E7） |

---

## 4. 段階（各段の終わりに試験と Windows の確認手順を付けて止まる）

| 段 | 内容 | 試験（macOS） | Windows | 前提 |
|---|---|---|---|---|
| **3B-0** | **R2-1：PCERecomp で System Card 3.0 と日本版のディスクが起動するか**。`pce_trace --frame --bios syscard3.pce <JP cue> N` で数か所の画像と、`FAILED` の行や未実装の CD の行が出ないことを見る。BIOS のタイトル画面からゲームのタイトルまで。U11（`region_japan` は 0 のまま）も同時に見る。PCERecomp の変更が要るとわかったら、**変えずに止めて報告** | 煙試験 `jp_boot_ysjp`（案）：決めたフレームで止まらずに走り、BIOS の画面とゲームの画面が出る（画像の要約値ではなく、`FAILED` の有無と VDC の表示状態で判定。期待値の決め方は 3B-0 の結果を見て提案する） | 同じ試験 | `pce_trace` のみ（M3 不要） |
| 3B-1 | CMake とツールの受け皿：`YS_JP_CUE`・`YS_JP_BIOS`、`ys_edition(ID jp …)`（disc.toml がないので生成 C と多くの試験は定義されない）、ラベル `jp`、`editions.py` の `jp`、`build.*` の `jp` を使えるようにする | 北米版の試験の定義が前と同じ（M2 と同じ比べ方）。`-L jp` の試験は 3B-0 の煙試験だけ | `-TestAll` の本数と結果が前と同じ | M2 の Windows の確認 |
| 3B-2 | `profile_jp.c`（identity・BIOS・NULL）と `ys_profile_jp`。依存の規則の試験（`01` の 9 章の grep）を `host_layers` と同じ形で quick に足す | 規則の試験。profile が 2 つ混ざるとリンクが失敗すること（意図した失敗）を一度だけ手で確かめる | 同じ | 3A |
| 3B-3 | `YsIandII_JP` のターゲット（インタプリタのみ）、`.rc` のテンプレート、`host_paths_init` に profile の `app_id` | `YsIandII_JP --version`（案）が `edition jp` を出す試験 | `.rc` の確認（`tools/ci/check_rc.py`）とアイコンの埋め込み | 3B-2、M3（macOS で `main.c` がビルドできること） |
| 3B-4 | 起動ログ（`ys_profile_report`）とメニューの灰色 | 無効な機能の一覧がログに出る試験（ROM 不要）。メニューは `app_tests` の形で、項目が選べないことを確かめる | 同じ | 3B-3 |
| 3B-5 | BIOS 必須の流れ（ini の `bios_file`、ファイル選択、BIOS がないときの文） | 合成の小さな CUE（ROM 由来のデータなし）と、BIOS なしの ini で、決めた文で止まる試験 | 同じ。ダイアログは手で 1 回 | 3B-3 |
| 3B-6 | ディスクの扱い（Q1）とセッションの版（`# edition:`、違う版の拒否） | 合成 CUE で拒否の文。版の違うセッションで拒否 | 同じ | 3B-5 |
| 3B-7 | M2-4 のツールの引数、パッケージ（`--edition jp`、README_jp）、`build.* --edition jp` | `--help` と引数の試験。パッケージは Windows のみ（macOS の `.app` は M3 の形に合わせる） | `-Package -Edition jp`、`app_package_ysjp`（案） | 3B-3 |
| 3B-8 | まとめ：E1〜E8 の確認、`05_jp_disabled_features.md`、CLAUDE.md の更新、Windows の確認手順（before／after） | 全試験 | 全試験と `-Edition` の 3 通り | すべて |

- 日本版の試験の名前は Q2 の接尾辞で作る。北米版の試験の名前と期待値は変えない（作業規則 v2 の 6）。
- 試験の期待値は、日本版を実際に動かして得た値だけを使う。何を確かめている値かも一緒に記録する。北米版の値は流用しない。

---

## 5. 始める前に決めていただくこと

| ID | 質問 | 案（推奨） |
|---|---|---|
| Q1 | 日本版のディスクの識別値（`03` の 3.2 で確認済み）を **3B で `profile_jp.c` の `disc` に入れる**か、3C まで NULL（明示の文で拒否）にするか | **3B で入れる**。値は [C] で、3B-5 の BIOS の流れを本物のディスクで端から端まで確かめられる。入れないと `YsIandII_JP` はディスクを受け付けず、E3 を合成のデータでしか確かめられない |
| Q2 | 日本版の試験名の接尾辞（`02` の B2） | `_ysjp`（例 `jp_boot_ysjp`） |
| Q3 | `YS_JP_CUE`・`YS_JP_BIOS` の既定のファイル名 | `roms/jp/Ys I & II (Japan).cue`、`roms/bios/syscard3.pce`（今の置き場所 [C]。上流の場所はないので `roms/<版>/` だけを見る） |
| Q4 | 版ごとにアイコンの絵柄を分けるか（`05` の H4） | 3B では北米版と同じアイコン。分けるなら絵はユーザーが用意する |
| Q5 | 日本版の BIOS を照合するか（`YsBiosPolicy` の識別値） | 3.0 の CRC（`03` の 8.1）が違うときは**警告を出して続ける**（拒否はしない。ほかの吸い出しや版の 3.0 で動く可能性を残す）。照合しなかったことはログに出す |
| Q6 | 3B の `YsIandII_JP` を、生成 C なし（インタプリタのみ）でビルドしてよいか | **はい**。3C で disc.toml と生成 C ができたら、北米版と同じ形（再コンパイル）に替える。インタプリタで動くことは、3C のロックステップの基準にもなる [A] |
| Q7 | 順序：3B-0・3B-1 を M3・3A の前に行うか、フェーズ順どおり M3 → 3A → 3B にするか | **3B-0 だけを先に行う**（R2-1 は日本版の計画全体の前提で、PCERecomp の変更が要るかどうかを早く知りたい）。3B-1 以降はフェーズ順どおり |
| Q8 | `pce_runtime`（汎用の実行ファイル）にどの profile をリンクするか（`02` の B5） | 3A で決める（3B には影響しない）。3A の計画で提案する |

---

## 6. 危険と、起きたときの扱い

| ID | 危険 | 扱い |
|---|---|---|
| K1 | System Card 3.0 が PCERecomp で起動しない（追加 RAM、未実装の CD コマンド）[U] | 3B-0 で最初に確かめる。PCERecomp の変更が要るなら、変更せずに止めて報告する（変える場合はフォーク `DCTomoMD/PCERecomp` にブランチを作る） |
| K2 | インタプリタのみの `YsIandII_JP` が遅く、実時間で動かない [A] | 3B の試験は実時間に頼らない形にする（`--frame` の画像、ログ）。実時間で遊べるのは 3C 以降 |
| K3 | `main.c` の BIOS の流れの変更が北米版を変える | 北米版の `-TestAll`（Windows）と macOS の全試験で、前後の本数と結果が同じことを各段で確かめる（E5） |
| K4 | 版の分岐が `if (japanese)` の形で入り込む | 3B-2 の依存の規則の試験（grep）で止める |
| K5 | ROM・BIOS 由来のデータが試験や Git に入る（漢字フォントは BIOS 由来） | 合成 CUE は自分で作る。画像の期待値はハッシュではなく、状態で判定する。`games/ys_jp/` には 3B では何も入れない |
| K6 | ランチャーの効果音の取り出しと、ゲームのフォントによるメニューが、実 BIOS では動かない（`01` の 6.3） | 3B では無効のまま。ログと表に残す |

---

## 7. Windows で確かめること（各段の手順は段ごとに 09 と同じ形で書く）

- 北米版の 99 本：本数、名前、合格と skip が前と同じ（before は 3B を始める直前のコミット）
- `YsIandII_JP.exe` のビルド。`.rc` のテンプレートから作った製品名とアイコンの埋め込み
- `-Edition jp`・`-Edition usa`・`-Edition all` で選ばれる試験
- `-Package -Edition jp`：`dist\YsIandII_JP\`、`dist\YsIandII_JP.zip`
- BIOS のファイル選択のダイアログ（手で 1 回）

---

## 8. 3B で閉じる未解決の項目

| ID | 内容 | 3B の段 |
|---|---|---|
| R2-1 | System Card 3.0 の起動 | 3B-0 |
| U11 | `region_japan` の要否（起動試験で 0 のまま動くか） | 3B-0 |
| M2-3 | 日本版の変数、試験名・ターゲット名の引数化、`profile`・`EQUIP` の引数、`.rc`・パッケージの版ごとの名前 | 3B-1、3B-3、3B-7 |
| M2-4 | ツール 6 本の版の引数 | 3B-7 |
| M1-5 | `.rc` を版ごとのテンプレートにする | 3B-3 |
| B2 | 日本版の試験名の接尾辞 | Q2 |
| H4 | 版ごとのアイコン | Q4 |

3C 以降に残るもの：R2-2〜R2-7、U6・U7・U12・U13 の残り、`disc.toml` と生成 C、機能の有効化、日本版のセッション。
