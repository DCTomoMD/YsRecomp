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

## 10. M3-3 の結果（2026-11-21、macOS arm64）

- コードと CMake は変えていない。`runtime/main.c` は**変更なしで** macOS でコンパイルできた。

| 実行したこと | 結果 |
|---|---|
| `scripts/build.sh --target YsRecomp --target pce_runtime`（`YsRecomp`・`pce_app`・`pce_runtime` のオブジェクトを消して作り直し、警告を全部見た） | 成功。`YsRecomp` は `Mach-O 64-bit executable arm64`（70 MB）。警告は 2 種類：`runtime/app/menu.c:448` の `-Wswitch`（M1 から既知）、リンカーの `ignoring duplicate libraries: libpce_core.a`（`ys_trace`・`YsRecomp`・`pce_runtime` の 3 つ。9 章）。M1 で見た SDL の非推奨 API の警告は、ここでは出なかった [C] |
| `YsRecomp` を使う試験 19 本のうち、パッケージの 2 本を除く 17 本（`-j 4`。`RUN_SERIAL` は 1 本ずつ） | **16 本合格、1 本失敗（`app_menu_intro_ys`）**、skip 0（12 分 49 秒）[C] |
| `app_launcher_ys`（`package` ラベルだが、パッケージは使わない） | 合格（ランチャーの表紙の画像を `stb_image` で読む）[C] |

- 合格した 16 本：`dialog_ys`、`app_start_ys`、`app_menu_ys`、`app_menu_image_ys`、`app_stall_ys`、`app_mark_ys`、
  `app_stall_device_ys`、`app_hiccup_ys`、`app_menu_chip_ys`、`app_intro_ys`、`app_crop_ys`、`app_torn_window_ys`、
  `app_load_no_border_ys`、`app_rhythm_sewer_ys`、`app_prep_ys`、`app_ring_list_ys`。実時間の試験（`TESTS_TEMPO_REAL`）も、しきい値はそのまま。
- 残したもの：`app_package_ys`（M3-6）。

### 10.1 `app_menu_intro_ys` の失敗（M3-3-1）

- 症状：試験の前半（イントロ中にメニューが開かないこと）は合格した。後半の「`--turbo --frames 600 --screen-dump 590 1`
  で 590 フレーム目の窓の画像を 1 枚とる」で、画像が 1 枚も書かれない（`FAILED: no image (code 0)`）。単独で 3 回とも失敗 [C]。
- 原因：
  - `--turbo` では、ゲームは窓を待たずに進み、窓は「描けるだけ描く」（`runtime/main.c` の 15 行目の説明）。
  - `--screen-dump` は、窓が実際に描いた画像だけを保存する（`screen_dump`、1243 行目）。
  - macOS の Metal の窓は、turbo の 600 フレームのうち 126 フレームしか描かなかった（約 80 枚／秒。ゲームは約 400 フレーム／秒）[C]。
    隠した窓でも見える窓でも同じ程度（129 と 79）[C]。
  - 590 フレーム目がちょうど描かれる確率が低く、手で 6 回動かすと書かれたのは 1 回だけだった [C]。
  - 比べるために SDL の描画方式を替えると、OpenGL は 581 フレーム、software は 155 フレームを描いた [C]。Metal の窓は、
    V-Sync を切っても、表示のための画像（drawable）が返ってくる速さで止まると見ている [A]。
  - Windows ではこの試験は毎回合格している（07a、09 の 7 章）。Direct3D の窓がほぼ毎フレームを描くためと見ている [A]。
- 試験の期待値は変えていない。直し方は承認待ち（下の Q9）。

### 10.2 リスクの状態

| ID | 結果 |
|---|---|
| R10（game スレッドのスタック） | `SDL_CreateThread(emu_thread, "game", NULL)` はスタックの大きさを指定しない（macOS の既定は 512 KB、Windows は 1 MB [A]）。17 本の試験（長いセッションを含む）で溢れなかった [C]。大きさをそろえるかは Q11 |
| R13（Retina） | 窓は `SDL_WINDOW_HIGH_PIXEL_DENSITY` なしで作られる。`--window-size 960 540` の画像は 960×540 [C]。Retina の画面では macOS が拡大して表示する（くっきりさせるかは M3 の後の任意の改良） |
| R14・R20（Metal、隠した窓） | 隠した窓でも Metal で描けて、画像を読み出せた（`app_menu_image_ys`、`app_crop_ys` などが合格）[C]。ただし turbo で描ける枚数が少ない（10.1） |
| R19（`stb_image`） | `app_launcher_ys` が合格 [C] |

### 10.3 決めていただくこと

| ID | 質問 | 案（推奨を先に） |
|---|---|---|
| Q9 | `app_menu_intro_ys` の直し方 | **案 A（推奨）**：`--screen-dump` を指定したときだけ、ゲームが保存する範囲のフレームを、窓が描くまで待つ（`runtime/main.c`。プレイヤーの動作は変わらない。OS によらず、指定したフレームが必ず保存される）。ほかの `--screen-dump` の試験（`app_load_frame` 系、`app_menu_under` 系）も同じ道具を使うので、Windows で前後の比較が要る。<br>案 B：試験の側で、590 の 1 枚ではなく、終わりの近くの範囲を保存して最後の 1 枚を見る（期待値の数は同じだが、試験の中身を変えることになる）。<br>案 C：macOS だけ、試験の窓を OpenGL にする（OS ごとの抜け道になるので推奨しない） |
| Q10 | 警告の扱い（E1） | **`-Wswitch`**：`menu.c:448` の `switch` に、足りない 2 つ（`MENU_KEY_LEFT`、`MENU_KEY_RIGHT`）を何もしない分岐として書く（動作は変わらない）。<br>**リンカーの注意**：`platform_macos.cmake` で `-Wl,-no_warn_duplicate_libraries` を付ける（同じライブラリが 2 回並ぶことへの注意だけを消す。理由をコメントに書く）。どちらも Windows には影響しない |
| Q11 | game スレッドのスタック | **今は変えない**（溢れた形跡はない）。M3-8 の history の 10 本でも溢れなければ、そのまま閉じる。<br>別案：Windows と同じ 1 MB を `SDL_PROP_THREAD_CREATE_STACKSIZE_NUMBER` で指定する |

**2026-11-21：ユーザーの回答は「全部推奨で」。** Q9 の案 A は、実装の前に次の 1 点を変えた：
- `--screen-dump` を指定しただけでゲームが待つようにすると、`app_menu_image_ys`（`run_menu_under_test.py`）が弱くなる。
  この試験は、ゲームが窓より先のフレームにいる状態（出力の `with the console on a newer frame than the one shown`）を
  `--screen-dump` で見ているため。
- そのため、待つのは新しいオプション **`--screen-dump-wait`** を付けたときだけにし、`app_menu_intro_ys` だけがそれを使う。

### 10.4 直したこと

| ファイル | 変更 |
|---|---|
| `runtime/main.c` | `--screen-dump-wait`（試験用、`--turbo` のとき）：ゲームのスレッドは、`--screen-dump` の範囲のフレームを、窓がそのフレームの画像を保存するまで先に進めない（最大 2 秒。`dump_wait()`、`g_dump_wait`）。窓は `screen_dump` で画像を読んだ後に、そのフレームの番号を知らせる（`SDL_AtomicInt`）。オプションを付けなければ、今までと同じ動き。プレイヤーの動作は変わらない |
| `tools/app/run_menu_opening_test.py` | 後半の実行に `--screen-dump-wait` を足した。期待値（`lit < 10000`、`white <= 200` で失敗）は変えていない |
| `runtime/app/menu.c` | メインのページの `switch` に `MENU_KEY_LEFT`・`MENU_KEY_RIGHT`（何もしない）を足した。`-Wswitch` が消える。動作は今と同じ（前も `switch` を抜けて `return 0`） |
| `cmake/platform_macos.cmake` | `add_link_options(-Wl,-no_warn_duplicate_libraries)`。Apple の `ld` の「同じライブラリが 2 回」の注意だけを消す（`pce_core` は直接と `pce_app` 経由の 2 回並ぶが、リンクは 1 回）。理由をコメントに書いた。Windows には入らない |

### 10.5 直した後の macOS の結果

| 実行したこと | 結果 |
|---|---|
| `scripts/build.sh`（全ターゲット） | 成功。**警告 0**（`menu.c`・`main.c` を作り直し、すべて再リンク）[C]。`pce_trace.c:456` の `-Wformat` は、PCERecomp のファイルを今回作り直していないので出ていない（残っている） |
| `app_menu_intro_ys` を単独で 3 回 | 3 回とも合格。毎回 `20812 lit pixels, white 255`（同じ値）[C] |
| `YsRecomp` の試験 18 本（`app_launcher_ys` を含む、`-j 4`） | **18 本すべて合格**、skip 0（12 分 49 秒）。`app_menu_image_ys` も合格 [C] |
| quick（21 本） | 20 本合格。失敗は `cpu_huc6280` だけ（`reference/` がない、R21。M3-7）[C] |

- `ys_trace` の 42 本は、今回は再リンクだけ（`main.c` は `ys_trace` に入らない）なので、再実行していない。

### 10.6 Windows で確かめること（`runtime/main.c`・`menu.c` と試験のスクリプトを変えたため）

before は `4a92ba8`（M3-2）、after は M3-3 のコミット。09 の 6 章と同じ実機と手順（`$env:Path` に Build Tools の CMake）。

```powershell
git checkout 4a92ba8; git submodule update
.\build.ps1
ctest --test-dir build -C Release -LE history -j 2 --repeat until-pass:2 *> before_m33_testall.txt
git checkout feature/japanese-disc-support; git pull; git submodule update
.\build.ps1 *> after_m33_build.txt
ctest --test-dir build -C Release -LE history -j 2 --repeat until-pass:2 *> after_m33_testall.txt
ctest --test-dir build -C Release -R "^(app_menu_intro_ys|app_menu_image_ys)$" -V *> after_m33_menu.txt
Select-String -Path before_m33_*.txt, after_m33_*.txt -Pattern "tests passed|\*\*\*Failed|warning|lit pixels|newer frame" |
  ForEach-Object { "$($_.Filename): $($_.Line.Trim())" } | Set-Content -Encoding utf8 m33_results.txt
```

| ID | 確かめること |
|---|---|
| X1 | `after_m33_build.txt` に新しい警告がない（`menu.c` の `switch` を MSVC でもコンパイル） |
| X2 | 全試験：before・after とも 89 本、最後まで失敗は `cpu_huc6280` だけ、skip は同じ 7 本 |
| X3 | `app_menu_intro_ys` が合格し、`lit pixels` の行が出る。`app_menu_image_ys` が合格（`newer frame` の行の数は前と同じでなくてよい） |

## 11. M3-4 の結果（2026-11-21、macOS arm64）

### 11.1 直したこと

| ファイル | 変更 |
|---|---|
| `runtime/host/host_paths_os.h`（新規） | host の中だけで使う宣言 `host_paths_os_user_dir(base, app_id)`：書き込み先を OS が決める |
| `runtime/host/win/paths_win.c`（新規） | 実行ファイルのフォルダを返す（**今と同じ**。`app_id` は使わない） |
| `runtime/host/mac/paths_mac.c`（新規） | `SDL_GetPrefPath("", app_id)`：`~/Library/Application Support/<app_id>/`（D3、Q3：組織名は空）。`app_id` がないときや作れないときは実行ファイルのフォルダ（ログに残す） |
| `runtime/host/common/host_paths_sdl.c` | 資源は今と同じ `SDL_GetBasePath()`。書き込み先は `host_paths_os_user_dir` で、**最初に `host_user_dir()` を呼んだときに**決める（macOS では SDL がそのときフォルダを作るので、ディスクを引数で渡す起動や `--config` の試験では、ユーザーのフォルダに何も作らない） |
| `cmake/platform_windows.cmake`・`platform_macos.cmake` | それぞれの `paths_*.c` を `PLATFORM_HOST_SOURCES` に足した。試験の期待を `PLATFORM_USER_DIR`（`exe` / `pref`）で渡す |
| `CMakeLists.txt` | 新しい試験 `host_paths`（`app_tests paths ${PLATFORM_USER_DIR}`、ROM 不要）を quick に足した。**試験は 99 本 → 100 本** |
| `tests/app/app_tests.c` | `test_paths`：資源が実行ファイルのフォルダであること。書き込み先が、`exe` では実行ファイルのフォルダ、`pref` では `$HOME/Library/Application Support/YsRecompHostPathsTest/`（実装とは別に組み立てた期待値）であること。`pref` では、そこに書いて読み戻し、書いたファイルとフォルダを消す |
| `runtime/main.c`（H3、Q4） | `window_fullscreen_followed()`：窓が `screen_apply` を通らずに全画面に入った・出たとき（`SDL_EVENT_WINDOW_ENTER/LEAVE_FULLSCREEN`）、設定 `fullscreen` を窓に合わせ、オプションメニューの表示と ini をそろえる（Alt+Enter と同じ保存）。ランチャーとゲームの 2 つの事件のループに入れた |

- **H3 の調べ（[C]、SDL 3.2.30 のソース）**：
  - SDL は macOS の「Window」メニューに `Toggle Full Screen`（Cmd+Ctrl+F）を作り（`SDL_cocoaevents.m:504`）、緑のボタンと同じ macOS 標準の全画面に切り替える。窓が `SDL_WINDOW_RESIZABLE`（`main.c` の窓はこれ）なら使える。
  - キーの事件は、メニューより先にゲームにも渡る（`Cocoa_DispatchEvent` の後に `[super sendEvent:]`）。そのため、ゲームで Cmd+Ctrl+F を処理すると 2 回切り替わる。**キーの処理は足さず、設定を窓に合わせる形にした**（2026-11-21、ユーザーの選択）。Alt+Enter はそのまま。
  - Windows では、この事件は `WIN_SetWindowFullscreen` の中（`SDL_windowswindow.c:1292`）と `SDL_UpdateFullscreenMode` の確定の時だけ送られる。最小化（別の窓への切り替え）は確定しないので送られない。つまり Windows では、ゲーム自身の `screen_apply` の結果としてしか届かず、そのときは値が同じなので何もしない（**Windows の動作は変わらない**、ソースの読みで [C]、実機は 11.4 の Y3）。

### 11.2 macOS で確かめたこと

| 実行したこと | 結果 |
|---|---|
| `scripts/build.sh`（全ターゲット） | 成功、警告 0 [C] |
| `host_paths`、`host_layers` | 合格。`paths: user data /Users/<user>/Library/Application Support/YsRecompHostPathsTest/`。試験の後、そのフォルダは残っていない [C]。`host_layers`（`check_layers.py`）は新しい `host/mac/` と `host/win/` のファイルも見て合格 [C] |
| ディスクを引数で渡す起動（`--config` あり） | `~/Library/Application Support/YsRecomp` は作られない [C] |
| プレイヤーの起動（引数なし、`~/Library/Application Support/YsRecomp/YsRecomp.ini` に `game_folder` を置いて、`--turbo --frames 120`） | セーブが `~/Library/Application Support/YsRecomp/saves/Ys Book I & II (USA).brm` に書かれた。実行ファイルの隣（`build/Release/`）には ini も `saves/` も書かれない [C]。確かめた後、このフォルダは消した |
| history 以外の全試験（`-j 4`、H3 の前） | 90 本のうち 81 本合格、skip 7 本（Windows と同じ 7 本、`reference/` がない）、失敗 2 本：`cpu_huc6280`（R21、M3-7）、`app_package_ys`（`ERROR: the .exe does not have the icon embedded`：Windows のパッケージ作りのまま、M3-6）（18 分 13 秒）[C] |
| H3 の後の quick（22 本） | 21 本合格、失敗は `cpu_huc6280` だけ [C] |
| H3 の後の `YsRecomp` の試験 18 本（`-j 4`） | **18 本すべて合格**、skip 0（12 分 52 秒）。試験の後も `~/Library/Application Support/YsRecomp` は作られていない [C] |

- **macOS で確かめていないもの**：Cmd+Ctrl+F と緑のボタンで、設定（オプションメニューの表示と ini）が追従すること。隠した窓の試験では全画面にしない（`hidden_window()`）ので、見える窓で手で確かめる必要がある。M3-5 の `.app` の確認（ユーザー）で一緒に行う。

### 11.3 リスクの状態

| ID | 結果 |
|---|---|
| R05（書き込み先） | macOS は Application Support、Windows は今と同じ [C]（Windows は 11.4 で確認） |
| M1-2（`host_paths` の方針） | 解決（macOS 側）。`.app` の資源の場所（`Contents/Resources`）は M3-5 |
| H2（組織名） | 空文字（Q3）で実装した |
| H3（全画面のショートカット） | 上の 11.1。手での確認は M3-5 |

### 11.4 Windows で確かめること（10.6 の M3-3 の確認と一緒に行う）

before は `4a92ba8`（M3-2）、after は M3-4 のコミット。10.6 の手順の after を M3-4 のコミットにして実行し、次も行う。

```powershell
ctest --test-dir build -C Release -R "^host_paths$" -V *> after_m34_paths.txt
Select-String -Path after_m34_paths.txt -Pattern "paths:|all good|FAILED"
```

| ID | 確かめること |
|---|---|
| Y1 | `host_paths` が合格し、`paths: user data` が実行ファイルのフォルダ（`...\build\Release\`）。試験の数は after で 100 本（`Compare-Object` の差は `host_paths` の 1 本だけ） |
| Y2 | `-Package` の後、`dist\YsRecomp\YsRecomp.exe` を引数なしで起動すると、ini と `saves\` が今までどおり `dist\YsRecomp\` に書かれる |
| Y3 | Alt+Enter で全画面と窓を行き来でき、オプションメニューの表示と ini の `fullscreen` が合う（M3-4 の前と同じ） |

## 12. M3-5 の結果（2026-11-21、macOS arm64）

### 12.1 直したこと（macOS だけ。Windows のファイルとビルドは変えていない）

| ファイル | 変更 |
|---|---|
| `cmake/platform_macos.cmake` | **最低の macOS の版（H5、Q5）**：`CMAKE_OSX_DEPLOYMENT_TARGET` が空なら `13.0`。今までは空で、Apple Clang はビルドした Mac の版（macOS 27）を最低にしていた（`-mmacosx-version-min` がなかった [C]）ので、ほかの Mac では開けなかった。SDL3 3.2.30 は 10.13 まで動く（`docs/README-macos.md`）。<br>**`platform_app_executable`**：`MACOSX_BUNDLE`（`YsRecomp.app/Contents/MacOS/YsRecomp`。試験は `$<TARGET_FILE>` でこれを動かすので、そのまま）、`Info.plist`、アイコン、ランチャーの画像を `Contents/Resources/launcher/` に入れる、ビルドの後に bundle 全体を ad-hoc 署名（`codesign --force --sign - --timestamp=none`、D12）。`CFBundleIdentifier` はキャッシュ変数 `YS_MACOS_BUNDLE_ID`（既定 `local.ysrecomp.YsRecomp`） |
| `runtime/host/mac/Info.plist.in`（新規） | 名前 `YsRecomp`、版 `1.0.0`（Windows の `.rc` と同じ）、`LSMinimumSystemVersion` は `CMAKE_OSX_DEPLOYMENT_TARGET`、`NSHighResolutionCapable`、カテゴリーはゲーム |
| `tools/package/make_icon.py` | `--iconset FOLDER`：`.ico` と同じ絵（ディスク由来のものは使わない）で、macOS の `.iconset` の PNG 10 枚（16〜1024。32 までは顔だけ、64 からは全身、整数倍）を書く。このときは `.ico` に触らない。`iconutil -c icns` で `.icns` にする（Q7）。`.icns` はビルドのフォルダに作り、Git には入れない |

- `.ico` について：引数なしで `make_icon.py` を macOS で動かすと、`.ico` のバイトが Windows で作ったものと違う（大きさは同じ）。PNG の画素と PNG 以外の部分は同じで、違うのは zlib の圧縮だけ [C]。M3-5 の作業中に誤って一度書き直したので、Git の中身に戻した（`git checkout -- runtime/host/win/ysrecomp.ico`、SHA-1 `5ea00552…` を確認）。

### 12.2 macOS で確かめたこと

| 実行したこと | 結果 |
|---|---|
| `scripts/build.sh`（全ターゲット。最低の版を変えたので生成 C から全部） | 成功。コンパイルのフラグに `-mmacosx-version-min=13.0` [C]。警告 24 件：SDL3 のソースの非推奨 API 22 件（`-Wdeprecated-declarations`。最低の版を 13 にしたので出る。M1 でも見えていたもの）と、`pce_trace.c:456` の `-Wformat` 2 件（PCERecomp。`pce_trace` と `ys_trace` の 2 回）。**自分たちのコードの警告は 0** |
| `.app` の中身 | `Contents/MacOS/YsRecomp`、`Contents/Info.plist`、`Contents/Resources/YsRecomp.icns`、`Contents/Resources/launcher/`（4 枚）、`Contents/_CodeSignature/` [C] |
| `codesign -v --strict` | `valid on disk`、`satisfies its Designated Requirement` [C] |
| `vtool -show-build` | `platform MACOS`、`minos 13.0` [C] |
| `plutil -p Info.plist` | 上の表の値 [C] |
| R23：`.app` を別の場所にコピーし、`sandbox-exec` で `docs/` を読めなくしてランチャーを動かした | `.app` の画像で 4 枚とも読めた（`missing image` 0 件）。比べるため `.app` の画像を外すと 4 件とも `missing image`。つまり `.app` は `PCE_DOCS_DIR`（ソースの絶対パス）に頼らない [C]。`SDL_GetBasePath()` が `.app` の中では `Contents/Resources/` を返すことも、これで確かめた [C] |
| 古い作り方の `build/Release/YsRecomp`（`.app` でない実行ファイル） | ビルドのフォルダに残っていたので消した（ビルドの出力だけ） |

- `PCE_DOCS_DIR` は今もコンパイルに入っている（`docs/` の絶対パスの文字列がバイナリに残る）。`.app` の中に画像があるときは使われない。Windows も同じ。取り除くかは M3-6（パッケージ）で決める。

### 12.3 決めていただくこと

| ID | 質問 | 案（推奨を先に） |
|---|---|---|
| Q12 | SDL3 のソースの非推奨 API の警告 22 件（E1） | **SDL3 のターゲットだけ `-Wno-deprecated-declarations` で抑える**（外部のライブラリで、直せない。理由をコメントに書く。自分たちのコードの警告は抑えない）。別案：抑えずに、E1 の例外として文書に書く |
| Q13 | `pce_trace.c:456` の `-Wformat`（PCERecomp） | **フォーク `DCTomoMD/PCERecomp` に直しのブランチを作る**（`long` の `g_watch_vram` を `%04X` に渡しているので、`(unsigned)` を付ける 1 か所。表示は変わらない。上流へ出すかは後で）。作業規則 v2 により、PCERecomp を変える前に承認が必要 |
| Q14 | `CFBundleIdentifier` | **`local.ysrecomp.YsRecomp` のまま**（実在しないドメインの形。個人の名前を入れない）。別案：ご自身の逆ドメイン名を指定する |

**2026-11-21：ユーザーの回答は「全部推奨で」。** Q12〜Q14 は推奨の案に決定。

### 12.3.1 Q12・Q13 の直しと、その後の結果

| ファイル | 変更 |
|---|---|
| `CMakeLists.txt` | SDL3 を取り込んだ直後に `platform_sdl(SDL3-static)` を呼ぶ（各 `platform_*.cmake` が定義する関数。冒頭のコメントにも足した） |
| `cmake/platform_macos.cmake` | `platform_sdl`：SDL3 のターゲットだけに `-Wno-deprecated-declarations`（理由をコメントに書いた） |
| `cmake/platform_windows.cmake` | `platform_sdl`：何もしない（Windows は今と同じ） |
| `external/PCERecomp/tools/trace/pce_trace.c`（フォーク） | 456 行目：`g_watch_vram`（`long`）を `(unsigned)g_watch_vram` にして `%04X` に渡す。表示は変わらない。Ys 固有のものは入れていない |

| 実行したこと | 結果 |
|---|---|
| `scripts/build.sh` | SDL3 と `pce_trace.c` を作り直した（オブジェクトが新しくなったことを確かめた）。**ビルドの警告 0 件**（E1 の macOS 側）。`codesign -v --strict` は有効のまま [C] |
| quick（22 本） | 21 本合格、失敗は `cpu_huc6280` だけ（R21）[C] |

### 12.3.2 history 以外の全試験（Q12・Q13 の前、最低の版 13.0 の後）

- 100 本が定義され（`host_paths` を含む）、history 以外は 90 本。**合格 80 本、失敗 3 本、skip 7 本**（19 分 11 秒）[C]。
  - skip 7 本：M3-4 と同じ（`reference/` がない、M3-7）。
  - `cpu_huc6280`（R21、M3-7）、`app_package_ys`（M3-6）：M3-4 と同じ。
  - **`app_prep_ys`：新しい失敗（M3-5-1）**。

### 12.3.3 `app_prep_ys` の不安定な失敗（M3-5-1、未解決）

- この試験は、新しいフレームの用意を「専用のスレッド」と「窓」の 2 通りで実行し、600 フレーム目からの用意したフレームが
  全部同じであることを確かめる（実時間の試験、`RUN_SERIAL`）。
- 結果：
  - 全試験の中で失敗：`3845 on its own thread, 3842 on the window; ... 3244 in both, 0 different; only in one: [2277, 2278]`。
  - 単独で 3 回：合格、合格、失敗（`only in one: [2947]`）[C]。
  - M3-3 と M3-4 では、それぞれ 1 回ずつ合格していた [C]。
- わかっていること：
  - 両方で用意したフレームの中身は、毎回すべて同じ（`0 different`）。
  - 失敗は、窓の側が数枚を用意しなかったこと（片方にしかないフレーム）で、その番号は毎回違う。時間の揺れで決まる [C]。
  - macOS では、画面の更新の時刻が取れない（`host: display refresh timing not available on this platform`、R11：null の host）
    ので、窓は自分の見積もりで動く。窓が 1 枚とばすことと関係があるかもしれない [A]。
  - 最低の版を 13.0 にしたことと関係があるかは、わからない [U]。
  - Windows では、`--repeat until-pass:2` と `build.ps1` のやり直しで実行しているので、まれな失敗が隠れている可能性もある [U]。
- 期待値は緩めていない。skip も足していない。原因の調べは承認待ち（12.5 の Q15）。

### 12.4 ユーザーに手で確かめていただくこと（見える窓。試験の窓は隠してあるのでできない）

1. Finder で `build/Release/YsRecomp.app` を開く（ダブルクリック）。自分のビルドなので Gatekeeper の警告は出ない見込み [A]。
2. Dock とアプリの切り替え（Cmd+Tab）に Adol のアイコンが出る。
3. 初回はディスクのフォルダを聞かれるので `roms/usa` を選ぶ。ランチャー（表紙 4 枚）が出る。
4. Cmd+Ctrl+F（または緑のボタン）で全画面になり、もう一度押すと窓に戻る。そのあと Esc → オプションメニュー → Video の「全画面」の表示が、そのときの状態と合う（H3）。
5. 終了後、`~/Library/Application Support/YsRecomp/YsRecomp.ini` があり、`fullscreen=` が最後の状態になっている。`build/Release/` には ini も `saves/` もできていない。

結果（どの番号が期待どおりで、どれが違ったか）を教えてください。

### 12.4.1 Windows で確かめること（10.6・11.4 と一緒に）

Windows で変わったのは、`CMakeLists.txt` の `platform_sdl(SDL3-static)` の呼び出しと、`platform_windows.cmake` の空の
`platform_sdl` だけ（`make_icon.py` は `--iconset` のときだけ動きが違う）。

| ID | 確かめること |
|---|---|
| Z1 | `.\build.ps1` が通り、新しい警告がない。`git status` で `runtime/host/win/ysrecomp.ico` が変わっていない |

### 12.5 決めていただくこと（M3-5 の後）

| ID | 質問 | 案（推奨を先に） |
|---|---|---|
| Q15 | `app_prep_ys` の不安定な失敗（12.3.3） | **M3-6 の前に原因を調べる**：① macOS で単独で 10 回動かして失敗の割合を出す、② 最低の版を戻したビルドでも同じように落ちるかを確かめる、③ `run_prep_test.py` と `main.c` の「窓が用意する」道筋を読み、窓がフレームをとばす条件を特定する。直し方の案を出して止まる（試験の期待値は変えない）。<br>別案：M3-8（R11 の null の host を含むまとめ）まで記録だけにする |

**2026-11-21：Q15 の回答は「推奨で」**（M3-6 の前に原因を調べる）。

### 12.6 `app_prep_ys`（M3-5-1）の調べ

| 実行したこと | 結果 |
|---|---|
| ctest で単独 10 回 | 合格 9、失敗 1（`only in one: [1754]`）[C] |
| 窓に用意させる方（`--prep-inline`）だけを、`PCE_AUDIO_LOG=1`（窓が止まった時刻）と `PCE_PACE_LOG=1`（窓の時計のとばし・待ち）を付けて 28 回 | **抜け 0 回**。窓が止まったのはどの回も起動直後（0.0 秒）の 1 回だけ。時計のとばしは 3 フレーム目だけ（600 より前で、試験は見ない）[C] |
| 同じ記録を付けて ctest で 15 回 | **15 回とも合格** [C] |
| 合計 | ctest の試験 28 回で失敗 2 回（ほかに全試験の中で 1 回）。窓に用意させる実行は、試験の中も含めて 56 回で抜け 2 回 [C] |

わかったこと：
- 窓は、そのとき最新のフレームを 1 枚だけ用意する（`main.c` の窓のループ、`pace_b == latest`）。ゲームのスレッドは窓の合図を最大 35 ms 待ち、来なければ「窓が止まった」として音の装置に合わせて進む（`emu_thread`）。このとき 2 フレーム続けて進むと、その間のフレームは窓では用意されない [C]（コードの読み）。
- 試験の失敗は、窓の側のフレームが 1〜2 枚少ないときに起きていて、この道筋と合う [C]。
- **Windows との違い**：Windows は起動時に `vblank_raise_priority()` でプロセス全体を `ABOVE_NORMAL_PRIORITY_CLASS` にする（`host/win/process_win.c:9`。コメント：全コアが忙しいとき、窓の描く枚数が 4977 → 7021 に戻った）。macOS は null で何もしない（`host/null/process_null.c`）[C]。
- そのため、ほかの処理が重なったときに macOS の窓が 35 ms 以上遅れ、ゲームが 2 フレーム進むと見ている [A]。記録を付けた 43 回では再現しなかったので、確かめられていない。
- 最低の版 13.0 との関係は調べていない（再現の割合が低く、比べるには数十回ずつ要る）[U]。

### 12.7 決めていただくこと

| ID | 質問 | 案（推奨を先に） |
|---|---|---|
| Q16 | `app_prep_ys` の次の調べと直し | **案 A（推奨）**：① まず再現の方法を作る：全コアを忙しくした状態（Windows の調べと同じ考え方。CPU を回すだけのプロセスを 8 本）で `app_prep_ys` を 5 回動かし、失敗の割合が上がるかを見る。② 上がれば、macOS の `vblank_raise_priority` を実装する（`host/mac/process_mac.c`、C だけ：窓と game のスレッドの QoS を `QOS_CLASS_USER_INTERACTIVE` にするなど。Windows は変えない）。③ 同じ負荷で、失敗がなくなることを確かめる。試験の期待値は変えない。<br>案 B：再現できないので、M3-8 まで記録だけにする（全試験では `--repeat until-pass:2` で実行する） |

**2026-11-21：Q16 の回答は「推奨で」（案 A）。**

### 12.8 案 A の手順 1（負荷をかけて再現させる）の結果

| 実行したこと | 結果 |
|---|---|
| CPU を回すだけのプロセス（`yes > /dev/null`）を 8 本動かしたまま、ctest で `app_prep_ys` を 5 回 | **5 回とも合格**、抜け 0（窓の用意したフレームは 3844〜3845、いつもと同じ）[C]。負荷のプロセスは終わった後に止め、残っていないことを確かめた |
| SDL がスレッドの優先度をどう上げるか（`SDL_systhread.c`） | macOS でも、`SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_HIGH)` で窓・game・prep のスレッドは `SCHED_RR`、優先度 45 になる [C]。Windows との違いは、プロセス全体の優先度（`ABOVE_NORMAL_PRIORITY_CLASS`。描画のドライバーなど SDL が作らないスレッドにも効く）だけ |
| 画面のスリープとの重なり（失敗はどれも長い実行の後半だった） | `pmset -g` で `displaysleep 0`（画面は消えない）。この仮説は外れ [C] |

- **CPU の取り合いが原因という仮説は、裏づけられなかった。** そのため、手順 2（macOS の優先度の処理を作る）には進んでいない（根拠のない直しになるため）。
- 原因は不明のまま [U]。失敗の割合は、ctest の試験 33 回で 2 回（ほかに全試験の中で 1 回）。

### 12.9 決めていただくこと

| ID | 質問 | 案（推奨を先に） |
|---|---|---|
| Q17 | `app_prep_ys` をこの先どう調べるか | **案 B1（推奨）**：試験の道具に、原因を見るための記録だけを足す。`run_prep_test.py` が窓の側の実行に `PCE_AUDIO_LOG=1`・`PCE_PACE_LOG=1` を付け、失敗したときは、抜けたフレームの前後の「窓が止まった」「時計のとばし」の行を表示する（期待値と合否の条件は変えない）。そのうえで M3-6〜M3-8 の全試験を進め、自然に起きたときの記録で原因を決める。Windows でも同じ記録が出る。<br>案 B2：何も足さず、M3-8 まで記録だけにする（全試験は `--repeat until-pass:2` で実行する） |

**2026-11-21：Q17 の回答は「推奨で」（案 B1）。**

### 12.10 `app_prep_ys` に原因を見るための記録を足した

| ファイル | 変更 |
|---|---|
| `tools/app/run_prep_test.py` | 2 つの実行（専用のスレッド、窓）に `PCE_AUDIO_LOG=1`・`PCE_PACE_LOG=1` を付ける（1 回の実行で数十〜二百行。出力は今までどおり `build/app_output/preparo/{own_thread,window}.txt` に残る）。「片方にしかないフレーム」で失敗したときは、どちらの実行で抜けたかと、その前後 30 フレームの記録（`[frame N] clock: ...`）と、秒で数える記録（`window stalled`、`frame took`）を、最大 40 行表示する（`log_lines`）。**合否の条件と期待値は変えていない** |

| 実行したこと | 結果 |
|---|---|
| `py_compile`、保存してあった記録で `log_lines` を試した | 前後のフレームの行と、秒の行が出る [C] |
| ctest で `app_prep_ys` を 1 回 | 合格。`window.txt` に `clock:` の行が 46、`own_thread.txt` に 42 [C] |

- これからの全試験（M3-6〜M3-8、Windows の確認）で失敗したときは、ctest の出力に原因を見るための行が出る。その記録で原因を決める。
- Windows で確かめること（10.6・11.4・12.4.1 と一緒に）：**Z2** `app_prep_ys` が今までどおり合格する（記録の環境変数は Windows でも同じ意味）。

### 12.11 12.4 の手での確認の結果（2026-11-21、ユーザー）

| # | 結果 |
|---|---|
| 1 | アイコンは正しく表示された |
| 2 | ランチャー（メニュー画面）は正しく動いた |
| 3 | Cmd+Ctrl+F は正しく動いた。最初は全画面で起動し、Cmd+Ctrl+F で窓モードになる（**Windows 版の Alt+Enter と同じ挙動**、ユーザーの確認）。終了後の ini は `fullscreen=0`（最後の状態と合う）[C] |
| 4 | `~/Library/Application Support/YsRecomp/` に `YsRecomp.ini`・`saves/`・`menu_font.bin`・`launcher_sfx.bin` ができた [C] |
| 5 | `build/Release/` に新しいファイルはない（あるのはビルドで作られたものだけ：試験用のプログラム、`.a`、`YsRecomp.app`）[C] |

- H3 の手での確認は完了。M3-5 の macOS の確認は、M3-5-1（`app_prep_ys`、12.10 の記録待ち）を除いて完了。

## 13. M3-6 の計画（パッケージ）

### 13.1 調べたこと

- `make_package.py` は Windows 専用：`.exe` の DLL とアイコンを確かめ、`dist/YsRecomp/`（`YsRecomp.exe`、`README.txt`、`launcher/`）と `dist/YsRecomp.zip` を作る。前にプレイしたフォルダの ini・`saves/` などを残す（`KEEP`）。zip には、プログラムが一度動いて作った `menu_font.bin`・`launcher_sfx.bin`（ディスクから作るもの）も入れる。
- `run_package_test.py`（`app_package_ys`）は、zip を一時フォルダに展開し、`.exe` の隣に ini を置いて**引数なしで起動**し、セーブと記録のセッションが `.exe` の隣にできることを確かめる。
- `package/README.txt` は Windows 向け（Windows 10/11、Alt+Enter、「すべて YsRecomp フォルダの中」など）。
- **macOS では、引数なしの起動は `~/Library/Application Support/YsRecomp/` を使う。** そこには今、ユーザーが実際にプレイした ini とセーブがある [C]。`HOME` を替えても書き込み先は替わらない（`HOME=<別のフォルダ> app_tests paths pref` でも本物のホームのフォルダになった）[C]。そのため、パッケージの試験を今の形で macOS で動かすと、**ユーザーの本物の設定とセーブに書き込んでしまう**。

### 13.2 決めていただくこと

| ID | 質問 | 案（推奨を先に） |
|---|---|---|
| Q18 | パッケージの試験が、ユーザーの本物の Application Support に触れないようにする方法 | **案 A（推奨）**：試験用の環境変数 `PCE_USER_DIR` を host 層に足す（設定されていれば、書き込み先をそのフォルダにする。すべての OS で同じ意味。設定しなければ今と同じ）。macOS のパッケージの試験は、一時フォルダを `PCE_USER_DIR` にして `.app` を起動する。`host_paths` の試験にも、この変数の確認を足す。<br>案 B：試験の中だけ別の `app_id` を使うオプションを足す（`--app-id`）。<br>案 C：引数なしの起動は試験せず、`--config` で起動する（プレイヤーと同じ起動を確かめられなくなるので推奨しない） |
| Q19 | macOS のパッケージの形 | **`dist/YsRecomp/`（`YsRecomp.app` と `README.txt`）と `dist/YsRecomp.zip`**。zip は `ditto -c -k --keepParent`（Q6、実行権限・署名・属性を保つ）。`launcher/` は `.app` の中にあるので外には置かない。設定とセーブは Application Support にあるので、`KEEP` はない（作り直すたびにフォルダを作り直す） |
| Q20 | `menu_font.bin`・`launcher_sfx.bin`（ディスクから作るもの）を macOS の zip に入れるか | **入れない**。macOS ではプログラムが Application Support に作るので、パッケージのフォルダにはできない。初回の起動でプレイヤーのディスクから作られる（Windows でも、ないときはそうなる）。ROM 由来のデータを配布物に入れないことにもなる（作業規則 v2 の 3）。Windows の zip は今のまま |
| Q21 | macOS の README | **`package/README-macOS.txt` を新しく作る**（Windows の `README.txt` は変えない）。zip の中では `README.txt` という名前にする。中身：必要なもの（macOS 13 以降、Apple Silicon）、初回の開き方（自分で署名した ad-hoc なので、Finder で右クリック →「開く」、または「システム設定 → プライバシーとセキュリティ →このまま開く」。D12）、全画面は Cmd+Ctrl+F・Alt+Enter・緑のボタン、ファイルの場所（`~/Library/Application Support/YsRecomp/`）。操作とオプションの説明は Windows 版と同じ文 |
| Q22 | 試験の名前 | **`app_package_ys` のまま**（どちらの OS でも試験の名前は同じ 100 本）。CMake が OS ごとのスクリプトを渡す（Windows：今の `run_package_test.py`、macOS：新しい `run_package_test_macos.py`）。macOS の試験は、`.app` の署名（`codesign -v`）、`Info.plist`、実行権限、zip の中身、zip を展開した `.app` が `PCE_USER_DIR` の一時フォルダで起動し、セーブと記録のセッションがそこにできることを確かめる。`reference/`・GPL のものが入っていないことも確かめる（K6） |

- `scripts/build.sh --package` は、`build.ps1 -Package` と同じ手順（`package` ラベルの試験、`make_package.py`）にする。
- Windows の `make_package.py` の動き（`.exe`、`KEEP`、zip の中身）は変えない。Windows で前後の zip の中身が同じことを確かめる（5 章）。

**2026-11-21：Q18〜Q22 の回答は「全部推奨で」。**

### 13.3 M3-6 で直したこと

| ファイル | 変更 |
|---|---|
| `runtime/host/common/host_paths_sdl.c`、`host_paths.h` | **`PCE_USER_DIR=FOLDER`**（試験用、すべての OS）：設定されていれば、書き込み先をそのフォルダにする（なければ作る）。設定しなければ今と同じ（Windows は実行ファイルの隣、macOS は Application Support）（Q18） |
| `tests/app/app_tests.c`、`CMakeLists.txt` | `host_paths` の試験に、`PCE_USER_DIR` を設定したときの確認を足した（`app_tests paths <policy> <dir>`。書き込み先が `<dir>/user/` になり、フォルダが作られること） |
| `tools/package/make_package.py` | macOS の分岐（`make_macos`）。`.app` の署名（`codesign --verify --strict`）、アイコン、ランチャーの画像を確かめ、`dist/YsRecomp/`（`YsRecomp.app` と `README.txt`）を毎回作り直し、`ditto -c -k --norsrc --noextattr --keepParent` で `dist/YsRecomp.zip` を作る（Q19）。`menu_font.bin`・`launcher_sfx.bin` は入れない（Q20）。**Windows の分岐は変えていない**（`MACOS` のときだけ `make_macos` へ） |
| `package/README-macOS.txt`（新規） | macOS 用の README（Q21）。必要なもの（Apple silicon、macOS 13 以降）、初回の開き方（右クリック →「開く」、または「プライバシーとセキュリティ」→「このまま開く」）、全画面（Cmd+Ctrl+F、Option+Return、緑のボタン）、ファイルの場所（`~/Library/Application Support/YsRecomp/`）。オプションの説明は Windows 版と同じ文（Alt+Tab → Cmd+Tab だけ替えた）。zip の中では `README.txt` |
| `tools/package/run_package_test_macos.py`（新規） | macOS の `app_package_ys`（Q22）：フォルダと zip の中身、署名、`Info.plist` の実行ファイル名、zip に `__MACOSX`・`._*`・`reference/`・Geargrafx・ディスクや設定のファイルがないこと（K6）、作り直しでフォルダが作り直されること。zip を一時フォルダに `ditto -x -k` で展開し、`PCE_USER_DIR` を一時フォルダにして引数なしで起動：600 フレーム、セーブ（2048 バイト）と記録のセッションがそのフォルダにでき、`.app` の隣に何もできず、起動後も署名が有効なこと |
| `cmake/platform_*.cmake`、`cmake/ys_edition.cmake` | `PLATFORM_PACKAGE_TEST`：`app_package_ys` のスクリプトを OS ごとに選ぶ（Windows は今の `run_package_test.py`）。試験の名前は同じ |
| `scripts/build.sh` | `--package`：`package` ラベルの試験（`--test-all` のときは省く）と `make_package.py`（`build.ps1 -Package` と同じ手順）。`--help` の範囲を 1 行のばした |

### 13.4 macOS で確かめたこと

| 実行したこと | 結果 |
|---|---|
| `scripts/build.sh` | 成功、警告 0 [C] |
| `host_paths`、`host_layers`、`app_package_ys` | 3 本とも合格。`app_package_ys`：`OK: package with ['README.txt', 'YsRecomp.app'] (19751 KB zipped), signature valid`、`OK: building again makes the folder anew`、`OK: the package ran from a clean folder and reached the game; the user data went to PCE_USER_DIR.` [C] |
| 本物の `~/Library/Application Support/YsRecomp/` | 試験の前後で `YsRecomp.ini` の更新時刻（12:45:34）が変わらず、セーブも 1 つのまま。**試験は本物の設定とセーブに触れていない** [C] |
| zip を `ditto -x -k` と `unzip` の両方で展開 | どちらも署名が有効で、実行権限（`-rwxr-xr-x`）が残る [C] |
| `scripts/build.sh --package` | `package` の 2 本が合格。`dist/YsRecomp/`（`README.txt`、`YsRecomp.app`）と `dist/YsRecomp.zip`（19751 KB）。`dist/` は `.gitignore` にある [C] |
| quick（22 本） | 21 本合格、失敗は `cpu_huc6280` だけ（R21）[C] |

- R07（macOS のパッケージの試験）は解決。

### 13.5 Windows で確かめること（10.6・11.4・12.4.1 と一緒に）

| ID | 確かめること |
|---|---|
| P1 | `host_paths` が合格し、`paths: with PCE_USER_DIR, user data ...\build\app_output\host_paths\user/` が出る |
| P2 | `app_package_ys` が今までどおり合格する（Windows のスクリプトは変えていない） |
| P3 | `.\build.ps1 -Package` の `dist\YsRecomp.zip` の中身（ファイルの一覧と大きさ）が、M3 の前と同じ |

## 14. M3-3〜M3-6 の Windows での確認手順

M2 と同じ実機（`07a` の 3 章、Windows 11 x64）、09 の 6 章と同じ形。10.6・11.4・12.4.1・13.5 の確認を 1 回にまとめる。

- before：`4a92ba8`（M3-2。M3-3 の前）。サブモジュールは `2e86f2c`。
- after：`feature/japanese-disc-support` の先頭（M3-6 のコミットとこの文書を含む）。サブモジュールはフォークの `728e91b`
  （ブランチ `trace-format-warning`。M3-5 で `pce_trace.c` を 1 か所直した）。
- Windows で変わったもの：`runtime/main.c`（`--screen-dump-wait`、全画面の設定を窓に合わせる処理）、`runtime/app/menu.c`
  （`switch` の 2 つの分岐）、host 層（`paths_win.c`、`host_paths_sdl.c` の `PCE_USER_DIR`）、`app_tests.c`（`host_paths`）、
  `CMakeLists.txt`・`platform_windows.cmake`（`platform_sdl` は空、`PLATFORM_*` の変数）、`run_menu_opening_test.py`、
  `run_prep_test.py`（記録の環境変数）、`make_package.py`（macOS の分岐を足しただけ）、`pce_trace.c`（キャスト）。
  試験は 99 本 → 100 本（`host_paths`）。

### 14.1 before（`4a92ba8`）

```powershell
cd C:\work\YsRecomp
$env:Path = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;" + $env:Path
git fetch; git checkout 4a92ba8; git submodule update
git log --oneline -1; git -C external/PCERecomp log --oneline -1      # -> 4a92ba8 / 2e86f2c
Remove-Item -Recurse -Force build
.\build.ps1 *> m36_before_build.txt
ctest --test-dir build -C Release -N | Select-String "Total Tests"     # -> 99
ctest --test-dir build -C Release -LE history -j 2 --repeat until-pass:2 *> m36_before_testall.txt
ctest --test-dir build -C Release -N | Select-String "Test +#" | ForEach-Object { ($_ -split '\s+')[3] } | Set-Content m36_before_names.txt
.\build.ps1 -Package *> m36_before_package.txt
python -c "import zipfile;[print(i.filename,i.file_size) for i in zipfile.ZipFile('dist/YsRecomp.zip').infolist()]" *> m36_before_zip.txt
```

### 14.2 after（ブランチの先頭）

```powershell
# (in a new window, the $env:Path line of 14.1 again first)
git checkout feature/japanese-disc-support; git pull
git submodule sync; git submodule update
git log --oneline -1; git -C external/PCERecomp log --oneline -1      # -> the branch tip / 728e91b
Remove-Item -Recurse -Force build
.\build.ps1 *> m36_after_build.txt
ctest --test-dir build -C Release -N | Select-String "Total Tests"     # -> 100
ctest --test-dir build -C Release -LE history -j 2 --repeat until-pass:2 *> m36_after_testall.txt
ctest --test-dir build -C Release -N | Select-String "Test +#" | ForEach-Object { ($_ -split '\s+')[3] } | Set-Content m36_after_names.txt
Compare-Object (Get-Content m36_before_names.txt) (Get-Content m36_after_names.txt)
ctest --test-dir build -C Release -R "^(host_paths|app_menu_intro_ys|app_menu_image_ys|app_prep_ys)$" -V *> m36_after_detail.txt
.\build.ps1 -Package *> m36_after_package.txt
python -c "import zipfile;[print(i.filename,i.file_size) for i in zipfile.ZipFile('dist/YsRecomp.zip').infolist()]" *> m36_after_zip.txt
Compare-Object (Get-Content m36_before_zip.txt) (Get-Content m36_after_zip.txt)
git status --short                                                      # -> nothing (the .ico is not rewritten)
```

まとめ：

```powershell
Select-String -Path m36_*.txt -Pattern "warning|tests passed|\*\*\*Failed|Not Run|Skipped|Total Tests|paths:|lit pixels|newer frame|Prepared frames|missing in the|Package|Zip|Built" |
  ForEach-Object { "$($_.Filename): $($_.Line.Trim())" } | Set-Content -Encoding utf8 m36_results.txt
```

### 14.3 手で確かめること（after の `dist\YsRecomp\`）

1. `dist\YsRecomp\YsRecomp.exe` をダブルクリックで起動する（初めてならディスクのフォルダを選ぶ）。
2. Alt+Enter で全画面と窓を 2 回行き来し、Esc → オプションメニュー → Video の「全画面」の表示が、そのときの状態と合う。
3. 終了後、`dist\YsRecomp\YsRecomp.ini` の `fullscreen=` が最後の状態と合い、`dist\YsRecomp\saves\` がある（今までどおり実行ファイルの隣）。

### 14.4 チェックポイント

| ID | 確かめること | 元 |
|---|---|---|
| X1 | `m36_after_build.txt` に、before にない警告がない（`menu.c` を MSVC でもコンパイル） | 10.6 |
| X2 | 全試験：before 89 本・after 90 本（history 以外）、最後まで失敗は `cpu_huc6280` だけ、skip は同じ 7 本 | 10.6 |
| X3 | `m36_after_detail.txt`：`app_menu_intro_ys` が合格して `lit pixels` の行がある。`app_menu_image_ys` が合格 | 10.6 |
| Y1・P1 | `Total Tests` が before 99・after 100。`Compare-Object` の差は `=> host_paths` の 1 行だけ。`host_paths` が合格し、`paths: user data` が `...\build\Release\`、`paths: with PCE_USER_DIR, user data ...\build\app_output\host_paths\user/` | 11.4・13.5 |
| Y2・Y3 | 14.3 の 1〜3（ini と `saves\` が実行ファイルの隣。Alt+Enter で設定が合う） | 11.4 |
| Z1 | `.\build.ps1` が `Built.` で終わる。`git status --short` が何も出さない（`ysrecomp.ico` が書き直されていない） | 12.4.1 |
| Z2 | `m36_after_detail.txt`：`app_prep_ys` が合格（`Prepared frames` の行）。`m36_after_testall.txt` に `app_prep_ys` の `***Failed` があれば（2 回目で合格していても）、その行と `missing in the` の後の記録を貼る | 12.10 |
| P2 | `app_package_ys` が合格（`m36_after_testall.txt`・`m36_after_package.txt`） | 13.5 |
| P3 | zip の `Compare-Object` が何も出さない（中身と大きさが同じ。`YsRecomp.exe` の大きさが変わる場合は、その行を貼る） | 13.5 |

- P3 の補足：`YsRecomp.exe` は `main.c` と `menu.c` を変えたので、大きさが変わることがある。そのときは差を説明として記録する（ほかのファイルが同じであればよい）。

### 14.5 結果を貼るときの内容

`m36_results.txt` の中身、2 つの `Compare-Object` の出力、`git status --short` の出力、14.3 の 1〜3 の結果。

### 14.6 結果（2026-11-22）

Windows 11 x64（M2 と同じ実機）、`C:\work\YsRecomp`。

**M3-3〜M3-5（X1〜X3・Y1〜Y3・Z1〜Z2）**：ユーザーの報告は「Windows での動作は期待どおり」（`911ad88` で実行）。
ただし、14.1・14.2 のログ（`m36_before_*.txt`・`m36_after_*.txt`）はこのフォルダに残っていない。警告の差（X1）、
全試験の本数と結果（X2）、`app_prep_ys` の失敗の有無（Z2）は、ログで確かめていない。ログによる確認は M3-8 の
Windows での全体確認でまとめて行う（未確認として残す）。

**M3-6（P1〜P3）**：`c16ecde` でビルドして、次を実行した（14.2 の一部。before の zip 一覧はないので P3 は中身を直接見た）。

```powershell
ctest --test-dir build -C Release -R "^(host_paths|app_package_ys)$" -V *> m36_p.txt
.\build.ps1 -Package *> m36_p_package.txt
python -c "import zipfile;[print(i.filename,i.file_size) for i in zipfile.ZipFile('dist/YsRecomp.zip').infolist()]" *> m36_p_zip.txt
```

| ID | 結果 | 根拠 |
|---|---|---|
| P1 | 合格 | `host_paths` 合格。`paths: user data C:\work\YsRecomp\build\Release\`（今までどおり実行ファイルの隣）。`paths: with PCE_USER_DIR, user data C:/work/YsRecomp/build/app_output/host_paths/user/` |
| P2 | 合格 | `app_package_ys` 合格（9.39 秒）。`OK: package with ['README.txt', 'YsRecomp.exe', 'launcher']`、`OK: building again keeps ...`、`OK: the package ran in a clean folder and reached the game.`。`m36_p.txt`・`m36_p_package.txt` とも `100% tests passed, 0 tests failed out of 2` |
| P3 | 合格（中身を直接確認） | zip の中身は `YsRecomp/YsRecomp.exe`（110402560 バイト）、`YsRecomp/README.txt`（6308）、`YsRecomp/launcher/` の jpg 4 枚だけ（28521 KB）。`.app`・`README-macOS`・`.ini`・`.bin` はない。before との大きさの比較はしていない |

- `git status --short`：追跡しているファイルの変更はない（`.ico` は書き直されていない）。` M external/PCERecomp` が出た。
  サブモジュールが記録の `728e91b` でなく、前の `2e86f2c` のままと考えられる（ASSUMPTION。今回の手順に
  `git submodule update` を入れていなかった）。`2e86f2c` と `728e91b` の差は `pce_trace.c` の 1 行（printf のキャスト）
  だけで、`YsRecomp.exe` には入らないので、P1〜P3 の結果は変わらない。
- サブモジュールの確認（ユーザーが実行）：`git submodule status` は `+2e86f2c... external/PCERecomp` で、推測どおり前の
  位置だった（CONFIRMED）。`git -C external/PCERecomp status --short` は何も出さない（サブモジュールの中の変更はない）。
  `git submodule sync; git submodule update` で `728e91b` に移った。
- `??` の `*.txt` は M2・M3 の確認で作ったログ（コミットしない）。

## 15. M3-7 の計画（Geargrafx との比較）

### 15.1 調べたこと（2026-11-22、macOS）

**`reference/` の取得**（Q2 の承認による。`external/PCERecomp/reference/`。PCERecomp の `.gitignore` で Git の管理外）：

| もの | 取ったもの | 大きさ |
|---|---|---|
| Geargrafx | `https://github.com/drhelius/Geargrafx.git` のタグ `1.8.6`（`3d6345c4f7b8269b3b4eb074c3d804c4c52a7f0c`、2026-10-30）。`--depth 1` で取り、タグ `1.8.3`〜`1.8.9` を足して確かめた | 27 MB |
| HuC6280 の試験 | `https://github.com/SingleStepTests/huc6280.git` の `4ce378e04e243437e6f74dfe76c3cb7c70e45bf1`（2025-07-06、タグなし）。`v1/` に 256 ファイル | 258 MB |
| Mednafen 1.32.1 | `mednafen-1.32.1.tar.xz`（SHA-256 `de7eb94ab66212ae7758376524368a8ab208234b33796625ca630547dbc83832`）から `pcecd.cpp`・`pcecd.h`・`scsicd.cpp` の 3 ファイル（INDEX.md の手順のまま） | — |

**Geargrafx の版**（Q2 では「最新のリリースのタグ」としていた）：

- 最新の `1.8.9`（2026-11-21）では、`gg_trace` のリンクが失敗した（`LoadDetector::Init` などが未定義）[C]。
  `1.8.7` から `src/load_detector.cpp` が加わったが、PCERecomp の `tools/reference_trace/CMakeLists.txt` の
  ソースの一覧（`GG_CORE_SOURCES`）にないため。
- `1.8.3`〜`1.8.6` には `load_detector.cpp` がなく、一覧の 37 ファイルがすべてある [C]。一覧にないのは、使われていない
  `cdrom_drive*`・`cdrom_physical_image`・`media_file_libretro` だけ。
- 上流の作者が使った Geargrafx のコミットは、どこにも記録がない（PCERecomp は 2026-11-07 に 1 コミットで公開された）。
  一覧と合う最新の版が `1.8.6` なので、上流が使ったのは `1.8.6` か、その前後の master と考えられる（ASSUMPTION）。
- いまは `1.8.6` を置いている（Q23 で決める）。

**ビルド**：`gg_trace`・`gg_cdinfo` は PCERecomp の `tools/reference_trace/` にある別の CMake プロジェクト（Windows の
手順は `build.ps1`。macOS の手順はない）。PCERecomp のファイルを変えずに、次の手で `build/Release/` に作った（`build/` は
PCERecomp の `.gitignore` で管理外）。

```bash
cmake -S external/PCERecomp/tools/reference_trace -B external/PCERecomp/tools/reference_trace/build -G "Ninja Multi-Config" \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
  -DCMAKE_C_FLAGS=-ffp-contract=off -DCMAKE_CXX_FLAGS=-ffp-contract=off
cmake --build external/PCERecomp/tools/reference_trace/build --config Release
```

- 警告 0、約 11 秒 [C]。
- **`-ffp-contract=off` は必須** [C]：付けずにビルドした `gg_trace`（スクラッチのフォルダ）で `session_ys_game` を
  キャッシュなし（`PCE_GG_CACHE=0`）で動かすと、`SOUND different from frame 3400 (56.8 s)` で失敗した。付けたものでは
  合格する。Apple Clang は既定で積和をまとめる（FMA）ので、Geargrafx の音の浮動小数点の計算が Windows（MSVC）と変わる。
  私たちのコードに付けているもの（D4、`cmake/platform_macos.cmake`）と同じ理由。

**試験の結果**（macOS、`c16ecde` のビルドのまま）：

| 試験 | 結果 | 中身 |
|---|---|---|
| `cpu_huc6280` | 合格 [C] | `256 opcodes tested, 640000 tests, 0 failures`。試験の中で除外する分（`skipped: 13 (TMA #0), 2698 (block overwriting its own source/stack)`）は上流の試験の作りのまま |
| `trace_ys_boot` | 合格 [C] | `EQUAL: 3000000 instructions` |
| `trace_ys_run` | 合格 [C] | `EQUAL: 8000000 instructions` |
| `session_ys_boot` | 合格 [C] | `EQUAL: 1200 frames, image and sound identical to Geargrafx`（解像度が画面の途中で変わる 2 フレームは比べない。上流の作り） |
| `session_ys_save` | 合格 [C] | `EQUAL: 6600 frames, ...`（同 3 フレーム） |
| `session_ys_game` | 合格 [C] | `EQUAL: 9000 frames, ...`（同 5 フレーム） |
| `cd_image` | **skip のまま**（試験として） | PCERecomp の `CMakeLists.txt:114` が `gg_cdinfo.exe` の名前を固定しているので、macOS では「not built」で 77 を返す。同じスクリプトに macOS の `gg_cdinfo` を手で渡すと、合成した CD イメージと北米版のディスク（43 トラック）で `PASSED` [C] |
| `perf_ys` | skip のまま | `reference/` とは関係がない。`build/perf_ref/hle`（最適化の前に `run_perf_check.py --save` で記録した基準）がないため。Windows でも同じ。9 章の「M3-7 で `reference/` を入れてから」は誤り |

### 15.2 決めていただくこと

| ID | 決めること | 案（推奨） | ほかの案 |
|---|---|---|---|
| Q23 | Geargrafx の版 | **`1.8.6` に固定する**（`3d6345c`）。PCERecomp の一覧のままビルドでき、上流が使った版に最も近い。M3-8 で Windows にも同じものを入れる | `1.8.9` にして、PCERecomp の一覧に `load_detector` を足す（PCERecomp の変更が増え、上流の試験が作られたときと違う Geargrafx と比べることになる） |
| Q24 | PCERecomp の変更（フォーク `DCTomoMD/PCERecomp`、`728e91b` から新しいブランチ `macos-reference-tools`） | **承認する**。中身は 3 つで、どれも Ys に関係しない：① `CMakeLists.txt:114` の `gg_cdinfo.exe` を `gg_cdinfo${CMAKE_EXECUTABLE_SUFFIX}` にする（`cd_image` が macOS で検査できる）。② `tools/reference_trace/CMakeLists.txt` で、MSVC 以外のとき `gg_core` に `-ffp-contract=off` を付ける（付け忘れると音の比較が失敗する。Windows の MSVC のビルドは変わらない）。③ `tools/reference_trace/build.sh` を足す（`build.ps1` の macOS 版。Ninja Multi-Config、arm64、macOS 13） | 変えない：`cd_image` は macOS で skip のまま、`-ffp-contract=off` は手順に書くだけ（忘れると失敗する） |
| Q25 | macOS で `gg_trace` をビルドする手順 | **Windows と同じく手で 1 回**：`external/PCERecomp/tools/reference_trace/build.sh`（Q24 の ③）。YsRecomp の `scripts/build.sh` には入れない（Windows の `build.ps1` も入れていない）。CLAUDE.md の macOS の節に書く | `scripts/build.sh --reference` を足す |
| Q26 | `perf_ys` | **skip のまま**。最適化の前の基準は今からは作れない（今 `--save` で作ると、今と今を比べるだけになる）。9 章の誤りを直して記録する | 今の状態を基準として記録する（これからの変更の検査にはなる） |

- Q24 を承認いただいたら：フォークのブランチで 3 つを変え、YsRecomp のサブモジュールをそのコミットに進め、`gg_trace` を
  `build.sh` で作り直して、`console` ラベルの試験と `cpu_huc6280`、history 以外の全試験を動かす。
- Windows（M3-8）：`reference/` に同じ Geargrafx `1.8.6` と HuC6280 の試験 `4ce378e` を入れ、`build.ps1` で `gg_trace` を
  作って、skip が 1 本（`perf_ys`）になることを確かめる。

**2026-11-22：ユーザーの回答。Q23 は「BGM のノイズ（Windows 版のほうが多い）の原因が Geargrafx の版なら `1.8.9`、無関係なら
推奨」、Q24〜Q26 は推奨。** Geargrafx は YsRecomp の本体に入っていない（本体の CMake は `tools/reference_trace` を参照せず、
`YsRecomp` の実行ファイルに Geargrafx の記号は 0 件 [C]。使うのは試験の道具の `gg_trace`・`gg_cdinfo` だけ）。そのため
Geargrafx の版はノイズと無関係で、**Q23 は推奨の `1.8.6`** に決定。

### 15.3 M3-7 で直したこと

PCERecomp（フォーク `DCTomoMD/PCERecomp`、`728e91b` から新しいブランチ `macos-reference-tools`。Q24）：

| ファイル | 変更 |
|---|---|
| `CMakeLists.txt`（114 行目） | `cd_image` に渡す名前を `gg_cdinfo.exe` から `gg_cdinfo${CMAKE_EXECUTABLE_SUFFIX}` に（Windows は今と同じ `.exe`） |
| `tools/reference_trace/CMakeLists.txt` | MSVC 以外のとき `gg_core` に `-ffp-contract=off`（PUBLIC。`gg_trace`・`gg_cdinfo` にも付く）。先頭のコメントに `build.sh` を追記。MSVC の分岐は変えていない |
| `tools/reference_trace/build.sh`（新規、実行権限付き） | `build.ps1` の macOS・Linux 版：初回だけ Ninja Multi-Config で構成し、`--config Release --parallel` でビルド。アーキテクチャや macOS の版は指定しない（試験の道具で、配らない。YsRecomp の決定 D9・H5 を PCERecomp に持ち込まない） |

YsRecomp：`CLAUDE.md` の macOS の節（Q25：`reference/` の版と `build.sh`、試験の状況）、この文書。

- Ys に関係するものは PCERecomp に入れていない（上流の規則）。

### 15.4 macOS で確かめたこと

- `tools/reference_trace/build/` を消して `build.sh` で作り直した：警告 0。Release のコンパイル 50 か所すべてに
  `-ffp-contract=off` が付いた（付いていないのはリンクの 2 か所だけ）[C]。
- `scripts/build.sh`（構成し直し）：`Built.`。試験は 100 本のまま。
- `console` ラベル 17 本（`-j 4`）：**17 本すべて合格**（153 秒）。`cd_image` が初めて試験として合格した（skip ではない）[C]。
- キャッシュなし（`PCE_GG_CACHE=0`）で `session_ys_game`（`EQUAL: 9000 frames, image and sound identical to Geargrafx`）、
  `cpu_huc6280`（`640000 tests, 0 failures`）、`cd_image`（`PASSED`）が合格 [C]。
- history 以外の全試験（`-j 4 --repeat until-pass:2`）：**90 本のうち 89 本合格、skip 1 本（`perf_ys`）、失敗 0**
  （1112 秒）[C]。再実行で合格したものはない（`***Failed` の行がない）。`app_prep_ys` も 1 回目で合格。
  M3-6 までの「skip 7 本・`cpu_huc6280` の失敗」が「skip 1 本（`perf_ys`、Q26）」になった。

### 15.5 Windows で確かめること（M3-8 で全体の確認と一緒に）

- `reference/` に同じものを入れる：Geargrafx `git clone --depth 1 --branch 1.8.6`（`3d6345c` を確かめる）、HuC6280 の試験
  （`4ce378e` を確かめる）、Mednafen の 3 ファイル。`tools\reference_trace\build.ps1` で `gg_trace.exe`・`gg_cdinfo.exe` を作る。
- サブモジュールを `macos-reference-tools` のコミットにして、history 以外の全試験：skip が `perf_ys` の 1 本だけ、失敗 0。
  `cd_image` が `.exe` の名前のまま合格する（`${CMAKE_EXECUTABLE_SUFFIX}` が Windows で `.exe` になる）。

### 15.6 BGM の音質の差（ユーザーの報告、M3-7 とは別の件）

- 報告：Windows 版のほうが BGM のノイズが多い。
- Geargrafx とは無関係（上の回答のとおり）[C]。
- 分かっていること [C]：音は両方とも同じコード（`runtime/main.c:2744`）で、44100 Hz・16 ビット・ステレオで SDL3 に渡している。
  その先の、装置の形式への変換（例：48000 Hz）は SDL3 と OS が行う。浮動小数点の計算は両方とも積和をまとめない（D4）。
- 原因は UNKNOWN。考えられるもの（どれも ASSUMPTION）：① `YsRecomp.ini` の設定の違い（`chip_filter`、音量など。Windows と
  Mac で別の .ini を使っている）。② プログラムが出した後の経路（SDL3 の変換、Windows の音の仕組み、音の装置やスピーカー）。
- 切り分けの案：両方で同じ .ini と同じセッションを `--audio-wav` で記録し、WAV が同じかを比べる（同じなら ②、違えば
  プログラムの中）。まず Windows と Mac の .ini の音の設定を比べる。実施はユーザーの指示があってから。
- 2026-11-22（M3-8 の Windows の確認、`11` の 5.7 の V8）：Windows の .ini（`C:\work\icon_before\YsRecomp.ini`）は
  `chip_filter=1`、音量はすべて 100 で、Mac と同じ [C]。① の見込みは低い [A]。Q31 の案の `dist\YsRecomp\YsRecomp.ini` は
  誤りで（.ini は遊んだ `YsRecomp.exe` の隣にでき、作ったばかりの `dist` にはない）、`11` の 5.3 の手順を直した。

## 16. M3-8 の計画（まとめ）

### 16.1 やること（6 章の表の M3-8）

1. macOS：history の 10 本（`motion_save_ys`、`cd_pop_ys`、`crop_flash_ys`、`item_window_ys`、`gpu_window_ys`、
   `motion_intro2_ys`、`motion_cave_ys`、`motion_light_ys`、`motion_ys`、`motion_temple_ys`）。history 以外の 90 本は
   M3-7 で済んでいる（15.4。89 本合格、skip 1 本）。history が合格すれば、R06（浮動小数点の一致）と Q11（game スレッドの
   スタック、溢れなければ閉じる）を閉じる。
2. macOS と Windows の結果を比べる表（E7）：試験ごとに両方の結果と時間を並べる。Windows の結果は 4 の確認で受け取る。
3. M-US の記録：`04_cross_platform_test_matrix.md` を更新する。
4. CLAUDE.md の macOS の節を書き直し、README を更新する。
5. Windows で M3 全体の before／after の確認（09 の 6 章と同じ形）。M3-3〜M3-5 のログがない分（14.6）と、M3-7 の 15.5 を含む。

### 16.2 決めていただくこと

| ID | 決めること | 案（推奨） | ほかの案 |
|---|---|---|---|
| Q27 | M3 のまとめを置く場所 | **新しい文書 `review_portability/11_m3_summary.md`**：結果、決定（D・Q）、閉じたリスク、残ったこと、macOS と Windows の比較の表（E7）を短くまとめる。この文書（10）は経過の記録として残す | この文書に 17 章として足す |
| Q28 | README.md | **macOS を足す**：冒頭の「Windows」を「Windows と macOS（Apple silicon）」に、「How to play」に macOS（`.app`、初回の開き方、データの場所は README-macOS.txt と同じ）、「Building from source」に macOS（Xcode Command Line Tools、Homebrew の cmake と ninja、Python 3.11 以上の `.venv`、`scripts/build.sh`）。英語で、上流の文の形に合わせる | 日本版の対応が終わる Phase 4 まで README は変えない（macOS の情報は CLAUDE.md と review_portability だけ） |
| Q29 | Windows での M3 全体の確認 | **before／after の両方（09 の 6 章と同じ形）**：先に `reference/`（Geargrafx `1.8.6`、HuC6280 の試験 `4ce378e`）を入れて `gg_trace.exe` を作り、before＝`d0430b1`（M3-1 の直前）と after＝ブランチの先頭で、ビルドの警告、試験の一覧、history 以外の全試験、history、パッケージの zip を比べる。両方で Geargrafx の比較が動くので、before の skip も 1 本（`perf_ys`）になるはず。ログは `m38_*.txt` に残す。所要時間は 1 時間半ほど（生成 C のビルド 2 回、全試験 2 回、history 2 回） | after だけ動かし、M2 の Windows の結果（09 の 7 章）と比べる（M3-3〜M3-5 の前後の差がログで示せない） |
| Q30 | `04` の更新のしかた | **M0 の表は計画として残し、「6. M3 の結果（M-US）」の章を足す**：試験の行ごとに M-US の実際の結果（✓・skip とその理由）を書き、2 章の「状態」の列を更新する | 3 章の表の M-US の列を書き換える（M0 の時点の見込みが消える） |
| Q31 | BGM のノイズ（15.6）の手がかり | **Windows の確認に、.ini の音の設定を表示するだけの手順を足す**（`dist\YsRecomp\YsRecomp.ini` の `chip_filter` と `volume_*`。読むだけ）。Mac の設定は `chip_filter=1`、音量はすべて 100 [C]。違いがあれば記録し、切り分けは別の件として扱う | 足さない（M3-8 の範囲外として、別の機会に調べる） |

### 16.3 macOS の history（2026-11-22、`ecd7442`）

- `ctest -L history`（1 本ずつ）：**10 本すべて合格、失敗 0、skip 0**（727 秒）[C]。時間：`motion_save_ys` 75 秒、`cd_pop_ys`
  97 秒、`crop_flash_ys` 126 秒、`item_window_ys` 37 秒、`gpu_window_ys` 32 秒、`motion_intro2_ys` 127 秒、`motion_cave_ys`
  85 秒、`motion_light_ys` 61 秒、`motion_ys` 45 秒、`motion_temple_ys` 41 秒。
- これで macOS の 100 本は、合格 99 本、skip 1 本（`perf_ys`）、失敗 0（history 以外は 15.4）。
- R06（浮動小数点の一致）：期待値が固定の試験（M3-2）と history が合格したので閉じる。Q11（game スレッドのスタック）：
  history の長いセッションでも溢れなかったので、変えずに閉じる。

### 16.4 回答と、M3-8 で書いたもの

**2026-11-22：ユーザーの回答は「Q27〜Q31 は全部推奨で」。** 書いたもの：

- `11_m3_summary.md`（新規、Q27）：結論、終了条件 E1〜E7、各段の結果、決定と閉じたリスク、残ったこと、Windows での M3 全体の
  確認手順（5 章、Q29・Q31）、macOS と Windows の比較の表（6 章、E7。Windows の列は確認の後に埋める）。
- `04_cross_platform_test_matrix.md`：2 章の M-US の状態と、6 章「M3 の結果（M-US）」（Q30）。
- `CLAUDE.md`：macOS の節を書き直した（状態、前提、手順と時間、プレイヤーとデータの場所、Geargrafx、macOS の設定、host 層）。
- `README.md`：冒頭、遊び方（macOS）、全画面のキー、macOS でのビルド（Q28）。
- 次：ユーザーが Windows で `11` の 5 章を実行し、結果を受けて `11` の 6 章と E6・E7 を閉じる。

### 16.5 Windows での M3 全体の確認（2026-11-22）

- ユーザーが `11` の 5 章を Windows 11 の実機で実行した。V1〜V8 はすべて期待どおり [C]。詳細は `11` の 5.7。
- 要点：before（`d0430b1`）と after（`1a54bc7`、PCERecomp `76ab827`）とも警告なし。history 以外は 89 本→90 本（増えたのは
  `host_paths`）がすべて合格、skip は `perf_ys` だけ。history 10 本合格。zip の差は `YsRecomp.exe` の大きさだけ。
  `app_prep_ys` は 1 回目で合格。
- これで E6・E7 を閉じた。`11` の 6 章の Windows の列を埋め、違いを 6.1 に書いた。`04` の 2 章と 6.2 の W-US も更新した。
- 15.5（Windows の `cd_image`、Geargrafx `1.8.6`）も確かめた：`cd_image` は `.exe` の名前のまま合格。
- M3 はこれで終わり。次は Phase 3A（ユーザーの指示があってから）。
