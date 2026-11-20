# M3 計画：macOS で北米版を動かす（新しい基準 M-US）

- 作成日：2026-11-20。対象ブランチ：`feature/japanese-disc-support`（`2063a71`。PCERecomp はフォークの `2e86f2c`）
- **本書は計画である。** コード・CMake・試験は変更していない。実装は 6 章の質問への回答と、ユーザーの承認を得てから行う。
- 元にした文書：`03_macos_risk_register.md`（R04〜R25、決定 D1〜D12）、`05_host_layer_design.md`（H2〜H5、5 章の資源）、
  `04_cross_platform_test_matrix.md`（M-US の条件）、`06_m1_host_skeleton.md`（M1-2〜M1-4）、`09_m2_rom_paths_editions.md`。
- 表記：[C] 確認済み、[A] 推測、[U] 不明。
- この Mac：Apple Silicon、8 コア、メモリ 24 GB、macOS 27.0.1、Apple clang 21.0.0 [C]。

---

## 1. 目的と終了条件

M3 は、北米版のゲーム本体（生成 C、`ys_trace`、`YsRecomp`）を macOS でビルドし、試験する段階です。
その結果を、日本版の実装（3A 以降）で使う macOS の基準（M-US）にします。

| # | 終了条件 |
|---|---|
| E1 | 北米版の全ターゲット（生成 C、`ys_recomp`、`ys_trace`、`YsRecomp`、`pce_runtime`）が、macOS arm64 で警告なしにビルドできる。警告を抑える場合は理由を書く |
| E2 | macOS の `ctest` に、Windows と同じ 99 本の試験が定義される |
| E3 | macOS で、`history` を除く全試験が合格する。<br>skip が残る場合は、件数と理由を書き、合格には数えない。<br>試験の期待値は緩めず、skip も足さない（作業規則 v2 の 6） |
| E4 | `YsRecomp.app` を作れる。ダブルクリックで起動できる（ad-hoc 署名、D12）。設定とセーブは `~/Library/Application Support/<app_id>/` に保存される（D3） |
| E5 | macOS の `package` 試験が合格する。<br>Windows の試験を写したものではなく、`.app`・実行権限・資源を確かめる macOS 用の試験（R07） |
| E6 | Windows の結果が M3 の前と同じ：99 本の名前・合格・skip、パッケージの中身 |
| E7 | macOS と Windows の結果の違いを、すべて説明して記録する（浮動小数点 R06、Metal R14、改行 R25）。<br>違いが残る試験の扱いは、ユーザーが決める |

**M3 でしないこと**：
- vblank の時刻（CAMetalDisplayLink、R11）や GPU 時間の測定（R12）の実装。null のまま、無効であることをログと文書に残す。M3 の後に任意で行う。
- 署名と公証（D12：しない）。
- Intel Mac への対応（D9：対象外）。

---

## 2. 前提と今の状態

| 項目 | 状態 |
|---|---|
| M2 の Windows での確認（W1〜W8） | **待ち**。M3 は Windows の結果を比べる相手にするので、先に終えたい（6 章 Q1） |
| macOS でビルドできたもの | `pce_app`、`app_tests`、`pce_trace`、`bios_check` [C]。`pce_trace` で日本版も動いた（3B-0）[C] |
| macOS で合格した試験 | M1 の 13 本、`host_layers`、`app_disc_id`、`app_cd_menu_music`、`recomp_disc_ys`、`bios_c_ys`、`equip_preview_ys`、`equip_ice_preview_ys` [C] |
| 生成 C | macOS ではまだ生成していない。`games/ys/generated/` はない [C] |
| `reference/`（Geargrafx、HuC6280 の試験、Mednafen） | この Mac にはない [C]。入れるにはダウンロードが要る（承認が必要。6 章 Q2） |
| `platform_macos.cmake` | arm64、`-ffp-contract=off`、null の host 層。`platform_generated_code` と `platform_app_executable` は空 [C] |
| `host_paths` | すべての OS で、実行ファイルのフォルダを使う（`SDL_GetBasePath`）。macOS の方針は M3 で入れる [C] |
| 既知の警告 | `runtime/app/menu.c:448` の `-Wswitch`、SDL3 の非推奨 API の警告多数（M1）。`pce_trace.c:456` の `-Wformat`（PCERecomp、M1-6 の調査で見つけた） |

---

## 3. 段階（各段の終わりに、試験の結果と Windows での確認手順を付けて止まる）

| 段 | 内容 | 扱うリスク | macOS で確かめること | Windows |
|---|---|---|---|---|
| **M3-0** | 基準をとる：<br>• M3 の前の macOS で、全試験の一覧と結果を記録する（ビルドできる範囲）<br>• Windows の M2 の after の結果を受け取る | ― | `ctest -N` の 99 本の一覧 | M2 の確認の結果（W1〜W8） |
| **M3-1** | 生成 C のコンパイル：<br>• まず 24 本のうち 1 本だけを測る（時間と最大メモリ）<br>• 次に全部と `ys_recomp`<br>• 生成コードのフラグを `platform_generated_code` に入れる（警告の抑制。最適化の段階は測定の結果で提案する） | R04、R25 | 時間とメモリの記録。生成 C の中身が Windows の生成と同じか（改行を正規化して比べる） | 不要（生成器は変えない） |
| **M3-2** | `ys_trace` と、それを使う試験：<br>• `recomp` ラベルのロックステップ（`recomp_lockstep_ys` ほか）<br>• 期待値が固定の試験（`layout_switches_ys`、`hud_save_ys`、`battle_ys2` など） | R06、R10 | ロックステップの合格。期待値が固定の試験で Windows と同じ値が出るか。<br>違いが出たら、原因（libm の最後の桁など）を調べて止まる | 不要 |
| **M3-3** | `YsRecomp` と `pce_runtime`（`runtime/main.c` を初めて macOS でコンパイル）：<br>• game スレッドのスタック（R10）<br>• 隠したウィンドウで Metal が描けるか（R20）<br>• Retina（R13）<br>• `stb_image`（R19） | R10、R13、R19、R20 | `YsRecomp` を使う 18 本（`app_*_ys`）。<br>実時間の試験（`TESTS_TEMPO_REAL`）は、しきい値を緩めない | 不要（main.c を変えた場合は M3-4 と一緒に確認） |
| **M3-4** | host 層の macOS の方針：<br>• `host_paths` の書き込み先を `SDL_GetPrefPath`（D3、H2 の `<org>`）、資源を `.app` の `Contents/Resources` にする<br>• 全画面のショートカット（H3）<br>• 新しい試験 `host_paths`（書き込み先。ROM 不要、quick） | R05、R23 | 新しい試験。macOS で ini とセーブが Application Support に書かれること | **必要**：Windows は今と同じ「実行ファイルの隣」のまま。`-TestAll` と `-Package` の前後比較 |
| **M3-5** | `.app`：<br>• `platform_app_executable`（`MACOSX_BUNDLE`、`runtime/host/mac/Info.plist.in`、`.icns`、ad-hoc 署名）<br>• `PCE_DOCS_DIR` の絶対パスに頼らない（R23）<br>• 最低の macOS の版（H5） | R23 | `.app` が起動する。`codesign -v` が通る。`Info.plist` の値 | 不要（Windows の `.rc` は変えない） |
| **M3-6** | パッケージ：<br>• `make_package.py` の macOS の作り方（`.app` を権限を保つ形でまとめる。Q6）<br>• macOS 用のパッケージ試験（`app_package_ys` の macOS 版）<br>• Gatekeeper の手順を README に書く（D12）<br>• `scripts/build.sh --package` | R07 | macOS の `package` ラベルが合格する | **必要**：Windows の `-Package` の zip が前と同じ中身 |
| **M3-7** | Geargrafx との比較（D5）：<br>• `reference/` を入れる（承認後、Q2）<br>• `gg_trace` と `gg_cdinfo` を macOS でビルドする<br>• `console` ラベルの試験と `cpu_huc6280`（R21） | R09、R21 | `trace_ys_*`、`session_ys_*`、`fade_ys`、`hle_timing_ys`、`cd_image`、`cpu_huc6280` が実際に検査して合格する | 不要 |
| **M3-8** | まとめ：<br>• 全試験と history（10 本、長い）<br>• macOS と Windows の結果を比べる表（E7）<br>• M-US の記録（`04` の組み合わせ表を更新）<br>• CLAUDE.md の macOS の節、README | R06、R14 | `history` を含む全試験 | M3 全体の before／after の確認手順（09 の 6 章と同じ形） |

- M3-7 は M3-1〜M3-6 とは独立しているので、Q2 の承認が出れば、どの段の後にでも行える。
- PCERecomp の変更が要るとわかったら、変更せずに止めて報告する（フォーク `DCTomoMD/PCERecomp` にブランチを作るかは、その時に決めていただく）。今の時点で変更が要りそうな箇所：
  - `PCERecomp/CMakeLists.txt:114` の `gg_cdinfo.exe` の固定（M3-7 の `cd_image` 試験）
  - `pce_trace.c:456` の `-Wformat` 警告（E1）
  - `reference_trace` の CMake の MSVC 前提 [U]

---

## 4. 危険と、起きたときの扱い

| ID | 危険 | 扱い |
|---|---|---|
| K1 | 生成 C のコンパイルに時間かメモリがかかりすぎる（R04） | 1 本を測ってから全部を始める。並列数を制限するか、最適化の段階を下げる案を、測定値とともに提案して止まる（勝手に変えない） |
| K2 | 期待値が固定の試験が macOS だけで落ちる（R06：libm、Metal） | 期待値は緩めない。原因を特定し、直し方の案を出して止まる。<br>OS で値が違うことを受け入れて OS ごとの期待値にするかは、ユーザーが決める |
| K3 | game スレッドのスタックが溢れる（R10） | まずスタックの使用量を測る。SDL のスタックの大きさの指定（`SDL_PROP_THREAD_CREATE_STACKSIZE_NUMBER`）で直す案を出す |
| K4 | 隠したウィンドウで Metal が描けず、window の試験が動かない（R20） | 原因を調べて止まる。skip にはしない |
| K5 | M3-4 の `host_paths` の変更が Windows の書き込み先を変える | 方針は OS ごとに分け、Windows は今と同じ値を返す。Windows の確認で前後が同じことを確かめる |
| K6 | `reference/` の中身（GPL-3）が配布物や Git に入る | `reference/` は Git の管理外のまま（PCERecomp の `.gitignore` を確かめる）。パッケージの試験で、入っていないことを確かめる |

---

## 5. Windows で確かめること

- M3-4・M3-6 の後：
  - `-TestAll` の 99 本の名前・合格・skip が、M3 の前と同じ
  - `-Package` の `dist\YsRecomp.zip` の中身が同じ（ファイル一覧と大きさ。M1 の C 系の確認と同じ形）
  - 書き込み先が実行ファイルの隣のまま
- M3 の最後（M3-8）：before を M3 の直前のコミット、after を M3 の最後のコミットにして、09 の 6 章と同じ形で全体を確かめる。

---

## 6. 始める前に決めていただくこと

**2026-11-20：ユーザーの回答は「全部推奨で」。Q1〜Q8 はすべて「案（推奨）」の列のとおりに決定**（Q2：ダウンロードを承認。Windows の実機には `reference/` がサブモジュールにも横のフォルダにもないことを確認：`Test-Path ..\PCERecomp\reference` → False）。

| ID | 質問 | 案（推奨） |
|---|---|---|
| Q1 | M2 の Windows での確認（W1〜W8）が終わる前に、M3 を始めてよいか | **M3-1（生成 C の測定）だけは先に始める**。macOS だけの作業で、Windows に影響しない。M3-4 以降は、M2 の確認の後にする |
| Q2 | `reference/`（Geargrafx、HuC6280 の試験、Mednafen 1.32.1 の 3 ファイル）をこの Mac に入れるダウンロードを承認するか（D5 の実施。作業規則 v2 では SDL3 以外の取得は承認が必要） | **承認する**。2026-11-20 の確認で、**Windows の実機にも `reference/` はない**（`git -C external\PCERecomp\reference\Geargrafx rev-parse HEAD` が「No such file or directory」。07a の skip 6 本と `cpu_huc6280` の失敗と合う）。そのため Geargrafx はダウンロードの時点の最新のリリースのタグに固定し、コミットを記録して、M3-8 で Windows にも同じコミットを入れる。HuC6280 の試験も、取ったコミットを記録して両方で同じものを使う |
| Q3 | `SDL_GetPrefPath` の組織名 `<org>`（H2） | **空文字**：`~/Library/Application Support/YsRecomp/` になる |
| Q4 | macOS の全画面のショートカット（H3） | **Cmd+Ctrl+F を足す**（Alt+Enter も残す） |
| Q5 | 最低の macOS の版（H5、D9） | **macOS 13**。SDL3 3.2.30 の要件と、Apple Clang の既定を確かめてから最終の値を提案する |
| Q6 | macOS のパッケージの形（D10：zip か DMG） | **zip**（`ditto -c -k --keepParent` で作る。実行権限と属性を保つ。追加の道具は要らない） |
| Q7 | `.icns` の作り方 | **macOS 標準の `iconutil`**（追加のインストールなし）。`make_icon.py` が PNG の組を作り、`iconutil` が `.icns` にする。ROM 由来のデータは使わない |
| Q8 | 生成 C のビルドの並列数 | **測定してから決める**。最初は 1 本ずつ。24 GB のメモリで安全な数を、M3-1 の測定値から提案する |

---

## 7. M3 で閉じる未解決の項目

| ID | 内容 | 段 |
|---|---|---|
| R04 | 生成 C のコンパイル時間とメモリ | M3-1 |
| R05 | 書き込み先 | M3-4 |
| R06 | 浮動小数点の一致（`-ffp-contract=off` の効果の確認） | M3-2、M3-8 |
| R07 | macOS のパッケージ試験 | M3-6 |
| R09、R21 | Geargrafx と HuC6280 の参照 | M3-7 |
| R10 | スタック | M3-3 |
| R13、R14、R20 | Retina、Metal、隠したウィンドウ | M3-3、M3-8 |
| R19 | `stb_image` | M3-3 |
| R23 | `PCE_DOCS_DIR` | M3-5 |
| R25 | 改行 | M3-1 |
| M1-2、M1-4 | `host_paths` の方針、`.app` | M3-4、M3-5 |
| H2、H3、H5 | 組織名、全画面のショートカット、最低の版 | Q3〜Q5 |

M3 の後に残るもの：
- R11・R12・M1-3（vblank と GPU 時間の macOS 実装。任意）
- macOS の `-Wswitch` と SDL の非推奨 API の警告のうち、E1 で抑えずに直すと決めたもの（直すかどうかは M3-3 で提案する）

---

## 8. M3-1 の結果（2026-11-20、macOS arm64、Apple clang 21.0.0）

| 測ったもの | 時間 | 最大メモリ | 結果 |
|---|---|---|---|
| 生成 C の生成（`recomp.py generate`） | 51 秒 | 347 MB | 25 本、86 MB（LF）。16,567 関数、220,294 命令。生成器自身の注意 `1419 targets with unknown bank` は Windows と同じ形の情報 |
| `rc_part15.c`（いちばん大きい部分、7.2 MB）を単独でコンパイル（`-O3 -ffp-contract=off`） | 2 分 11 秒 | 2.6 GB | 警告 0 |
| `ys_recomp` 全体（生成 ＋ 25 本のコンパイル。同時 4 本） | 4 分 27 秒（CPU 10 分 31 秒） | 2.7 GB（最大の 1 プロセス） | 成功、コンパイラの警告 0。`libys_recomp.a` 92.7 MB |

見つかった不具合と直し：
- **`add_custom_command` に `VERBATIM` がない**：
  - 症状：BIOS とディスクの名前の `(USA) (v2.0)` の括弧と `&` が、macOS のシェルで構文として読まれ、生成が `syntax error near unexpected token '('` で失敗した [C]。
  - 直し：`cmake/ys_edition.cmake` の 2 つ（生成 C、装備のフレーム）に `VERBATIM` を付けた。Ninja に渡るコマンドで、パスが引用符で囲まれることを確かめた [C]。
  - 確かめたこと：試験の定義は前と同じ（行番号の情報を除いて差 0）[C]。
  - Windows：今も動いていて、`VERBATIM` は各シェルに合った引用にするだけなので、振る舞いは変わらない見込み [A]。**Windows での確認が必要**（生成のやり直しを含むビルド）。

`platform_macos.cmake` に入れたもの：
- 生成 C の同時のコンパイルを 4 本までにする job pool（`generated_c=4`）。Windows の `platform_windows.cmake` は変えていない。
- 警告を抑えるフラグは足していない（コンパイラの警告が出なかったため）。

R25（改行）：
- macOS の生成 C は LF [C]。
- 25 本の中身の SHA-256（ファイル名順に、`名前 + NUL + 中身`。CRLF を LF にそろえて計算）の先頭 16 桁は `9f8978190fba1dde`。
- Windows でも同じ計算をして一致すれば、生成器は OS によらず同じ C を出している。**Windows でも `9f8978190fba1dde`**
  （09 の 7 章の W10）[C]。R25 は閉じる。

```powershell
python -c "import hashlib,glob,os;h=hashlib.sha256();[h.update(os.path.basename(f).encode()+b'\0'+open(f,'rb').read().replace(b'\r\n',b'\n')) for f in sorted(glob.glob('games/ys/generated/c/*.c'))];print(h.hexdigest()[:16])"
```

## 9. M3-2 の結果（2026-11-21、macOS arm64、Apple clang 21.0.0）

- 前提：M2 の Windows での確認は完了（09 の 7 章、W1〜W10 合格）。コードと CMake は変えていない（M3-2 は確かめるだけ）。

| 実行したこと | 結果 |
|---|---|
| `scripts/build.sh --target ys_trace` | 成功（2 分 01 秒。`ys_recomp` は M3-1 のものを使用）。`ys_trace` は `Mach-O 64-bit executable arm64` [C] |
| ビルドの警告 | 2 件：① `pce_trace.c:456` の `-Wformat`（既知、PCERecomp。3 章）、② リンカーの `ignoring duplicate libraries: 'pcerecomp/Release/libpce_core.a'`（新しく見つかった。同じライブラリがリンクの行に 2 回並ぶことへの Apple の `ld` の注意で、結果には影響しない [A]）。どちらも E1 の対象。直し方は M3-3 で `YsRecomp` のリンクと一緒に提案する |
| `ctest -N` | 99 本（E2 と同じ数）[C] |
| `ys_trace` を直接使う試験 44 本のうち、`ys_trace` だけで動く 42 本（`-j 4`） | **42 本すべて合格、skip 0**（6 分 44 秒）[C] |

- 合格した 42 本：
  - ロックステップ：`recomp_lockstep_ys`、`recomp_lockstep_ys_nobios`（生成 C ＝ インタープリタ。1 回目で合格）。
  - 期待値が固定の試験（Windows と同じ期待値のまま）：`layout_switches_ys`、`hud_save_ys`、`hud_save_ys2`、`battle_ys2`、
    `scroll_ys`、`scroll_back_ys`、`actors_ys`、`actors_ys1`、`equipment_ys`、`masks_*`、`zoom_*`、`crop_flash_ys`、
    `cd_pop_ys`、`motion_*`（10 本）、`gpu_*`（3 本）、`equip_*`（9 本）ほか。
- 残した 2 本：`dialog_ys`（`YsRecomp` も使う。M3-3）、`perf_ys`（`run_perf_check.py`。Windows でも skip。M3-7 で
  `reference/` を入れてから）。
- **R06（浮動小数点）**：期待値が固定の試験（滑らかな動き、フェード、GPU の組み立てを CPU で真似るもの、装備）が macOS でも
  同じ値で合格した。`-ffp-contract=off` で、試験が見る範囲では Windows と同じ結果になっている [C]。`YsRecomp` の画面（Metal）と
  history の 10 本は、M3-3・M3-8 で確かめる（R06 は M3-8 で閉じる）。
- `--gpu` の試験（`gpu_window_ys`、`gpu_roof_ys`、`gpu_behind_roof_ys`、`motion_cave_fast_ys`、`motion_edge_ys`、
  `masks_mine_ys`、`equip_ys1`、`equip_ys2`）：`ys_trace` は隠したウィンドウ（`SDL_WINDOW_HIDDEN`）に SDL の renderer を作り
  （`ys_trace_ext.c:1546`）、`runtime/app/gpu_draw.c` で GPU に組み立てさせた画像を、CPU の `motion_render` の画像と点ごとに
  比べる [C]。これが macOS で合格したので、隠したウィンドウでも GPU で描けて、CPU と同じ画像になる [C]。どの renderer
  （macOS の既定は Metal）が選ばれたかは表示されない [A]。R20 は `ys_trace` の範囲では問題なし。`YsRecomp` の窓（R14・R20）は M3-3 で確かめる。
