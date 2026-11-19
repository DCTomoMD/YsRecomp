# A 案の追補：B 案（GitHub Actions）を取り込んだ後の変更点

作成日：2026-11-19。`07_plan_a_windows_arm_vm.pdf`（以下「PDF」）の手順はそのまま使い、この文書の 3 点だけを
差し替える。手順 1・2・5・6、C1〜C9、判定の考え方、よくあるつまずきは PDF のとおり。

## 1. 状況の変化（CONFIRMED）

| 項目 | PDF を書いた時点 | 今 |
|---|---|---|
| `feature/japanese-disc-support` の先頭 | `bfab31f`（M1） | `e9c4f6e`（M1 + B 案。`8dce8c5`、`542cf5a`、`e9c4f6e`） |
| フォーク | なし（bundle で渡す） | `https://github.com/DCTomoMD/YsRecomp`（公開。B 案の CI は緑：run 44455745500） |
| サブモジュール | bundle で渡す | `.gitmodules` の URL（公開の上流 PCERecomp、`1b9d25a`）からそのまま取れる。CI の checkout で確認済み |

B 案で変わったのは、CI 用のファイル（`.github/`、`tools/ci/`）、`build.ps1 -VsPath`（省略時の動作は同じ）、
ROM がないときに 77 で skip する試験 2 本（`recomp_disc_ys`、`equip_ice_preview_ys`）。ROM があるときは、
どちらも以前と同じコマンドで動く。ctest の試験の本数は変わらない（C5・C6 の「M1 の前より 1 件多い」はそのまま）。

## 2. 差し替える手順

### 手順 3・4（PDF 5・6 章）：bundle の代わりに clone

VM の PowerShell で：

```powershell
mkdir C:\work
git clone --recurse-submodules -b feature/japanese-disc-support https://github.com/DCTomoMD/YsRecomp.git C:\work\YsRecomp
cd C:\work\YsRecomp
git log --oneline -1                                # -> e9c4f6e review: record plan B CI result ...
git -C external\PCERecomp log --oneline -1          # -> 1b9d25a ...
```

ROM と BIOS は、これまでどおり共有フォルダから `C:\work\YsRecomp\roms\` の直下へコピーする（PDF 手順 5、C1）。
`C:\work\PCERecomp` というフォルダは作らない。

### 手順 7（PDF 9 章）：確かめる対象と SDL3 のハッシュ

確かめる対象は **`e9c4f6e`（ブランチの先頭）** とする。M1 に B 案の修正が加わった、これから先に進むコードそのもので、
ROM があるときに `recomp_disc_ys` と `equip_ice_preview_ys` が元の試験として合格することもここで確かめられる。
M1 だけを見たいときは `git checkout bfab31f` に替える（結果の比べ方は同じ）。

SDL3 のファイルは、CMake の版によって置き場所が違うことがあるので、決め打ちのパスではなく探す：

```powershell
git checkout feature/japanese-disc-support
git submodule update
Remove-Item -Recurse -Force build
.\build.ps1 -Test     *> after_test.txt
.\build.ps1 -TestAll  *> after_testall.txt
.\build.ps1 -Package  *> after_package.txt
$env:PCE_HIDDEN_WINDOW=1; $env:SDL_AUDIO_DRIVER="dummy"
build\Release\YsRecomp.exe --frame-timing --frames 600 "roms\Ys Book I & II (USA).cue" |
  Select-String "GPU \(Direct3D 11|host:" *> after_gpu.txt
Get-ChildItem -Recurse build\_deps -Filter SDL3-3.2.30.tar.gz | Get-FileHash -Algorithm SHA256 *> after_sdl.txt
python tools\trace\check_all_sessions.py --help *> after_tools.txt
```

C10 の期待値は PDF と同じ（`4C3B0933…2E3D`）。ファイルが見つからず `after_sdl.txt` が空なら、その旨を結果に書く。

### 追加のチェックポイント

| 番号 | 何を見るか | 合格の条件 | どこで |
|---|---|---|---|
| C11 | ROM ありでの B 案の 2 本 | `after_testall.txt` で `recomp_disc_ys` と `equip_ice_preview_ys` が Passed（skip ではない）。`before_testall.txt` でも Passed | before/after_testall.txt |

C11 を確かめやすくするため、結果のまとめ（PDF 10 章）のパターンに試験名を足す：

```powershell
cd C:\work\YsRecomp
Select-String -Path before_*.txt, after_*.txt -Pattern "tests passed|\*\*\*Failed|Not Run|Skipped|failed|GPU \(|host:|Hash|usage|recomp_disc_ys|equip_ice_preview_ys" |
  ForEach-Object { "$($_.Filename): $($_.Line.Trim())" } | Set-Content -Encoding utf8 plan_a_results.txt
```

注意：`build.ps1` は `-Verbose` なしでは合格した試験の名前を出さない。C11 の行が出なければ、次で個別に確かめる。

```powershell
ctest --test-dir build -C Release -R "^(recomp_disc_ys|equip_ice_preview_ys)$" *> after_c11.txt
```

（`ctest` が PATH にないときは、Build Tools の `...\CMake\CMake\bin\ctest.exe` をフルパスで呼ぶ。）

## 3. 結果（2026-11-20、ユーザーが実行）

### 3.1 環境
- VM（VMware、Windows 11）はビルドが遅すぎたため、途中から **Windows 11 x64 の実機** に切り替えた。before と after は
  どちらも実機の結果（同じ機械で比べている）。
- 実機：Ryzen 9 7945HS、メモリ 64 GB、RTX 4080 SUPER。VS 2022 Build Tools（winget で入れ、ワークロード
  「C++ によるデスクトップ開発」を後から追加）。Python 3.15.2（上流の前提 3.14 と違う。両方の版で同じなので比べ方には影響しない）。
- VS 2026 しか案内がなかったが、`build.ps1` と生成器（`Visual Studio 17 2022`）が 2022 固定のため、2022 を使った。

### 3.2 手順の変更（PDF・2 章からの差）
- 試験は `build.ps1 -Test`/`-TestAll` ではなく ctest を直接実行した：
  `ctest --test-dir build -C Release -L quick` と `ctest --test-dir build -C Release -LE history -j 2 --repeat until-pass:2`。
  理由：Windows PowerShell 5.1 では、`build.ps1` の `$ErrorActionPreference = 'Stop'` のもとで ctest の
  標準エラー（`Errors while running CTest`）が例外になり、要約の行を書き出す前に止まる（`cpu_huc6280` が失敗するため必ず起きる）。
- スクリプトの実行は `Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass`（その窓だけ）で許可した。
- **取り違えが 1 回あった**：最初の before は、実際には `e9c4f6e` で動いていた（after の `git checkout feature/...` が
  `Already on` と出たことと、before と after の試験の名前・本数が同じだったことで判明）。`build\` を消して `06b62c6` で
  取り直した。今後は、試験の前に次で構成の版を確かめる：
  `git log --oneline -1; ctest --test-dir build -C Release -N | Select-String "host_layers|Total Tests"`
  （`06b62c6` では `host_layers` が出ず、`Total Tests: 98`）。

### 3.3 チェックポイント

| 番号 | before（`06b62c6`） | after（`e9c4f6e`） | 判定 |
|---|---|---|---|
| C1 | BIOS の SHA-1 一致 | — | 合格 |
| C2・C5 | quick 20 本、失敗は `cpu_huc6280` のみ | quick 21 本、失敗は `cpu_huc6280` のみ | 合格（`host_layers` の 1 本の差） |
| C3・C6 | 全試験 88 本、最後まで失敗は `cpu_huc6280` のみ | 89 本、最後まで失敗は `cpu_huc6280` のみ。名前の差は `host_layers` だけ（`Compare-Object`） | 合格 |
| skip | `trace_ys_boot`、`trace_ys_run`、`session_ys_boot`、`session_ys_game`、`session_ys_save`、`perf_ys`、`cd_image` | 同じ 7 本 | 合格 |
| C4・C7 | パッケージ試験 2 本合格、zip 28,519 KB | 2 本合格、zip 28,520 KB | 合格 |
| C8 | アイコン：未報告 | after のアイコンは見た | **見比べが未報告** |
| C9 | GPU（D3D11）平均 3.05 ms、`host:` の行なし | 平均 2.98 ms、`host:` の行なし | 合格 |
| C10 | — | SDL3 の SHA-256 `4C3B0933…2E3D` 一致、`check_all_sessions.py --help` が使い方を表示 | 合格 |
| C11 | `recomp_disc_ys`、`equip_ice_preview_ys` 合格 | 両方合格（ROM ありで元の試験として動く） | 合格 |

- `cpu_huc6280`：`reference/huc6280-tests` が Git にないため両方で失敗（B 案の CI と同じ既知の失敗）。
- skip の 7 本：Geargrafx の `gg_trace`、`build/perf_ref/hle`、PCERecomp の `cd_image` の参照データがないため（77）。
- `recomp_lockstep_ys`（06 の M1-6）：正しい before（`06b62c6`）と、取り違えた回（`e9c4f6e`）の両方の全試験で、1 回目に
  失敗し 2 回目に合格した。出力は `LOCKSTEP OK: 254726888 instructions and 9000 frames identical`（照合は成功）なので、
  終了コードが 0 でなかった。単独（`--repeat until-fail:5` を 5 回）では 25 回すべて合格。原因は未調査
  （他の試験との同時実行、ファイルの取り合い、終了処理のどれか [A]）。M1 の前からあるので M1 の判定には影響しない。
- zip の 1 KB の差：M1 でアイコンの置き場所と host 層のコードが変わったため [A]。パッケージの試験は両方合格。

## 4. A 案に戻るときのプロンプト（PDF 11 章の差し替え。3 章の結果で使い終わった）

```
CLAUDE.md（上流の本文と作業規則 v2）を読んでください。次に review_portability/06_m1_host_skeleton.md、
review_portability/07_plan_a_windows_arm_vm.pdf、review_portability/07a_plan_a_addendum.md を読んでください。

A 案（Windows 11 ARM の VM）で、M1 + B 案（e9c4f6e）の Windows での確認をしました。基準は M1 の前の 06b62c6 です。
手順は 07 の PDF の手順 6 と、07a の差し替えのとおりです。

結果（plan_a_results.txt の中身、after_c11.txt があればその中身、C1・C8 を目で見た結果）：
（ここに貼る）

VM の情報：VM のソフト（Parallels / UTM）、Windows の版、割り当てた CPU とメモリ：
（ここに書く）

お願い：
- C1〜C11 のチェックポイントごとに、合格・不合格・判断できない を表にしてください。before と after の両方で失敗
  した試験は VM が原因として分けてください。
- M1 または B 案が原因と考えられる違いがあれば、原因を調べ、修正案を出してください。修正は承認を得てから行い、
  M2 には進まないでください。
- 結果を review_portability/06_m1_host_skeleton.md に記録し（4 章の M1-1 の状態も更新）、コミットのコマンドを
  示して止まってください。
- roms/ は読み取り専用です。サブモジュール external/PCERecomp に変更が必要になったら、変更せずに止めて報告して
  ください。
- 始める前に、判断が必要な質問をまとめて先に聞いてください。
```
