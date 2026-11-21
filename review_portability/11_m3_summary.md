# M3 のまとめ：macOS で北米版を動かす（新しい基準 M-US）

- 作成日：2026-11-22。対象ブランチ：`feature/japanese-disc-support`。PCERecomp はフォーク `DCTomoMD/PCERecomp` の
  `76ab827`（ブランチ `macos-reference-tools`）。
- 経過の記録（調べたこと、質問と回答、各段の結果）は `10_m3_plan.md`。本書はその結論だけを短くまとめる（Q27）。
- 表記：[C] 確認済み、[A] 推測、[U] 不明。
- この Mac：Apple Silicon、8 コア、メモリ 24 GB、macOS 27.0.1、Apple clang 21.0.0。Windows：Windows 11 x64 の実機
  （`C:\work\YsRecomp`、MSVC、Python 3.15）。

---

## 1. 結論

- 北米版のゲーム本体（生成 C、`ys_recomp`、`ys_trace`、`YsRecomp.app`、`pce_runtime`、パッケージ）は、macOS arm64 で
  **警告なしにビルドでき**、**100 本の試験のうち 99 本が合格**する。skip は 1 本（`perf_ys`）、失敗は 0 [C]。
- 試験の期待値は 1 つも緩めていない。skip も足していない。Windows と同じ期待値のまま合格している。
- これを日本版の実装（Phase 3A 以降）の macOS の基準 **M-US** とする。
- Windows でも、M3 の前と後で結果が変わらないことを確かめた（5.7）。after は 100 本のうち 99 本が合格、skip 1 本
  （`perf_ys`）、失敗 0 で、macOS と同じ [C]。終了条件 E1〜E7 はすべて達成した。

## 2. 終了条件（`10` の 1 章）

| # | 条件 | 状態 |
|---|---|---|
| E1 | 全ターゲットが警告なしにビルドできる。抑える場合は理由を書く | **達成** [C]。抑えたのは SDL3 の非推奨 API の 22 件だけ（SDL3 のターゲットだけ、外部のライブラリで直せないため。Q12）。自分たちのコードの警告は直した（`menu.c` の `-Wswitch`、`pce_trace.c` の `-Wformat`）。Apple の `ld` の重複ライブラリの注意は、結果に影響しないので `-no_warn_duplicate_libraries`（Q10） |
| E2 | Windows と同じ試験が定義される | **達成** [C]。M3 の前の 99 本 ＋ `host_paths`（M3-4 で両方の OS に足した）＝ 100 本。名前はどちらの OS でも同じ（`app_package_ys` は OS ごとのスクリプト、Q22） |
| E3 | history を除く全試験が合格する。skip は件数と理由を書く | **達成** [C]。history 以外 90 本：89 本合格、skip 1 本（`perf_ys`：最適化の前の基準 `build/perf_ref/hle` がない。Windows でも同じ。Q26）。history 10 本も合格 |
| E4 | `YsRecomp.app` が起動し、設定とセーブが Application Support に保存される | **達成** [C]。ユーザーが手で確かめた（`10` の 12.11：アイコン、ランチャー、Cmd+Ctrl+F、ini の場所） |
| E5 | macOS 用の `package` 試験が合格する | **達成** [C]。`app_package_ys`（`run_package_test_macos.py`：署名、`Info.plist`、zip の中身、展開した `.app` が一時フォルダで起動）、`app_launcher_ys` |
| E6 | Windows の結果が M3 の前と同じ | **達成** [C]（5.7）。before（`d0430b1`）と after（`1a54bc7`）とも警告なし。history 以外は 89 本→90 本（増えたのは `host_paths`）がすべて合格（skip は両方とも `perf_ys` だけ）、history 10 本も両方で合格。zip の差は `YsRecomp.exe` の大きさだけ |
| E7 | macOS と Windows の違いをすべて説明して記録する | **達成** [C]（6 章）。合否は 100 本とも同じ。違うのは、中身が OS ごとに違う試験（`app_package_ys`、`host_paths`）と時間だけ |

## 3. 各段の結果

| 段 | 内容 | 結果 | コミット |
|---|---|---|---|
| M3-1 | 生成 C | 生成 51 秒、`ys_recomp` 全体 4 分 27 秒（同時 4 本、最大 2.7 GB）、警告 0。生成 C は Windows と同じ（改行をそろえた SHA-256 の先頭 `9f8978190fba1dde`、R25）。`add_custom_command` に `VERBATIM`（括弧と `&` を含むパス） | `4d575ff` |
| M3-2 | `ys_trace` | 42 本合格（ロックステップ、期待値が固定の試験、GPU と CPU の画像） | `4a92ba8` |
| M3-3 | `YsRecomp` | 18 本合格。隠した窓の Metal が turbo で描く枚数が少ないため、`app_menu_intro_ys` だけ `--screen-dump-wait`（試験の時だけ、保存する範囲のフレームを待つ。Q9） | `76e42eb` |
| M3-4 | 書き込み先 | macOS は `~/Library/Application Support/YsRecomp/`、Windows は今までどおり実行ファイルの隣。試験 `host_paths`。全画面の設定が窓に合わせて変わる（Cmd+Ctrl+F、緑のボタン、Alt+Enter） | `777b06f` |
| M3-5 | `.app` | `Info.plist`、`.icns`（`make_icon.py` と `iconutil`）、ランチャーの画像を `Contents/Resources` に、ad-hoc 署名、macOS 13 以上。警告 0 | `8ee6a1f` |
| M3-6 | パッケージ | `dist/YsRecomp/`（`YsRecomp.app`、`README.txt`＝`package/README-macOS.txt`）と `ditto` の zip。試験用の `PCE_USER_DIR`（本物の Application Support に触れない）。`scripts/build.sh --package` | `c16ecde` |
| M3-7 | Geargrafx との比較 | `reference/`：Geargrafx `1.8.6`（`3d6345c`）、HuC6280 の試験 `4ce378e`、Mednafen 1.32.1 の 3 ファイル。`gg_trace`・`gg_cdinfo` を macOS でビルド（`-ffp-contract=off` が必須）。`console` 17 本、`cpu_huc6280` が合格 | `ecd7442`（PCERecomp `76ab827`） |
| M3-8 | まとめ | history 10 本が合格（727 秒）。本書、`04` の 6 章、CLAUDE.md、README。Windows で M3 全体を確認（5.7） | `1a54bc7`、Windows の結果は本書の次のコミット |

## 4. 決定と、閉じたリスク

### 4.1 M3 で決めたこと（Q1〜Q31。詳細は `10`）

- 環境：arm64 だけ（D9）、macOS 13 以上（Q5）、FMA をまとめない `-ffp-contract=off`（D4。Geargrafx にも必須、M3-7）。
- 書き込み先：`SDL_GetPrefPath("", "YsRecomp")`（Q3）。資源は `.app` の `Contents/Resources`（R23）。
- 全画面：Cmd+Ctrl+F を足し、Alt+Enter も残す（Q4）。設定は窓の状態に合わせる（H3）。
- パッケージ：`ditto` の zip（Q6・Q19）、`.icns` は `iconutil`（Q7）、ディスクから作るファイルは入れない（Q20）、
  README は macOS 用を別に（Q21）、ad-hoc 署名で公証はしない（D12）。`CFBundleIdentifier` は `local.ysrecomp.YsRecomp`（Q14）。
- 警告：SDL3 の非推奨 API は SDL3 だけで抑える（Q12）。`pce_trace.c` はフォークで直す（Q13）。
- Geargrafx は `1.8.6` に固定（Q23。`1.8.7` 以降は PCERecomp のソースの一覧と合わない）。
- PCERecomp の変更はフォークの 2 つのブランチ：`trace-format-warning`（`728e91b`、M3-5）、`macos-reference-tools`
  （`76ab827`、M3-7：`cd_image` の実行ファイル名、Geargrafx の `-ffp-contract=off`、`build.sh`）。Ys に関係するものはない。

### 4.2 閉じたリスク

| ID | 内容 | 結果 |
|---|---|---|
| R04 | 生成 C の時間とメモリ | 4 分 27 秒、最大 2.7 GB（同時 4 本の job pool） |
| R05、M1-2 | 書き込み先 | macOS は Application Support、Windows は今と同じ |
| R06 | 浮動小数点の一致 | 期待値が固定の試験（M3-2）、Geargrafx との音の比較（M3-7）、history（M3-8）が合格 |
| R07 | macOS のパッケージ試験 | M3-6 |
| R09、R21 | Geargrafx と HuC6280 の参照 | M3-7 |
| R10、Q11 | game スレッドのスタック | 変えない（macOS の既定は 512 KB [A]）。history を含む全試験で溢れなかった |
| R13 | Retina | 窓は高密度を使わず、macOS が拡大する。くっきりさせるのは任意の改良 |
| R14、R20 | Metal、隠した窓 | 描けて読み出せる。turbo で描く枚数が少ない点は Q9 で扱った |
| R19 | `stb_image` | `app_launcher_ys` が合格 |
| R23、M1-4 | `PCE_DOCS_DIR`、`.app` | `.app` の資源から読む |
| R25 | 改行 | 生成 C が Windows と同じ |
| H2、H3、H5 | 組織名、全画面、最低の版 | 空文字、Cmd+Ctrl+F と設定の追従、macOS 13 |

### 4.3 残ったこと

| 項目 | 状態 |
|---|---|
| `app_prep_ys` のまれな失敗（M3-5-1） | 原因は未特定。窓が 35 ms を超えて遅れたときに 1〜2 フレーム抜ける。試験は失敗したときに前後の記録を出す（Q17）。M3-7・M3-8 の全試験では起きていない |
| `perf_ys` | skip のまま（Q26）。両方の OS で同じ |
| BGM の音質の差（`10` の 15.6） | Windows のほうがノイズが多いというユーザーの報告。原因は UNKNOWN。Windows の .ini の音の設定は Mac と同じだった（5.7 の V8）ので、設定の違いが原因である見込みは低い [A]。`--audio-wav` での切り分けは、ユーザーの指示があってから |
| R11、R12、M1-3 | vblank の時刻と GPU 時間の macOS 実装。null のまま（M3 の範囲外、任意） |
| 上流への提案 | PCERecomp の PR（`trace-image-write-message`）は上流の作者の返事待ち。`trace-format-warning`・`macos-reference-tools` を提案するかはユーザーが決める |

## 5. Windows での M3 全体の確認手順（Q29）

M2 と同じ実機。09 の 6 章と `10` の 14 章と同じ形。M3-3〜M3-5 のログがなかった分（`10` の 14.6）と、M3-7 の分（`10` の 15.5）を含む。

- before：`d0430b1`（M3-1 の直前）。サブモジュールは `2e86f2c`。
- after：`feature/japanese-disc-support` の先頭（本書のコミットを含む）。サブモジュールは `76ab827`。
- 先に `reference/` を入れておくので、before でも Geargrafx との比較と `cpu_huc6280` が動く。
- 所要時間：1 時間半ほど（生成 C を含むビルド 2 回、全試験 2 回、history 2 回）。

### 5.1 準備：`reference/` と `gg_trace.exe`（1 回だけ）

```powershell
cd C:\work\YsRecomp
$env:Path = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;" + $env:Path
git checkout feature/japanese-disc-support; git pull
git submodule sync; git submodule update
git -C external/PCERecomp log --oneline -1                                   # -> 76ab827
cd external\PCERecomp
git clone --depth 1 --branch 1.8.6 https://github.com/drhelius/Geargrafx.git reference/Geargrafx
git -C reference/Geargrafx rev-parse HEAD                                   # -> 3d6345c4f7b8269b3b4eb074c3d804c4c52a7f0c
git clone --depth 1 https://github.com/SingleStepTests/huc6280.git reference/huc6280-tests
git -C reference/huc6280-tests rev-parse HEAD                               # -> 4ce378e04e243437e6f74dfe76c3cb7c70e45bf1
.\tools\reference_trace\build.ps1
Test-Path tools\reference_trace\build\Release\gg_trace.exe, tools\reference_trace\build\Release\gg_cdinfo.exe   # -> True, True
cd ..\..
```

- HuC6280 の試験の `rev-parse` が `4ce378e...` でなかったとき（その後に更新されたとき）は、次の 2 行で合わせる。

```powershell
git -C external/PCERecomp/reference/huc6280-tests fetch --depth 1 origin 4ce378e04e243437e6f74dfe76c3cb7c70e45bf1
git -C external/PCERecomp/reference/huc6280-tests checkout 4ce378e04e243437e6f74dfe76c3cb7c70e45bf1
```

- Mednafen の 3 ファイルは、試験では使わない（読むための資料）。Windows には入れなくてよい。
- `reference/` と `tools\reference_trace\build\` は Git の管理外（PCERecomp の `.gitignore`）。サブモジュールのコミットを
  切り替えても残る。

### 5.2 before（`d0430b1`）

```powershell
git checkout d0430b1; git submodule update
git log --oneline -1; git -C external/PCERecomp log --oneline -1      # -> d0430b1 / 2e86f2c
Remove-Item -Recurse -Force build
.\build.ps1 *> m38_before_build.txt
ctest --test-dir build -C Release -N | Select-String "Total Tests"     # -> 99
ctest --test-dir build -C Release -N | Select-String "Test +#" | ForEach-Object { ($_ -split '\s+')[3] } | Set-Content m38_before_names.txt
ctest --test-dir build -C Release -LE history -j 2 --repeat until-pass:2 *> m38_before_testall.txt
ctest --test-dir build -C Release -L history *> m38_before_hist.txt
.\build.ps1 -Package *> m38_before_package.txt
python -c "import zipfile;[print(i.filename,i.file_size) for i in zipfile.ZipFile('dist/YsRecomp.zip').infolist()]" *> m38_before_zip.txt
```

### 5.3 after（ブランチの先頭）

```powershell
git checkout feature/japanese-disc-support; git pull
git submodule sync; git submodule update
git log --oneline -1; git -C external/PCERecomp log --oneline -1      # -> the branch tip / 76ab827
Remove-Item -Recurse -Force build
.\build.ps1 *> m38_after_build.txt
ctest --test-dir build -C Release -N | Select-String "Total Tests"     # -> 100
ctest --test-dir build -C Release -N | Select-String "Test +#" | ForEach-Object { ($_ -split '\s+')[3] } | Set-Content m38_after_names.txt
Compare-Object (Get-Content m38_before_names.txt) (Get-Content m38_after_names.txt)
ctest --test-dir build -C Release -LE history -j 2 --repeat until-pass:2 *> m38_after_testall.txt
ctest --test-dir build -C Release -L history *> m38_after_hist.txt
.\build.ps1 -Package *> m38_after_package.txt
python -c "import zipfile;[print(i.filename,i.file_size) for i in zipfile.ZipFile('dist/YsRecomp.zip').infolist()]" *> m38_after_zip.txt
Compare-Object (Get-Content m38_before_zip.txt) (Get-Content m38_after_zip.txt)
git status --short
Get-ChildItem -Path C:\ -Recurse -Filter YsRecomp.ini -ErrorAction SilentlyContinue |
  ForEach-Object { $_.FullName; Select-String -Path $_.FullName -Pattern "^(chip_filter|volume_)" }
```

.ini は遊んだ `YsRecomp.exe` の隣にできる（`runtime/main.c:2610`）。作ったばかりの `dist\YsRecomp` にはないので、
ドライブ全体から探す（遊んだ場所が C: 以外なら、`-Path` をそのドライブにする）。見つからなければ、既定値
（`chip_filter=1`、音量はすべて 100、`runtime/app/volume.c`）で動いている。

### 5.4 まとめ（貼るためのファイル 2 つ）

```powershell
Select-String -Path m38_*.txt -Pattern "warning|tests passed|\*\*\*Failed|Not Run|Skipped|Total Tests|Total Test time|missing in the|Package|Zip|Built" |
  ForEach-Object { "$($_.Filename): $($_.Line.Trim())" } | Set-Content -Encoding utf8 m38_results.txt
Select-String -Path m38_after_testall.txt, m38_after_hist.txt -Pattern "Test +#\d+:" |
  ForEach-Object { $_.Line.Trim() } | Set-Content -Encoding utf8 m38_after_tests.txt
```

### 5.5 チェックポイント

| ID | 確かめること | 元 |
|---|---|---|
| V1 | before・after とも `Built.`。`Total Tests` が before 99・after 100。名前の `Compare-Object` の差は `=> host_paths` の 1 行だけ | E2、Y1 |
| V2 | `m38_after_build.txt` に、before にない警告がない | E1、X1 |
| V3 | history 以外：before 89 本・after 90 本。どちらも skip は `perf_ys` の 1 本だけ、失敗 0（`cpu_huc6280`・`cd_image`・`trace_ys_*`・`session_ys_*` が合格） | E3、E6、X2、15.5 |
| V4 | `app_prep_ys` が合格。`***Failed` の行があれば（2 回目で合格していても）、その行と `missing in the` の後の記録を貼る | Z2 |
| V5 | history：before・after とも 10 本合格 | E6 |
| V6 | `app_package_ys`・`app_launcher_ys` が合格。zip の `Compare-Object` は何も出ないか、`YsRecomp.exe` の大きさの行だけ（M3 で `main.c` などを変えたため） | E6、P3 |
| V7 | `git status --short` に、`??` のログ以外が出ない（`.ico` などが書き直されていない） | Z1 |
| V8 | .ini の `chip_filter` と `volume_*` の値（Mac は `chip_filter=1`、音量はすべて 100）。違えば記録する | Q31 |

### 5.6 貼っていただくもの

`m38_results.txt` の中身、`m38_after_tests.txt` の中身、2 つの `Compare-Object` の出力、`git status --short` の出力、
.ini の行（V8）。

### 5.7 結果（2026-11-22、Windows 11 の実機）

after は `1a54bc7`（PCERecomp `76ab827`）、before は `d0430b1`（`2e86f2c`）。`reference/` は Geargrafx `1.8.6`、HuC6280 の試験 `4ce378e`。

| ID | 結果 | 判定 |
|---|---|---|
| V1 | before・after とも `Built.`。試験は before 99 本・after 100 本で、名前の差は `=> host_paths` の 1 行だけ | 合格 [C] |
| V2 | before・after とも、ビルドのログに `warning` の行がない | 合格 [C] |
| V3 | history 以外は before 89 本・after 90 本とも 100% 合格。skip はどちらも `perf_ys` の 1 本だけ、失敗 0。`cpu_huc6280`、`cd_image`（`gg_cdinfo.exe` の名前のまま）、`trace_ys_*`、`session_ys_*` が合格。時間は before 1375 秒・after 1374 秒（`-j 2`） | 合格 [C] |
| V4 | `app_prep_ys` は 138 秒で合格。`***Failed` の行はない（1 回目で合格） | 合格 [C] |
| V5 | history は before 10 本（660 秒）・after 10 本（657 秒）とも合格 | 合格 [C] |
| V6 | `package` の 2 本（`app_package_ys`、`app_launcher_ys`）は before・after とも合格。zip の差は `YsRecomp/YsRecomp.exe` の大きさだけ（110401536 → 110402560 バイト、+1024）。zip は 28520 KB → 28521 KB。`menu_font.bin`・`launcher_sfx.bin` が zip に入らないという注意は、before にも同じ 2 行が出ている（M3 で変わっていない） | 合格 [C] |
| V7 | `git status --short` は `??` のログだけ | 合格 [C] |
| V8 | 試験の外にある .ini は `C:\work\icon_before\YsRecomp.ini` の 1 つだけ（ほかは試験が作る `build\app_output\` の中）。`chip_filter=1`、`volume_*` はすべて 100 で、Mac と同じ | 設定は同じ [C]。ノイズを聞いたときの .ini がこれかどうかは [A]（`build\Release` で遊んだ場合、その .ini は `Remove-Item build` で消えている。.ini がなければ既定値で、やはり同じ設定） |

## 6. macOS と Windows の比較（E7）

- macOS：`ecd7442` の全試験（`-j 4 --repeat until-pass:2`、1112 秒）と history（1 本ずつ、727 秒）[C]。時間は並列で
  動かしたときのもの（`RUN_SERIAL` の試験は 1 本ずつ）。
- Windows：`1a54bc7` の after（`-j 2 --repeat until-pass:2`、1374 秒）と history（1 本ずつ、657 秒）[C]（5.7、`m38_after_tests.txt`）。
- ラベル：`usa` は全部に付くので書いていない。quick 22 本、console 17 本、recomp 49 本、package 2 本、history 10 本。

| # | 試験 | ラベル | macOS | Windows |
|---|---|---|---|---|
| 1 | `app_config` | quick | 合格（0 秒） || 合格（0 秒） |
| 2 | `app_volume` | quick | 合格（0 秒） || 合格（0 秒） |
| 3 | `app_menu` | quick | 合格（0 秒） || 合格（0 秒） |
| 4 | `app_video` | quick | 合格（0 秒） || 合格（0 秒） |
| 5 | `app_image` | quick | 合格（2 秒） || 合格（2 秒） |
| 6 | `app_crop` | quick | 合格（1 秒） || 合格（1 秒） |
| 7 | `app_redraw` | quick | 合格（0 秒） || 合格（0 秒） |
| 8 | `app_fade` | quick | 合格（0 秒） || 合格（0 秒） |
| 9 | `app_launcher` | quick | 合格（0 秒） || 合格（0 秒） |
| 10 | `app_input` | quick | 合格（0 秒） || 合格（0 秒） |
| 11 | `app_disc_id` | quick | 合格（0 秒） || 合格（0 秒） |
| 12 | `app_cd_menu_music` | quick | 合格（0 秒） || 合格（0 秒） |
| 13 | `host_paths` | quick | 合格（0 秒） || 合格（0 秒） |
| 14 | `host_layers` | quick | 合格（0 秒） || 合格（1 秒） |
| 15 | `nobios_ys` | quick | 合格（3 秒） || 合格（2 秒） |
| 16 | `equip_c_ys` | quick | 合格（6 秒） || 合格（4 秒） |
| 17 | `app_start_ys` | quick | 合格（1 秒） || 合格（1 秒） |
| 18 | `app_mark_ys` | quick | 合格（1 秒） || 合格（1 秒） |
| 19 | `cpu_huc6280` | quick | 合格（2 秒） || 合格（2 秒） |
| 20 | `pce_memory_io` | quick | 合格（0 秒） || 合格（0 秒） |
| 21 | `recomp_python` | quick | 合格（0 秒） || 合格（0 秒） |
| 22 | `tools` | quick | 合格（1 秒） || 合格（1 秒） |
| 23 | `trace_ys_boot` | console | 合格（2 秒） || 合格（2 秒） |
| 24 | `trace_ys_run` | console | 合格（7 秒） || 合格（7 秒） |
| 25 | `session_ys_boot` | console | 合格（3 秒） || 合格（3 秒） |
| 26 | `session_ys_game` | console | 合格（26 秒） || 合格（27 秒） |
| 27 | `session_ys_save` | console | 合格（18 秒） || 合格（19 秒） |
| 28 | `fade_ys` | console | 合格（11 秒） || 合格（8 秒） |
| 29 | `coverage_ys` | console | 合格（22 秒） || 合格（20 秒） |
| 30 | `nobios_cabin_ys` | console | 合格（4 秒） || 合格（4 秒） |
| 31 | `bios_c_ys` | console | 合格（10 秒） || 合格（8 秒） |
| 32 | `hle_timing_ys` | console | 合格（36 秒） || 合格（31 秒） |
| 33 | `equip_preview_ys` | console | 合格（155 秒） || 合格（127 秒） |
| 34 | `equip_ice_preview_ys` | console | 合格（132 秒） || 合格（108 秒） |
| 35 | `game_windows_ys` | console | 合格（5 秒） || 合格（4 秒） |
| 36 | `windows_ys1` | console | 合格（20 秒） || 合格（16 秒） |
| 37 | `panel_ys` | console | 合格（21 秒） || 合格（18 秒） |
| 38 | `windows_crop_ys` | console | 合格（21 秒） || 合格（17 秒） |
| 39 | `cd_image` | console | 合格（7 秒） || 合格（6 秒） |
| 40 | `recomp_disc_ys` | recomp | 合格（60 秒） || 合格（45 秒） |
| 41 | `recomp_lockstep_ys` | recomp | 合格（28 秒） || 合格（22 秒） |
| 42 | `recomp_lockstep_ys_nobios` | recomp | 合格（12 秒） || 合格（10 秒） |
| 43 | `dialog_ys` | recomp | 合格（36 秒） || 合格（31 秒） |
| 44 | `app_stall_ys` | recomp | 合格（10 秒） || 合格（11 秒） |
| 45 | `app_stall_device_ys` | recomp | 合格（10 秒） || 合格（11 秒） |
| 46 | `app_hiccup_ys` | recomp | 合格（31 秒） || 合格（31 秒） |
| 47 | `app_menu_ys` | recomp | 合格（11 秒） || 合格（7 秒） |
| 48 | `app_menu_chip_ys` | recomp | 合格（63 秒） || 合格（61 秒） |
| 49 | `app_menu_image_ys` | recomp | 合格（28 秒） || 合格（20 秒） |
| 50 | `app_intro_ys` | recomp | 合格（52 秒） || 合格（51 秒） |
| 51 | `app_menu_intro_ys` | recomp | 合格（8 秒） || 合格（7 秒） |
| 52 | `perf_ys` | recomp | skip（0 秒） || skip（0 秒） |
| 53 | `app_crop_ys` | recomp | 合格（33 秒） || 合格（26 秒） |
| 54 | `app_torn_window_ys` | recomp | 合格（65 秒） || 合格（65 秒） |
| 55 | `app_load_no_border_ys` | recomp | 合格（67 秒） || 合格（67 秒） |
| 56 | `layout_switches_ys` | recomp | 合格（7 秒） || 合格（5 秒） |
| 57 | `app_rhythm_sewer_ys` | recomp | 合格（172 秒） || 合格（169 秒） |
| 58 | `app_prep_ys` | recomp | 合格（139 秒） || 合格（138 秒） |
| 59 | `torn_window_ys` | recomp | 合格（17 秒） || 合格（14 秒） |
| 60 | `rhythm_cave_ys` | recomp | 合格（52 秒） || 合格（47 秒） |
| 61 | `save_window_ring_ys` | recomp | 合格（38 秒） || 合格（34 秒） |
| 62 | `equipment_ys` | recomp | 合格（15 秒） || 合格（13 秒） |
| 63 | `hud_save_ys` | recomp | 合格（5 秒） || 合格（4 秒） |
| 64 | `hud_save_ys2` | recomp | 合格（30 秒） || 合格（26 秒） |
| 65 | `scroll_ys` | recomp | 合格（10 秒） || 合格（8 秒） |
| 66 | `scroll_back_ys` | recomp | 合格（6 秒） || 合格（5 秒） |
| 67 | `actors_ys` | recomp | 合格（19 秒） || 合格（16 秒） |
| 68 | `actors_ys1` | recomp | 合格（11 秒） || 合格（8 秒） |
| 69 | `motion_cave_fast_ys` | recomp | 合格（66 秒） || 合格（77 秒） |
| 70 | `motion_fast_ys` | recomp | 合格（35 秒） || 合格（29 秒） |
| 71 | `gpu_roof_ys` | recomp | 合格（13 秒） || 合格（14 秒） |
| 72 | `equip_ys1` | recomp | 合格（78 秒） || 合格（71 秒） |
| 73 | `equip_ys2` | recomp | 合格（24 秒） || 合格（27 秒） |
| 74 | `equip_blue_monster_ys` | recomp | 合格（13 秒） || 合格（11 秒） |
| 75 | `equip_yellow_monster_ys` | recomp | 合格（40 秒） || 合格（31 秒） |
| 76 | `equip_monster_back_ys` | recomp | 合格（39 秒） || 合格（31 秒） |
| 77 | `equip_list_ys` | recomp | 合格（16 秒） || 合格（12 秒） |
| 78 | `equip_ice_ys` | recomp | 合格（25 秒） || 合格（21 秒） |
| 79 | `equip_fade_bios_ys` | recomp | 合格（21 秒） || 合格（17 秒） |
| 80 | `battle_ys2` | recomp | 合格（40 秒） || 合格（34 秒） |
| 81 | `motion_edge_ys` | recomp | 合格（28 秒） || 合格（31 秒） |
| 82 | `gpu_behind_roof_ys` | recomp | 合格（14 秒） || 合格（15 秒） |
| 83 | `masks_mine_ys` | recomp | 合格（9 秒） || 合格（9 秒） |
| 84 | `masks_window_ys` | recomp | 合格（16 秒） || 合格（12 秒） |
| 85 | `light_window_ys` | recomp | 合格（38 秒） || 合格（33 秒） |
| 86 | `zoom_save_ys` | recomp | 合格（48 秒） || 合格（38 秒） |
| 87 | `zoom_gameover_ys` | recomp | 合格（14 秒） || 合格（11 秒） |
| 88 | `app_ring_list_ys` | recomp | 合格（82 秒） || 合格（81 秒） |
| 89 | `app_launcher_ys` | package | 合格（20 秒） || 合格（19 秒） |
| 90 | `app_package_ys` | package | 合格（6 秒） || 合格（9 秒） |
| 91 | `motion_save_ys` | history | 合格（75 秒） || 合格（74 秒） |
| 92 | `cd_pop_ys` | history | 合格（97 秒） || 合格（89 秒） |
| 93 | `crop_flash_ys` | history | 合格（126 秒） || 合格（106 秒） |
| 94 | `item_window_ys` | history | 合格（37 秒） || 合格（35 秒） |
| 95 | `gpu_window_ys` | history | 合格（32 秒） || 合格（38 秒） |
| 96 | `motion_intro2_ys` | history | 合格（127 秒） || 合格（109 秒） |
| 97 | `motion_cave_ys` | history | 合格（85 秒） || 合格（72 秒） |
| 98 | `motion_light_ys` | history | 合格（61 秒） || 合格（52 秒） |
| 99 | `motion_ys` | history | 合格（45 秒） || 合格（44 秒） |
| 100 | `motion_temple_ys` | history | 合格（41 秒） || 合格（39 秒） |

### 6.1 違いの説明

- 結果：100 本とも同じ（合格 99 本、skip 1 本、失敗 0）。違いはない [C]。
- skip：両方とも `perf_ys` だけで、理由も同じ（最適化の前の基準 `build/perf_ref/hle` がない。Q26）[C]。
- 中身が OS ごとに違う試験（名前と合格の条件は同じ）：
  - `app_package_ys`：Windows は zip と `YsRecomp.exe`、macOS は `run_package_test_macos.py`（`.app`、署名、`Info.plist`）（Q22）。
  - `host_paths`：書き込み先が、Windows は実行ファイルの隣、macOS は Application Support であることを確かめる（M3-4）。
- 時間：試験ごとの時間の和は、macOS 2963 秒・Windows 2686 秒。10 秒以上で差が 1.3 倍を超えるのは 7 本で、どれも Windows の
  ほうが短い（`recomp_disc_ys` 60→45 秒、`app_menu_image_ys` 28→20 秒、`equip_list_ys` 16→12 秒、`masks_window_ys` 16→12 秒、
  `fade_ys` 11→8 秒、`app_menu_ys` 11→7 秒、`actors_ys1` 11→8 秒）。並列の数が違う（macOS `-j 4`、Windows `-j 2`）ので、
  同時に動く試験との取り合いが違うためと考える [A]。実時間で判定する試験も、両方とも合格している。
- history（1 本ずつ）：macOS 727 秒、Windows 657 秒。
