# M2：ROM・BIOS のキャッシュ変数、北米版だけの ys_edition()、GG_TRACE の .exe 固定の除去、ヘッダーのパス、--edition

- 作成日：2026-11-20。before（基準）：`18e1eaf`。PCERecomp（`external/PCERecomp`、`1b9d25a`）は変更していない。roms/ は読み取りのみ。
- 表記：[C] 確認済み（macOS で実行して確かめた）、[W] Windows の実機での確認待ち、[A] 推測。
- **Windows ではビルドも試験もしていない。** 6 章の手順で、M1 と同じ実機（`07a` の 3 章）で確かめる必要がある。
- 対応するリスク：R03（ROM の固定名）、R08（ヘッダーの `\` のパス）、R24（`recomp_disc_ys` の skip）。設計：`review_jp/02_build_layout.md` 2・5 章、`05_host_layer_design.md` の D5・D7。

---

## 1. M2 の前に決めたこと（2026-11-20、ユーザー：すべて推奨案）

| ID | 決定 |
|---|---|
| B1 | 変数名は `YS_USA_CUE`・`YS_USA_BIOS`。空（既定）のときは上流の場所（`roms/` 直下）、なければ `roms/usa/`（ディスク）と `roms/bios/`（BIOS）。日本版の `YS_JP_CUE`・`YS_JP_BIOS` は M2 では定義しない。候補のファイルは `roms/jp/Ys I & II (Japan).cue` と `roms/bios/syscard3.pce`（Phase 3B で使う。中身の確認は `review_jp/03` の 8 章） |
| R2-3 | BIOS は**ファイル名で選び、中身は確かめない**（今と同じ。違う BIOS は `recomp_disc_ys` のカバレッジ照合が CRC で拒否する）。構成時に使うパスと SHA-1 を表示する。`roms/bios/` には反転していない 2.0（CRC32 `FF2A5EC3`、SHA-1 `2bea3dac…`）が上流の名前で置かれている [C] ので、R2-3 は解消 |
| B3 | `-Edition` / `--edition` の既定値は `all`（ROM が見つかった版すべて。M2 では usa だけ）。ほかの版の試験を `-LE` で除く。`jp` は「まだない（Phase 3B）」で終了。どの版を定義するかは ROM の変数だけで決まる |
| 関数の範囲 | 旧 `CMakeLists.txt` の 166〜256 行（ROM がなくても定義され 77 で skip する試験）と 259〜834 行（ROM があるときだけのブロック）を、条件を変えずに `ys_edition()` へ移す。引数は `ID`・`CUE`・`BIOS`・`GAME_DIR` だけ。ターゲット名・試験名は今のまま（引数化は 3B） |
| R24 / 542cf5a | ROM なしの 77 の形は残し、表示する文だけを `no disc or BIOS (YS_USA_CUE / YS_USA_BIOS; ...)` に直す。R24 は macOS で ROM ありの元の試験が合格したので閉じる（4 章） |
| D7 | ツール側で、`\` → `/`、見つからなければファイル名が同じときだけ版のディスク・BIOS に置き換える。変数は `build/CMakeCache.txt` から読み、`--cue`・`--bios` で上書きできる。セッションのファイルは書き換えない |
| ツールの `--edition` | `run_perf_check`、`run_bios_check`、`check_all_sessions`、`check_session`（ヘッダーの `# edition:` がなければ usa）、`make_package`。ほかは 3B |
| macOS の確認範囲 | `app_disc_id`、`app_cd_menu_music`、`recomp_disc_ys`、`bios_c_ys` と、`pce_trace`・`bios_check` のビルド。`YsRecomp`・`ys_trace`・生成 C は M3 |

## 2. 変更の内容

### 2.1 CMake

| ファイル | 変更 |
|---|---|
| `cmake/ys_edition.cmake`（新規） | `ys_rom_file(VAR FILE SUBFOLDER DESC)`：キャッシュ変数（`FILEPATH`、既定は空）と 1 章の B1 の規則でパスを決め、`-- VAR: <path> (SHA-1 ...)` か `not found` を表示し、同じ名前の通常の変数に入れる（空のキャッシュを隠す。構成のたびに決め直すので、後からファイルを置いても再構成だけでよい）。`ys_edition(ID CUE BIOS GAME_DIR)`：M2 では `usa` 以外は構成エラー。旧 166〜256 行と 259〜834 行をそのまま移した本体（置き換えは 3 種類だけ：`${CMAKE_SOURCE_DIR}/games/ys/` → `${YS_GAME_DIR}/` 106 箇所、フォルダを渡す 3 本の試験の `${CMAKE_SOURCE_DIR}/roms` → cue のあるフォルダ `${YS_DISC_DIR}`、`recomp_disc_ys` の skip の文。行頭に 4 文字の字下げ）。最後に、関数の中で定義された試験に版のラベル（`usa`）を足す |
| `CMakeLists.txt` | `YS_CUE`・`SYSCARD2` の直書き → `ys_rom_file()` 2 本。`PCE_TEST_CUE`、`app_disc_id`、`app_cd_menu_music` は `${YS_USA_CUE}`。`GG_TRACE` は `gg_trace${CMAKE_EXECUTABLE_SUFFIX}`（Windows は `.exe` のまま）。旧ブロックの場所で `ys_edition(ID usa ...)` を呼ぶ（試験の順番も同じ）。グループのラベルは `set_property(... APPEND ...)` にして、版のラベルを消さない（グループどうしの重複はないので、グループのラベルは今と同じ） |

### 2.2 Python ツール

| ファイル | 変更 |
|---|---|
| `tools/editions.py`（新規） | 版の表（M2 は usa だけ：`YsRecomp`、`ys_trace`、`pce_trace`、`games/ys`、変数名とファイル名）。`get()`（jp は「まだない」、不明な版も終了コード 2）、`add_argument()`、`rom_file()`（CMake と同じ規則。`build/CMakeCache.txt` の値が優先）、`normalize()`、`header_path()`（D7）、`session_edition()` |
| `tools/pcerecomp_dir.py` | `GG_TRACE` を `build_paths.exe_name("gg_trace")` に（D5） |
| `tools/trace/check_session.py` | `--edition`（ヘッダーの `# edition:` が優先、なければ usa）、`--cue`、`--bios`。ヘッダーの game・bios を `header_path()`、save を `normalize()` で解決。既定の `--ours` は版の `pce_trace` |
| `tools/trace/run_bios_check.py` | `--edition`、`--cue`、`--bios`。既定のセッションは版のフォルダ。ヘッダーのパスを解決してから存在を確かめる（ないときは今と同じく 77） |
| `tools/trace/check_all_sessions.py` | `--edition`（トレースの実行ファイル、既定のフォルダ、起動中かを見るゲームの名前）、`--cue`、`--bios`（`check_session.py` にも渡す）。版の違うヘッダーのセッションは飛ばす（今のセッションにはこの行がない） |
| `tools/trace/run_perf_check.py` | `--edition`。`--exe`・`--cue` の既定値とセッションのフォルダを版から |
| `tools/package/make_package.py` | `--edition`（argparse に替えた。位置引数の使い方は同じ）。名前は版の実行ファイル名から（usa は今と同じ `YsRecomp`）。`FILES`・`KEEP`・`make(exe, out)` は今の形のまま（`run_package_test.py` が使う） |

- 期待値（`PASS_REGULAR_EXPRESSION`、セッション、カバレッジ）は変更していない。skip は追加していない（`recomp_disc_ys` の 77 は 542cf5a のものを文だけ直した）。

### 2.3 ビルドのスクリプト

- `build.ps1`：`-Edition usa|jp|all`（`ValidateSet`、既定 `all`）。試験は、選ばなかった版のラベルを `-LE "^(...)$"` で除く（`-TestAll` は `history` と一緒に 1 つの正規表現にする）。`-Package` は版ごとに `make_package.py --edition <版>`。
- `scripts/build.sh`：`--edition usa|jp|all`。同じ組み立て。
- 今の `-Edition all`（既定）と `usa` の試験の引数：`-Test` → `-L quick -LE ^(jp)$`、`-TestAll` → `-LE ^(history|jp)$`、`-TestHist` → `-L history -LE ^(jp)$`（`build.sh` の該当部分を取り出して実行して確かめた [C]。`build.ps1` は pwsh がこの Mac にないので [W]）。`jp` のラベルの試験はまだないので、選ばれる試験は M2 の前と同じ [C]（`ctest -N` の件数：quick 21、全試験 89）。

### 2.4 文書

- `CLAUDE.md`：作業規則 v2 の「roms/ の配置」と「ビルドと試験 → macOS」。`README.md`：ROM の置き場所に 1 行。

## 3. 移す前後で試験の定義が同じこと（macOS、[C]）

- `18e1eaf` を作業用フォルダに `git archive` で展開し、その `roms/` に上流の名前のシンボリックリンク（リンク先はユーザーの `roms/usa`・`roms/bios`。roms/ 自体には触れていない）を置いて構成した。今の作業ツリー（`roms/usa`・`roms/bios` を自動で選ぶ）と、CMake が書き出した `CTestTestfile.cmake` を比べた。
- 正規化したのは、ソースとビルドのフォルダ、ROM のフォルダ、`_BACKTRACE_TRIPLES`（CMake の行番号）、版のラベル `usa`、`gg_trace.exe` → `gg_trace`（macOS では `CMAKE_EXECUTABLE_SUFFIX` が空。Windows では `.exe` のままなので差にならない [A]）。
- 結果：**99 本すべてで、名前・順番・コマンド・属性（SKIP_RETURN_CODE、PASS/FAIL_REGULAR_EXPRESSION、TIMEOUT、RUN_SERIAL、ENVIRONMENT、グループのラベル）が同じ。残った差は 0 行。**
- ROM がないとき（`-DYS_USA_CUE=/nonexistent/...`）：`18e1eaf` の ROM なしの構成と同じ 32 本、名前も同じ。
- 版のラベル `usa` は 81 本（関数の中のもの）。付かないのは関数の外の 18 本（`app_*` の 12 本、`host_layers`、PCERecomp の `cpu_huc6280`・`pce_memory_io`・`recomp_python`・`cd_image`・`tools`）。

## 4. macOS で実行したこと（結果）

| コマンド | 結果 |
|---|---|
| `cmake -S . -B build` | `YS_USA_CUE: .../roms/usa/Ys Book I & II (USA).cue (SHA-1 ee92126a…)`、`YS_USA_BIOS: .../roms/bios/[BIOS] ... (v2.0).pce (SHA-1 2bea3dac…)` [C] |
| `scripts/build.sh --target app_tests --target bios_check --target pce_trace` | 成功。`pce_trace`・`bios_check` は `Mach-O 64-bit executable arm64`。ビルドの警告・エラーなし [C] |
| `ctest --test-dir build -C Release -R '^(app_disc_id\|app_cd_menu_music\|recomp_disc_ys\|bios_c_ys)$'` | **4 本とも合格**（M1 では前の 2 本が skip だった）。`app_disc_id`：7 つの場合すべて ok。`app_cd_menu_music`：2 つの場合が一致。`recomp_disc_ys`：`55 sessions ... outside the analysis: 10; from loads outside the map: 2`、`RESULT: OK`（上限 10・2 のまま）。`bios_c_ys`：`RESULT: all equal in every session` [C] |
| M1 で確かめた ROM 不要の 13 本 | 13 本すべて合格 [C] |
| 40 本のセッションのヘッダーの解決（`editions.header_path`） | `roms\…`（8 本、BIOS あり）も `D:\Roms\PCECD\YsI_II/…`（32 本）も `roms/usa/…cue`・`roms/bios/…pce` に解決。save も 40 本とも存在。ファイル名が違うもの（`roms\Other (USA).cue`）は置き換えない [C] |
| `python -m py_compile`（変更した 7 本と `run_package_test.py`）、各ツールの `--help` | 成功 [C] |
| 各ツールと `build.sh` の `--edition jp`、`--edition xx` | 「まだない（Phase 3B）」／「不明な版」で終了コード 2 [C] |
| `bash -n scripts/build.sh`、`--help` | 成功 [C] |

- **R24 は閉じる**：ROM があれば `recomp_disc_ys` は元の試験として動き、合格する（Windows は `07a` の C11、macOS は上の表）。ROM がないときは 542cf5a の 77（文だけ直した）。
- **R08 は解消（ツール側）**：`bios_c_ys` が macOS で「偽の skip」ではなく実際に検査して合格した。`check_all_sessions.py`・`check_session.py` は macOS では `ys_trace`・`gg_trace` がまだないので、通しては動かしていない（M3）。
- **R03 は解消**：`roms/usa`・`roms/bios` の配置のまま、ディスクと BIOS が使われる。
- macOS で動かしていないもの：`YsRecomp`・`ys_trace`・生成 C（M3）、`pce_trace` を使うほかの試験（`nobios_ys`、`coverage_ys`、`fade_ys`、`hle_timing_ys`、`equip_*_preview_ys` など。R06 の FMA などは M3 で比べる）、`gg_trace` を使う試験（D5、M3）、`build.ps1`（pwsh なし）。

## 5. 未解決

| ID | 内容 | 段階 |
|---|---|---|
| M2-1 | Windows で M2 の前後の動作が同じこと（6 章） | 実機（確認待ち） |
| M2-2 | `build.ps1 -Edition` の動作（pwsh がないため macOS では未確認） | 実機（6 章の W6） |
| M1-6 | `recomp_lockstep_ys` の 1 回目の失敗（M1 の前から）。M2 では原因を調べていない | 未定 |
| M2-3 | 日本版の変数（`YS_JP_CUE`・`YS_JP_BIOS`）、試験名・ターゲット名の引数化、`profile`・`EQUIP` の引数、`.rc`・パッケージの版ごとの名前 | Phase 3B |
| M2-4 | `equip_preview.py`・`equip_ice_preview.py`・`run_app_start_test.py`・`run_launcher_test.py`・`run_package_test.py`・`make_icon.py` の版の引数（CMake から明示の引数で呼ばれるので M2 では不要） | Phase 3B |
| M2-6 | `run_bios_check.py` をセッションの指定なしで動かすと、BIOS なしで記録したセッション（`# bios: -`、32 本）に当たったところで「missing ... -: test skipped」（77）で止まる。M2 の前からの動作（before と after で同じ）。**M2 の後で解決**：BIOS なしのセッションは比べる相手（本物の BIOS での実行）がないので飛ばし、その本数を表示する。BIOS ありのセッションが 1 本も残らないときは 77（何も比べずに合格にはしない）。macOS では指定なし・`--frames 600` で BIOS ありの 8 本を比べて `RESULT: all equal`（終了コード 0）、BIOS なしだけを指定すると 77、`bios_c_ys` は合格。フレームの制限なしの全セッションは時間がかかるので未実行 | 解決（M2 の後） |
| M2-5 | `equip_ice_preview.py` / `equip_preview.py` の skip の文（`no disc or BIOS in roms\`）は古い言い方のまま（542cf5a の文は CMake 側だけ直した） | 3B でまとめて |

## 6. Windows の実機での確認手順

M1 と同じ実機・同じ手順（`07a` の 3.2：ctest を直接実行、`Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass`）。
ROM と BIOS は今までどおり `roms\` 直下に上流の名前で置いたまま（M2 の既定値はこの場所を最初に見る）。before は `18e1eaf`、
after は M2 のコミット（以下 `<M2>`）。runtime の C は変えていないので、C8（アイコン）と C9（GPU）は不要。

### 6.1 before（`18e1eaf`）

```powershell
cd C:\work\YsRecomp
git fetch; git checkout 18e1eaf; git submodule update
Remove-Item -Recurse -Force build
.\build.ps1
git log --oneline -1; ctest --test-dir build -C Release -N | Select-String "host_layers|Total Tests"
ctest --test-dir build -C Release -N -L usa | Select-String "Total Tests"
ctest --test-dir build -C Release -L quick *> before_test.txt
ctest --test-dir build -C Release -LE history -j 2 --repeat until-pass:2 *> before_testall.txt
ctest --test-dir build -C Release -N | Select-String "Test +#" | ForEach-Object { ($_ -split '\s+')[3] } | Set-Content before_names.txt
python tools\trace\run_bios_check.py games\ys\sessions\session_20261017_161116.txt --routines E009 *> before_bios.txt
```

期待：`18e1eaf`、`host_layers` が出る、`Total Tests: 99`。`-L usa` は `Total Tests: 0`（ラベルがまだない）。

### 6.2 after（`<M2>`）

```powershell
git checkout feature/japanese-disc-support; git submodule update
git log --oneline -1                                   # -> <M2>
Remove-Item -Recurse -Force build
.\build.ps1
cmake -S . -B build | Select-String "YS_USA_"          # cmake が PATH にないときは Build Tools の cmake.exe をフルパスで
ctest --test-dir build -C Release -N | Select-String "host_layers|Total Tests"
ctest --test-dir build -C Release -N -L usa | Select-String "Total Tests"
ctest --test-dir build -C Release -L quick *> after_test.txt
ctest --test-dir build -C Release -LE history -j 2 --repeat until-pass:2 *> after_testall.txt
ctest --test-dir build -C Release -N | Select-String "Test +#" | ForEach-Object { ($_ -split '\s+')[3] } | Set-Content after_names.txt
Compare-Object (Get-Content before_names.txt) (Get-Content after_names.txt)
python tools\trace\run_bios_check.py games\ys\sessions\session_20261017_161116.txt --routines E009 *> after_bios.txt
.\build.ps1 -Package *> after_package.txt
.\build.ps1 -Edition jp *> after_edition_jp.txt
python tools\trace\check_all_sessions.py --help *> after_tools.txt
python tools\trace\run_bios_check.py --edition jp *>> after_tools.txt
```

### 6.3 チェックポイント

| 番号 | 何を見るか | 合格の条件 |
|---|---|---|
| W1 | 構成の表示 | `YS_USA_CUE: C:/work/YsRecomp/roms/Ys Book I & II (USA).cue (SHA-1 ...)`、`YS_USA_BIOS: C:/work/YsRecomp/roms/[BIOS] ... (v2.0).pce (SHA-1 2bea3dac…)`（roms\ 直下が選ばれる。cue の SHA-1 は改行の違いなどで macOS の `ee92126a…` と違ってもよい） |
| W2 | 試験の本数と版のラベル | after も `Total Tests: 99`、`-L usa` は `Total Tests: 81`。`Compare-Object` が何も出さない（名前と本数が同じ） |
| W3 | quick | before・after とも 21 本、失敗は `cpu_huc6280` だけ |
| W4 | 全試験（history 以外） | before・after とも 89 本、最後まで失敗は `cpu_huc6280` だけ。skip は同じ 7 本（`trace_ys_boot`、`trace_ys_run`、`session_ys_boot`、`session_ys_game`、`session_ys_save`、`perf_ys`、`cd_image`）。`bios_c_ys`・`recomp_disc_ys`・`app_disc_id`・`app_cd_menu_music` は合格 |
| W5 | パッケージ | `app_package_ys`、`app_launcher_ys` が合格。`dist\YsRecomp\` と `dist\YsRecomp.zip` ができる（大きさは M1 の after とほぼ同じ） |
| W6 | `-Edition jp` | `Edition 'jp' is not available yet (Phase 3B)` で止まる |
| W7 | ツール | `before_bios.txt` と `after_bios.txt` の表と `RESULT` 行が同じ（macOS では `cd_read $E009 7 7 0 0`、`RESULT: all equal in every session`）。`check_all_sessions.py --help` に `--edition`・`--cue`・`--bios` がある。`run_bios_check.py --edition jp` は `not available yet` |

- `recomp_lockstep_ys` が 1 回目に失敗し 2 回目に合格するのは M1-6（M2 の前から）。判定には使わない。
- 結果のまとめ：

```powershell
Select-String -Path before_*.txt, after_*.txt -Pattern "tests passed|\*\*\*Failed|Not Run|Skipped|failed|RESULT|YS_USA|Total Tests|not available|Package|Zip|usage" |
  ForEach-Object { "$($_.Filename): $($_.Line.Trim())" } | Set-Content -Encoding utf8 m2_results.txt
```

### 6.4 この確認に戻るときのプロンプト

```
CLAUDE.md（上流の本文と作業規則 v2）を読んでください。次に review_portability/09_m2_rom_paths_editions.md を読んでください。

M2 の Windows での確認を、07a の 3 章と同じ Windows 11 x64 の実機で、09 の 6 章の手順で行いました（before は 18e1eaf）。

結果（m2_results.txt の中身、Compare-Object の出力、W1 の構成の表示）：
（ここに貼る）

お願い：
- W1〜W7 のチェックポイントごとに、合格・不合格・判断できない を表にしてください。before と after の両方で失敗した
  試験は実機の環境が原因として分けてください。
- M2 が原因と考えられる違いがあれば、原因を調べ、修正案を出してください。修正は承認を得てから行い、M3 には進まないでください。
- 結果を review_portability/09_m2_rom_paths_editions.md に記録し（5 章の M2-1・M2-2 の状態も更新）、コミットのコマンドを
  示して止まってください。
- roms/ は読み取り専用です。サブモジュール external/PCERecomp に変更が必要になったら、変更せずに止めて報告してください。
- 始める前に、判断が必要な質問をまとめて先に聞いてください。
```
