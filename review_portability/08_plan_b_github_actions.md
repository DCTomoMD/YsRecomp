# B 案：GitHub Actions の Windows x64 で M1 を確かめる

- 作成日：2026-11-19。ブランチ `ci/windows-github-actions`（`bfab31f` から作成）。PCERecomp（`1b9d25a`）は変更していない。
- 表記：[C] 確認済み、[W] push 後に Actions で確かめる、[A] 推測。
- **まだ一度も Actions で実行していない。** 以下の「期待される結果」は、push 後に確かめる [W]。

## 1. 決めたこと（2026-11-19、ユーザー）

| 項目 | 決定 |
|---|---|
| 使う action | `actions/checkout`（v7.0.1 = `3d3c42e5aac5ba805825da76410c181273ba90b1`）、`actions/setup-python`（v7.1.0 = `6d4fd22b20465ccc0f7354476588e8df7c9ca0e0`）、`actions/upload-artifact`（v7.0.2 = `cf430e030ddbb5b0abf93d22962f4752f3646cd9`）。SHA は `git ls-remote --tags` で取得した [C]。CMake と MSVC は runner の Visual Studio 2022 のもの |
| build.ps1 | `-VsPath` 引数を追加。省略時は今と同じ `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools` |
| `cpu_huc6280` | `reference/huc6280-tests` がないので CI では必ず失敗する（終了コード 2、R21）。ダウンロードせず、「前後とも失敗する既知の失敗」として比べる |
| `.rc` | ROM がないと CMake は `YsRecomp` を作らない（`CMakeLists.txt:258`）。CI で `rc.exe` を使い `.rc` だけをコンパイルする |
| macOS の `pce_runtime` | 今回は行わない（M3 で扱う） |

## 2. 中身

| ファイル | 内容 |
|---|---|
| `.github/workflows/windows-ci.yml` | `windows-2022` の 1 つのジョブで、試験対象のコミット（`head`）と基準（`base`、既定 `06b62c6`、手動実行で変更可）を同じ runner で順にビルド・試験して比べる |
| `tools/ci/ctest_report.py` | 2 つの `ctest --output-junit` を比べる。passed（実行して合格）/ skipped（77）/ failed（失敗、または実行できなかった）に分け、skip は別に一覧にする。次の場合は不合格：新しい失敗、新しい試験の失敗、pass→skip、試験の消失、理由のない前後両方の失敗、`--require-pass` の不合格、結果ファイルがない |
| `tools/ci/check_rc.py` | `.res` に、アイコングループ・`.ico` の全画像（1 バイトずつ一致）・版情報があること。`--compare` で M1 の前の `.res` と同じであること |
| `build.ps1` | `-VsPath`（1 の表） |

ワークフローの手順：
1. 2 つのコミットを取得（サブモジュール込み）。ディスク・BIOS・生成コードが Git に入っていないことを確かめる（`.gitignore` の「protected content」と同じ拡張子。上流が管理しているセッションの `.brm` と `roms/README.md` は除く）。
2. Python 3.10（拒否されることの確認用）と 3.14 を入れる。vswhere で VS 2022 とその CMake を探す。
3. `head` は `build.ps1 -VsPath <VS> -Verbose`、`base` は build.ps1 と同じ CMake のコマンド（`base` の build.ps1 は場所が固定のため）。どちらも同じ VS の CMake を使う。
4. 両方で `ctest -C Release -j <コア数/2> --repeat until-pass:2 --output-junit`（build.ps1 と同じ選択肢、定義されたすべての試験）。
5. `ctest_report.py` で比べ、結果をジョブの Summary に出す。`host_layers` は合格が必須。
6. SDL3 の tar.gz の SHA256 が `4c3b0933…2e3d` であること（`head` の構成の `URL_HASH` でも検査済み）。
7. Python 3.10 を `-DPYTHON_EXE` に渡した構成が「Python 3.11 or later needed」で失敗すること（SDL3 はダウンロード済みのものを `FETCHCONTENT_SOURCE_DIR_SDL3` で再利用）。
8. `Launch-VsDevShell.ps1` の環境で、別のフォルダから `.rc` のフルパスを渡して `rc.exe` を実行（ビルドと同じく `.ico` が `.rc` の隣にあることを確かめる）。`base`（`runtime/app/`）と `head`（`runtime/host/win/`）の `.res` を `check_rc.py` で確かめ、両者が同じであること。
9. ログ・JUnit・`.res` を artifact（14 日）に保存。

## 3. macOS で確かめたこと

| 確認 | 結果 |
|---|---|
| ワークフローの YAML（Ruby の YAML で読む） | 読めた。手順 14 個 [C] |
| `py_compile`（2 本） | 成功 [C] |
| `check_layers.py` | `check_layers: OK` [C] |
| `ctest_report.py`：macOS の ctest 4.5.0 の JUnit（`app_config`、`app_disc_id`、`host_layers`、`cpu_huc6280`）で、同じ結果／新しい失敗／pass→skip／試験の消失／理由のない両方の失敗／結果ファイルなし | 終了コード 0／1／1／1／1／1 で、それぞれ正しい理由を出す [C]。JUnit では 77 の skip は `<skipped message="SKIP_RETURN_CODE=77"/>`、実行ファイルがないものは `<skipped message="Unable to find executable"/>` になる。後者は失敗として数える [C] |
| `check_rc.py`：`.ico` から自作した `.res`（rc.exe の出力の形式に合わせたもの）で OK、壊した `.ico` で FAILED（終了コード 1） | [C]。**本物の rc.exe の出力は読んでいない** [W] |
| M1 の前と後の `.ico`、`.rc` | どちらもバイト単位で同じ [C] |

- `build.ps1` の変更は PowerShell がないので macOS では実行していない [W]。

## 4. push 後に Actions で確かめる点と期待される結果 [W]

- ジョブ `windows-x64` が緑。
- Summary の表で、`head` は次のとおり [A]（`ctest_report.py` が数を出す）：
  - passed（実行）：`app_config`、`app_volume`、`app_menu`、`app_video`、`app_image`、`app_crop`、`app_redraw`、`app_fade`、`app_launcher`、`app_input`、`host_layers`（new test）、`pce_memory_io`、`recomp_python`、`tools`
  - skipped（77）：`app_disc_id`、`app_cd_menu_music`（ディスクなし）、`cd_image`（`gg_cdinfo.exe` なし）
  - failed：`cpu_huc6280` のみ（known failure (both)）
  - `base` も同じで、`host_layers` がないだけ。
- 「SDL3-3.2.30.tar.gz SHA256 = `4C3B…2E3D` (expected)」、「Python check: 3.10 refused …」、「Resources: … is the same as before M1」の 3 行が Summary に出る。
- 想定と違う場合：artifact `windows-ci-<sha>` の `head_build.log`、`head_tests.log` を見る。

## 5. この CI で確かめられないもの

- `YsRecomp.exe` のリンク、生成 C（`ys_recomp`）、`ys_trace`、`.rc` を含めた実際のリンク、`make_package.py` のアイコン埋め込み：ROM が必要。A 案（07）で確かめる。
- D3D11 の GPU 計測、DWM の vblank の実行時の動作（ウィンドウが必要）。
- ROM が必要な試験すべて（そもそも CMake が定義しない）。

## 6. 未解決

| ID | 内容 |
|---|---|
| B-1 | `windows-2022` の runner が将来なくなった場合（VS 2022 が入った image を選び直す） |
| B-2 | `.gitmodules` の URL をフォークに替えたら（D1）、そのフォークが公開されていないと `submodules: recursive` が失敗する |
| B-3 | `cd_image` が 77 で skip することは [A]（`run_cd_test.py:151-153` を読んだだけ） |
