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

**2026-11-20：ユーザーの回答は「全部推奨で」。Q1〜Q8 はすべて「案（推奨）」の列のとおりに決定。**

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

---

## 9. 3B-0 の結果（2026-11-20、macOS arm64、`pce_trace` のみ）

実行したこと（出力の画像と状態のファイルは作業用フォルダに置き、Git には入れていない）：

```
build/Release/pce_trace --frame --bios roms/bios/syscard3.pce "roms/jp/Ys I & II (Japan).cue" 3600 ... --every 120 ... --hashes ...
build/Release/pce_trace --frame --input tests/input/ys_game.txt --bios roms/bios/syscard3.pce "roms/jp/Ys I & II (Japan).cue" 9000 ... --every 300 ... --hashes ...
build/Release/pce_trace --frame --input tests/input/ys_game.txt --bios roms/bios/syscard3.pce "roms/jp/Ys I & II (Japan).cue" 6000 ... --dump-state ...
```

| 見たこと | 結果 | 状態 |
|---|---|---|
| ボタンなし 3600 フレーム | 終了コード 0。System Card 3.0 のタイトル画面（`VER. 3.00`）が出て、RUN を待つ点滅を 32 フレームごとに繰り返す | [C] |
| RUN を押す（北米版の試験と同じボタンの台本 `tests/input/ys_game.txt`）9000 フレーム | 終了コード 0、`FAILED` の行なし、CD の音楽 35.4 秒・ADPCM 3.9 秒。著作権の画面 → タイトル画面（Beginning / Continue）→ 新しいゲーム → 船の上の場面に進み、仮名と漢字の会話の窓が出る（BIOS のフォントで描かれる） | [C] |
| 6000 フレームの RAM | 日本版の GOLD の番地（`03` の 6.2 の [C]）に 1000、EXP の番地に 0。画面の `GOLD 01000`、`EXP 00000` と合う | [C]（番地の意味を実行で確かめた） |
| R2-1：PCERecomp で System Card 3.0 が起動するか | **起動する。PCERecomp の変更は不要**（少なくともゲームの最初の場面まで） | [C] |
| U11：`region_japan` | 0 のまま（どこからも設定されない）でゲームまで進む | [C]（この範囲で） |

**煙試験の案（`jp_boot_ysjp`。3B-1 で CMake の `YS_JP_CUE`・`YS_JP_BIOS` と一緒に作る。承認を得てから）**：
- 実行：`pce_trace --frame --input <ボタンの台本> --bios ${YS_JP_BIOS} ${YS_JP_CUE} 6000 <画像> --dump-state <状態>`
- 合格の条件（小さな Python スクリプトで判定。画像の要約値は使わない、6 章 K5）：
  1. 終了コード 0、`FAILED` の行がない
  2. `disc music played` が 0 秒より長い
  3. 状態の RAM で、日本版の GOLD の番地が 1000（新しいゲームの所持金。画面の表示と合うことを上で確かめた）
- ボタンの台本：`tests/input/ys_game.txt` はボタンだけの北米版のファイルなので、日本版用に同じ中身の
  `tests/input/ys_jp_boot.txt`（案）を作り、日本版で確かめたことをコメントに書く
- ROM がないときは 77（skip）。ラベル `jp`、`console`

---

## 10. 3B-1 の結果（2026-11-28、macOS arm64）：CMake とツールの受け皿

before（基準）は `8a1033d`。PCERecomp（`external/PCERecomp`、`76ab827`）は変更していない。roms/ は読み取りのみ。
表記：[C] macOS で実行して確かめた、[W] Windows の実機での確認待ち、[A] 推測。

### 10.1 変更の内容

| ファイル | 変更 |
|---|---|
| `CMakeLists.txt` | `ys_rom_file(YS_JP_CUE "Ys I & II (Japan).cue" jp … SUBFOLDER_ONLY)`、`ys_rom_file(YS_JP_BIOS "syscard3.pce" bios … SUBFOLDER_ONLY)`（Q3）。`ys_edition(ID jp … GAME_DIR games/ys_jp SUFFIX _ysjp BOOT_INPUT tests/input/ys_jp_boot.txt BOOT_FRAMES 6000 BOOT_RAM16 0x2C40=1000)`。`TESTS_CONSOLE` に `jp_boot_ysjp` |
| `cmake/ys_edition.cmake` | `ys_rom_file` に任意の引数 `SUBFOLDER_ONLY`（`roms/<フォルダ>/` だけを見る。北米版の呼び出しと表示は同じ）。`ys_edition` に `SUFFIX`・`BOOT_INPUT`・`BOOT_FRAMES`・`BOOT_RAM16`。`BOOT_INPUT` があれば煙試験 `<ID>_boot<SUFFIX>` を定義する。`GAME_DIR/disc.toml` がない版は、そこでラベルを付けて終わる（生成 C、実行ファイル、ほかの試験は定義されない）。ラベルを付ける部分は小さな関数 `ys_edition_label` にした（2 か所から呼ぶため）。`usa` 以外で `disc.toml` があると構成エラー（その下のターゲット名・試験名がまだ北米版のものなので。名前の引数化は 3B-3 以降）。北米版の本体（旧 48 行目より下）は 1 行も変えていない |
| `tools/trace/run_boot_test.py`（新規） | 煙試験。`pce_trace --frame --input <台本> --bios <BIOS> <cue> <フレーム> <画像> --dump-state <状態>` を動かし、(1) 終了コード 0 で `FAILED` の行がない、(2) `disc music played` が 0 秒より長い、(3) `--ram16 番地=値` ごとに、最後の状態の作業 RAM の 16 ビットの値が合う、を確かめる。ディスクか BIOS がなければ 77。画像は書くが判定に使わない（K5）。版を知らない（番地と値は呼び出す側が渡す） |
| `tests/input/ys_jp_boot.txt`（新規） | ボタンだけの台本（`ys_game.txt` と同じ押し方。`diff` で同じことを確かめた）。日本版で確かめたことをコメントに書いた（9 章） |
| `tools/editions.py` | `jp` を足した：`YsIandII_JP`・`ys_trace_jp`・`pce_trace_jp`・`games/ys_jp`（`02` の 2.2 の名前）、`YS_JP_CUE`・`YS_JP_BIOS`。`upstream_place=False`（CMake の `SUBFOLDER_ONLY` と同じ規則）。`PLANNED` は空にした |
| `build.ps1`、`scripts/build.sh` | `jp` を選べる（`available` に `jp`）。パッケージを作れる版は別の一覧（`packaged` = `usa`）：`-Package -Edition jp` は `Edition 'jp' has no player package yet (Phase 3B)` で止まり、`all` は北米版だけを作る |
| `CLAUDE.md` | 「roms/ の配置」と macOS の節の版の説明 |

決めたこと（質問せずに、文書の案のとおりにしたもの）：

- 煙試験の合格の条件は 9 章の案の 3 つ。GOLD の番地 `$2C40`（`03` の 6.2、[C]）と値 1000（9 章、[C]）は CMake の
  `ys_edition(ID jp …)` の引数に置いた。`profile_jp.c` はまだなく（3B-2）、3B では `reward` が NULL なので、試験の値の置き場は
  ここだけ。北米版の番地（`$2C41`）は使っていない。
- 煙試験は北米版と同じ `pce_trace` を使う（`ys_profile_usa` がリンクされている）。Ys のオプションを付けないので、拡張は
  エンジンのフックを入れない（`ys_trace_ext.c` の `ys_setup`：`on_instruction` を入れるのはオプションがあるときだけ [C]）。
  北米版の profile の値は読まれない。`pce_trace_jp` は profile ができてから（3B-2 以降）。
- ツール（`run_bios_check`・`check_all_sessions`・`run_perf_check`・`check_session`・`make_package`）は `--edition jp` を
  受け付けるが、日本版のセッション・実行ファイルがまだないので、ないものを言って終わる（10.3）。

### 10.2 北米版の試験の定義が前と同じこと（macOS、[C]）

M2（`review_portability/09` の 3 章）と同じく、CMake が書き出した `CTestTestfile.cmake` を比べた。今回は同じフォルダで
構成し直したので、そろえたのは CMake の行番号（`_BACKTRACE_TRIPLES`）だけ（別のフォルダで構成した場合は、ビルドのフォルダも）。

| 構成 | before（`8a1033d`） | after | 差 |
|---|---|---|---|
| 北米版あり・日本版あり（開発機の `build/`） | 100 本 | 101 本 | 増えたのは `jp_boot_ysjp` だけ。before にだけある行 0。after にだけある 12 行は、すべて `jp_boot_ysjp` の `add_test`・`set_tests_properties` と、それを囲む `if`〜`endif` |
| 北米版あり・日本版なし（`-DYS_JP_CUE=/nonexistent/…`） | 100 本 | 101 本 | 同じ（`jp_boot_ysjp` は定義され、実行すると 77） |
| 北米版なし・日本版あり（`-DYS_USA_CUE=/nonexistent/…`） | 33 本 | 34 本 | 同じ |
| 両方なし | 33 本 | 34 本 | 同じ |

- **北米版と共通の 100 本は、名前・順番・コマンド・属性（SKIP_RETURN_CODE、PASS/FAIL_REGULAR_EXPRESSION、TIMEOUT、
  RUN_SERIAL、ENVIRONMENT、ラベル）が 1 行も変わっていない。**
- ラベルの本数：`usa` 81、`jp` 1、`quick` 22、`console` 17 → 18、`recomp` 49、`history` 10、`package` 2。
- 構成の表示：`YS_JP_CUE: …/roms/jp/Ys I & II (Japan).cue (SHA-1 bdb25f76…)`、`YS_JP_BIOS: …/roms/bios/syscard3.pce
  (SHA-1 79f5ff55…)`（`03` の 8.1 の値と同じ）、`Edition jp: no disc map (…/games/ys_jp/disc.toml); only its boot test is defined`。

### 10.3 macOS で確かめたこと

| 確かめたこと | 結果 |
|---|---|
| ビルド（`scripts/build.sh`） | 警告 0、`Built.` [C] |
| `tools/check_layers.py --pcerecomp external/PCERecomp` | **OK** [C] |
| `jp_boot_ysjp` | 合格（7.4 秒）。`CD: disc music played 35.4 s; ADPCM played 3.9 s`、`RAM $2C40 (16 bits): 1000, expected 1000: ok` [C] |
| 煙試験がわざと違う値で落ちること | `--ram16 0x2C40=999` → `DIFFERENT`、失敗。北米版の番地 `0x2C41=1000` → 5123 で失敗。ボタンなしの 1200 フレーム → `the CD music did not play` で失敗。cue なし → 77 [C] |
| `ctest -LE history -j 4 --repeat until-pass:2` | **91 本のうち 90 本合格、skip 1 本（`perf_ys`）、失敗 0**（1107 秒）[C]。内訳：北米版と共通の 90 本は 89 本合格・`perf_ys` の skip（3B-1 の前と同じ）、日本版の `jp_boot_ysjp` が合格。やり直しになった試験はない。`app_prep_ys` は 1 回目で合格（142 秒） |
| `--edition` の 3 通り（`build.sh` が ctest に渡す引数と、`ctest -N` の本数） | 下の表 [C] |
| `scripts/build.sh --edition xx`／`--package --edition jp` | `Unknown edition`（2）／`Edition 'jp' has no player package yet (Phase 3B)`（2）[C] |
| ツールの `--edition jp` | `run_bios_check.py`：`no session found`（2）。`check_all_sessions.py`：`…/ys_trace_jp not found: build first`（1）。`make_package.py`：`no package for edition jp yet`（1）。`editions.get('xx')`：`unknown edition 'xx'; available: usa, jp`（2）[C] |
| `build.ps1` | pwsh がこの Mac にないので動かしていない [W] |

| `--edition` | `--test`（quick） | `--test-all` | `--test-hist` | `--package` の試験 |
|---|---|---|---|---|
| `all`（既定） | `-L quick`：22 本 | `-LE ^(history)$`：91 本（`jp_boot_ysjp` を含む） | `-L history`：10 本 | `-L package`：2 本 |
| `usa` | `-L quick -LE ^(jp)$`：22 本 | `-LE ^(history\|jp)$`：90 本（3B-1 の前と同じ） | `-L history -LE ^(jp)$`：10 本 | 2 本 |
| `jp` | `-L quick -LE ^(usa)$`：18 本（共通だけ） | `-LE ^(history\|usa)$`：20 本（共通 19 本と `jp_boot_ysjp`） | 0 本 | 0 本（その前に「まだない」で止まる） |

- 気づいたこと（3B-1 の範囲外、記録だけ）：日本版のディスクは、北米版の System Card 2.0 でも同じところまで進む
  （`run_boot_test.py` に 2.0 を渡すと合格する [C]）。煙試験は BIOS の版を確かめていない。BIOS の照合は 3B-5（Q5）。

### 10.4 Windows の実機での確認手順（保留：後日まとめて行う。`09` の 6 章と同じ形）

ROM と BIOS：北米版は今までどおり（`roms\` 直下に上流の名前）。日本版は `roms\jp\Ys I & II (Japan).cue`（と bin 43 本）、
`roms\bios\syscard3.pce` に置く（置かない場合、`jp_boot_ysjp` は 77 の skip になる。skip は合格ではない）。
before は `8a1033d`、after はブランチの先頭（3B-1 のコミットを含む）。サブモジュールはどちらも `76ab827`。
runtime の C は変えていないので、before のビルドを after に使い回してもよい（`Remove-Item build` は before の前だけ）。

```powershell
cd C:\work\YsRecomp
$env:Path = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;" + $env:Path
# before
git fetch; git checkout 8a1033d; git submodule update
Remove-Item -Recurse -Force build
.\build.ps1 *> b1_before_build.txt
ctest --test-dir build -C Release -N | Select-String "Total Tests"     # -> 100
ctest --test-dir build -C Release -N | Select-String "Test +#" | ForEach-Object { ($_ -split '\s+')[3] } | Set-Content b1_before_names.txt
Copy-Item build\CTestTestfile.cmake b1_before_ctest.cmake
ctest --test-dir build -C Release -LE history -j 2 --repeat until-pass:2 *> b1_before_testall.txt
# after
git checkout feature/japanese-disc-support; git pull; git submodule update
.\build.ps1 *> b1_after_build.txt
cmake -S . -B build | Select-String "YS_|Edition" *> b1_after_config.txt
ctest --test-dir build -C Release -N | Select-String "Total Tests"     # -> 101
ctest --test-dir build -C Release -N -L jp | Select-String "Test +#|Total Tests"   # -> jp_boot_ysjp, 1
ctest --test-dir build -C Release -N | Select-String "Test +#" | ForEach-Object { ($_ -split '\s+')[3] } | Set-Content b1_after_names.txt
Compare-Object (Get-Content b1_before_names.txt) (Get-Content b1_after_names.txt)
Compare-Object ((Get-Content b1_before_ctest.cmake) -replace ' _BACKTRACE_TRIPLES "[^"]*"','') ((Get-Content build\CTestTestfile.cmake) -replace ' _BACKTRACE_TRIPLES "[^"]*"','') | Out-File b1_ctest_diff.txt
ctest --test-dir build -C Release -LE history -j 2 --repeat until-pass:2 *> b1_after_testall.txt
ctest --test-dir build -C Release -R jp_boot_ysjp -V | Select-String "CD:|RAM|OK:|FAILED|skipped" *> b1_after_jp.txt
.\build.ps1 -TestAll -Edition usa *> b1_after_ed_usa.txt
.\build.ps1 -TestAll -Edition jp *> b1_after_ed_jp.txt
.\build.ps1 -Test *> b1_after_ed_all_quick.txt
.\build.ps1 -Package -Edition jp *> b1_after_pkg_jp.txt
.\build.ps1 -Package *> b1_after_pkg_all.txt
python tools\trace\run_bios_check.py --edition jp *> b1_after_tools.txt
git status --short
Select-String -Path b1_*.txt -Pattern "warning|tests passed|\*\*\*Failed|Not Run|Skipped|Total Tests|YS_|Edition|Package|Zip|Built|RAM|OK:|no session" |
  ForEach-Object { "$($_.Filename): $($_.Line.Trim())" } | Set-Content -Encoding utf8 b1_results.txt
```

| ID | 確かめること | 合格の条件 |
|---|---|---|
| W1 | 構成の表示 | `YS_USA_*` は前と同じ（`roms\` 直下）。`YS_JP_CUE: …/roms/jp/Ys I & II (Japan).cue (SHA-1 bdb25f76…)`、`YS_JP_BIOS: …/roms/bios/syscard3.pce (SHA-1 79f5ff55…)`、`Edition jp: no disc map …`（cue の SHA-1 は改行の違いで変わってもよい） |
| W2 | 試験の本数と名前 | before 100、after 101。名前の `Compare-Object` は `jp_boot_ysjp`（`=>`）の 1 行だけ |
| W3 | 試験の定義 | `b1_ctest_diff.txt` の行はすべて `=>` で、`jp_boot_ysjp` の行と `if(CTEST_CONFIGURATION_TYPE…`・`elseif`・`else()`・`endif()` だけ（`<=` の行がない） |
| W4 | 全試験（history 以外） | before 90 本、after 91 本。`jp_boot_ysjp` 以外の合格・skip・失敗が前後で同じ（skip は `perf_ys`）。after の `b1_after_build.txt` に `warning` の行がない |
| W5 | 煙試験 | `b1_after_jp.txt` に `RAM $2C40 (16 bits): 1000, expected 1000: ok` と `OK: the disc booted with the BIOS and reached the game`（macOS と同じ。日本版を置いていなければ `skipped`） |
| W6 | `-Edition` の 3 通り | `-TestAll -Edition usa`：90 本（`jp_boot_ysjp` が出ない）。`-TestAll -Edition jp`：20 本（`jp_boot_ysjp` を含み、`_ys` で終わる北米版の試験が出ない）。`-Test`（既定 `all`）：22 本 |
| W7 | パッケージ | `-Package -Edition jp` は `Edition 'jp' has no player package yet (Phase 3B)` で止まる。`-Package`（`all`）は今までどおり `dist\YsRecomp\` と `dist\YsRecomp.zip` を作る（日本版の分で失敗しない） |
| W8 | ツール | `run_bios_check.py --edition jp` は `no session found`（Python の例外が出ない） |
| W9 | 作業ツリー | `git status --short` に `??` のログ以外が出ない |

貼っていただくもの：`b1_results.txt`、2 つの `Compare-Object` の出力（`b1_ctest_diff.txt`）、`git status --short`。

#### CI の結果（2026-11-28、GitHub Actions `windows-ci`、Windows x64、ROM なし）

コミット `147b39b` の実行 46085272556 は成功（全ステップ）。実機の確認の代わりにはならない（下の「分からないこと」）。

| | 1 つ前（`8a1033d`、実行 46065566228） | 3B-1（`147b39b`） |
|---|---|---|
| 試験の数 | 33 | 34 |
| 合格（実際に動いた） | 15 | 15 |
| skip（77。合格ではない） | 17 | 18 |
| 失敗 | 1 | 1 |

- 増えた 1 本は `jp_boot_ysjp` で、skip（`missing disc or BIOS (…/roms/jp/Ys I & II (Japan).cue / …/roms/bios/syscard3.pce)`）。
  macOS の ROM なしの構成（33 → 34 本、10.2）と同じ本数 [C]。
- 失敗の 1 本は `cpu_huc6280`（`reference/huc6280-tests` が Git の外にあるため。基準 `06b62c6` でも 1 つ前でも同じ既知の失敗）。
- 比較の判定：`OK: no new failures, no lost tests, no pass turned into a skip`（`tools/ci/ctest_report.py`）。
- 分かったこと [C]：Windows x64（MSVC）で `build.ps1` の構成とビルドが通る。ROM の要らない試験の結果が 1 つ前と同じ。
  ROM がないとき、日本版の版の定義が構成を止めず、煙試験が 77 で skip になる。
- 分からないこと（実機の確認待ちのまま）：W1〜W9 のすべて。CI は ROM を使う試験を動かさず、`-Edition usa|jp|all`・
  `-Package -Edition jp`・`run_bios_check.py --edition jp` も呼ばない。CI のビルドの警告の行数はここでは数えていない。

### 10.5 未解決・次の段へ

| ID | 内容 | 段 |
|---|---|---|
| B1-1 | Windows での 3B-1 の確認（10.4。`build.ps1 -Edition` は macOS で動かせない） | 後日の実機 [W] |
| B1-2 | `ys_edition()` の中のターゲット名・試験名の引数化（`EXE`・`TRACE`・`PTRACE`・`PROFILE`・`EQUIP`。M2-3 の残り） | 3B-2、3B-3 |
| B1-3 | `pce_trace_jp`（日本版の profile をリンクしたもの）。それまで煙試験は `pce_trace` を Ys のオプションなしで使う | 3B-2 以降 |
| B1-4 | ツールの `--edition jp` は受け付けるだけ（セッション・実行ファイルがない）。M2-4 の 6 本の版の引数 | 3B-7 |
| B1-5 | `05` の 14.6（Windows での 3A の確認の結果）はまだ書かれていない（14.4 は保留のまま） | 後日の実機 [W] |
