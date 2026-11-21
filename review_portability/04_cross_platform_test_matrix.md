# M0-04：クロスプラットフォーム試験マトリクス

- 表記は `00_platform_inventory.md` の 1 章に従う。
- **本書は試験の分類と計画だけを扱う。** 試験の変更も実行もしていない。
- 試験の名前とラベルは `CMakeLists.txt:815-857` と、`review_jp/00` の 11 章による。
- 期待値（PASS_REGULAR_EXPRESSION、セッション、カバレッジ）はすべて北米版専用である [C]。日本版には使えない。

---

## 1. 区分の定義

| 区分 | 意味 | Git に入れるもの |
|---|---|---|
| **public** | ユーザーのディスクも BIOS もいらない。CI でも動かせる | 試験のコードと、ROM に由来しない期待値 |
| **private** | ユーザーが持っている ROM / BIOS が必要。ローカルか Windows VM で動かす | 試験のコード、ボタン入力のセッション、BRAM（上流の方針）。ROM、BIOS、それらから生成したコード・画像・音声は入れない |
| skip（77） | 必要なファイルがない | **合格ではない**（CLAUDE.md の作業規則 v2）。マトリクスでは「未検査」として数える |

---

## 2. 組み合わせ（4 つ）

| 組 | OS | ディスク | BIOS | 状態（M0 時点） |
|---|---|---|---|---|
| W-US | Windows x64（VM） | 北米版 | US System Card 2.0（`--bios` のときだけ） | 上流で動いている [A]。**Windows VM で確認が必要** → **M3 で確かめた**（2026-11-22、Windows 11 の実機：100 本のうち 99 本合格、skip 1 本、失敗 0。M3 の前と同じ。6 章） |
| W-JP | Windows x64（VM） | 日本版 | 日本版 System Card（種類は [U]） | 未対応（Phase 3 以降） |
| M-US | macOS arm64 | 北米版 | US 2.0 | ビルドできない（M3 で新しい基準にする）→ **M3 で基準になった**（2026-11-22：100 本のうち 99 本合格、skip 1 本、失敗 0。6 章） |
| M-JP | macOS arm64 | 日本版 | 日本版 | 未対応（Phase 3 以降） |

---

## 3. マトリクス

凡例：
- ✓：今の試験をそのまま使える見込み
- △：条件付き（備考を参照）
- ✗：今は使えない
- N：その組み合わせ用の新しい試験か参照データが必要
- ―：対象外

### 3.1 public（ROM 不要）

| 試験 | ラベル | W-US | W-JP | M-US | M-JP | 備考 |
|---|---|---|---|---|---|---|
| app_config、app_volume、app_menu、app_video、app_image、app_crop、app_redraw、app_fade、app_launcher | quick | ✓ | ✓（版に依存しない） | △ | △ | macOS ではビルドが通ることが前提（M3）。app_menu は実行ファイルの隣に書く（`app_tests.c:485`） |
| app_input | quick | ✓ | ✓ | △ | △ | 仮想ジョイスティック（`app_tests.c:2914-2925`）が macOS の SDL で動くか [U] |
| PCERecomp の単体試験（cpu、pce、cd、bios、recomp の unittest、tools の `--help`） | console / recomp | ✓ | ✓ | △ | △ | Python 3.11 以上が必要（R01）。cd_image は `gg_cdinfo.exe` を固定（R09）。`cpu_huc6280` は `reference/` がないと失敗する（R21） |
| **新設の候補（M1）** host 層の依存の向きの確認（grep） | quick | N | N | N | N | `02` の 4 章。OS にも版にも依存しない |
| **新設の候補（M3）** host_paths の書き込み先 | quick | N | ― | N | ― | ROM 不要。Windows は実行ファイルの隣、macOS は D3 の決定に従う |
| **新設の候補（Phase 3）** profile の選択（日米の判定） | quick | N | N | N | N | ROM ではなく、合成した小さな CUE で試す（ROM 由来のデータは入れない） |

### 3.2 private（ROM / BIOS が必要）

| 試験 | ラベル | W-US | W-JP | M-US | M-JP | 備考 |
|---|---|---|---|---|---|---|
| app_disc_id、app_cd_menu_music | quick | ✓ | ✗ → N | △ | ✗ → N | disc_id は北米版専用（`disc_id.c:13-15`）。試験は北米版のファイル名を固定（`app_tests.c:3189-3318`） |
| app_start_ys、app_mark_ys、nobios_ys、nobios_cabin_ys | quick / console | ✓ | ✗ → N | △ | ✗ → N | macOS：書き込み先（R05）と、ウィンドウを表示しない Metal 描画（R20） |
| equip_c_ys、equip_preview_ys、equip_ice_preview_ys | quick / console | ✓ | ✗ → N | △ | ✗ → N | 生成ステップで US のセッション 2 本を使う（`CMakeLists.txt:269-276`） |
| recomp_disc_ys、coverage_ys | recomp / console | ✓ | ✗ → N | △ | ✗ → N | 日本版には日本版の disc.toml とカバレッジが必要。macOS：Python 3.11 以上（R01）、`._*`（R15） |
| ys_recomp の生成とコンパイル（ビルドそのもの） | ― | ✓ | N | △ | N | macOS：Clang での時間とメモリ（R04） |
| trace_ys_boot、trace_ys_run、session_ys_boot/game/save、fade_ys、hle_timing_ys | console | ✓（gg_trace が必要） | N | △（gg_trace がないと 77 → 未検査） | N | Geargrafx の参照（R09、D5） |
| bios_c_ys | console | ✓ | N | ✗（偽の skip、R08） | N | ヘッダーの `roms\` パスをツール側で正規化するまで未検査 |
| ロックステップ（recomp ラベル、ys_trace） | recomp | ✓ | N | △ | N | 再コンパイル版とインタプリタを同じ OS の上で比べるので、OS の差を受けにくい [A] |
| 期待値が固定の試験（`layout_switches_ys`、`hud_save_ys`、`battle_ys2` など） | recomp / history | ✓ | ✗（北米版の期待値） | △ | ✗ → N | macOS：FMA（R06）で数値がずれる可能性。D4 で方針を決める |
| history（10 本） | history | ✓ | ― | △ | ― | 北米版の長いセッション。日本版の history は将来の課題 |
| 実時間の試験（`TESTS_TEMPO_REAL`：stall、hiccup、rhythm など） | 各ラベル、RUN_SERIAL | ✓ | N | △ | N | macOS：vblank がスタブ（R11）、Retina（R13）。しきい値は緩めない（規則 6） |
| app_package_ys、app_launcher_ys | package | ✓ | N | ✗（R07） | N | macOS 用のパッケージ試験を別に作る（.app と権限。期待値は同じ考え方で） |

---

## 4. 組み合わせごとの合格の条件（案）

| 組 | 合格とみなす条件 | 前提 |
|---|---|---|
| W-US | 上流の `-TestAll` が全件合格し、skip が上流と同じ件数であること。M1 の前後で結果が変わらないこと | Windows VM、ROM・BIOS を上流の固定名で（M2 からは変数で）置く。gg_trace をビルドする |
| M-US | public がすべて合格する。private のうち recomp・quick・history が合格する。console は D5 しだい。skip は件数と理由を記録し、合格には数えない | M1〜M3 の完了、R01 の解消、D3・D4・D5 の決定 |
| W-JP | 日本版の profile、disc.toml、カバレッジ、セッションによる新しい試験が合格する。北米版の試験も並行して合格し続ける | Phase 2〜3。日本版の参照データ（ROM に由来しないもの）の扱いをユーザーが承認すること |
| M-JP | W-JP と同じ試験が合格する。M-US と同じ方針で OS 差を扱う | 上記すべて |

---

## 5. 実行のしかた（記録のみ。今回は実行していない）

### Windows VM（W-US、上流の手順）
```powershell
git submodule update --init --recursive
.\build.ps1 -Test
.\build.ps1 -TestAll
.\build.ps1 -Package
```
- **Windows VM で確認が必要。**
- 期待結果：ROM と BIOS を置いた場合は全件が合格する [A]。置かない場合は public だけが走り、ROM を使う試験は 77 になる。

### macOS（M-US、M3 以降の想定。現時点では実行できない）
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DPYTHON_EXE="$PWD/.venv/bin/python"
```
```bash
cmake --build build --parallel
```
```bash
ctest --test-dir build -L quick --output-on-failure
```
- 実際のコマンドは M1 の段階 5（`scripts/build.sh` など）と、M2 の ROM 変数で決まる。

### ラベルの案（M2 以降）
- 版のラベル：`usa` / `jp`
- 区分のラベル：`public` / `private`
- 既存のラベル（quick、console、recomp、history、package）に加えて付ける。例：`ctest -L "quick" -L "usa"`
- OS は実行する機械で決まるので、ラベルにしない。

---

## 6. M3 の結果（M-US、2026-11-22）

- 3 章の表は M0 の時点の見込みとして残す（Q30）。実際の結果をこの章に書く。
- 対象：`ecd7442`（PCERecomp `76ab827`）。`reference/` に Geargrafx `1.8.6`、HuC6280 の試験 `4ce378e` を入れた状態。
- 試験ごとの結果と時間：`11_m3_summary.md` の 6 章。Windows の列も埋めた（2026-11-22、`1a54bc7`）。合否は 100 本とも同じ。

### 6.1 3 章の行ごとの結果

| 試験（3 章の行） | M0 の見込み | M3 の結果（M-US） | 備考 |
|---|---|---|---|
| app_config ほか public の app_* | △ | ✓ 合格 [C] | |
| app_input | △（仮想ジョイスティック [U]） | ✓ 合格 [C] | macOS の SDL3 でも仮想ジョイスティックが動いた |
| PCERecomp の単体試験 | △ | ✓ 合格 [C] | `cd_image` はフォークで実行ファイル名を直した（M3-7）。`cpu_huc6280` は `reference/` を入れて合格 |
| host 層の依存の向き（`host_layers`） | N | ✓ 合格 [C] | M1 で新設 |
| `host_paths` | N | ✓ 合格 [C] | M3-4 で新設（Windows：実行ファイルの隣、macOS：Application Support） |
| profile の選択（日米） | N | ― | Phase 3 |
| app_disc_id、app_cd_menu_music | △ | ✓ 合格 [C] | M2 |
| app_start_ys、app_mark_ys、nobios_ys、nobios_cabin_ys | △ | ✓ 合格 [C] | 隠した窓の Metal（R20）で描けた |
| equip_c_ys、equip_preview_ys、equip_ice_preview_ys | △ | ✓ 合格 [C] | |
| recomp_disc_ys、coverage_ys | △ | ✓ 合格 [C] | |
| ys_recomp の生成とコンパイル | △ | ✓ 成功 [C] | 4 分 27 秒、最大 2.7 GB、警告 0。生成 C は Windows と同じ（R25） |
| trace_ys_*、session_ys_*、fade_ys、hle_timing_ys | △（gg_trace が要る） | ✓ 合格 [C] | Geargrafx は `-ffp-contract=off` でビルドしないと音の比較が失敗する（M3-7） |
| bios_c_ys | ✗（偽の skip、R08） | ✓ 合格 [C] | M2 でヘッダーのパスを直した |
| ロックステップ | △ | ✓ 合格 [C] | |
| 期待値が固定の試験 | △（FMA、R06） | ✓ 合格 [C] | 期待値は Windows と同じまま |
| history（10 本） | △ | ✓ 合格 [C] | 727 秒 |
| 実時間の試験（`TESTS_TEMPO_REAL`） | △ | ✓ 合格 [C] | しきい値は緩めていない。`app_prep_ys` はまれに失敗する（M3-5-1、原因は未特定。`--repeat until-pass:2` で実行） |
| app_package_ys、app_launcher_ys | ✗（R07） | ✓ 合格 [C] | `app_package_ys` は macOS 用のスクリプト（`run_package_test_macos.py`、M3-6） |
| perf_ys | （3 章に行なし） | skip（77） | 最適化の前の基準 `build/perf_ref/hle` がない。Windows でも同じ。合格には数えない |

### 6.2 4 章の条件との照合

- M-US の条件「public がすべて合格する。private のうち recomp・quick・history が合格する。console は D5 しだい。skip は件数と
  理由を記録し、合格には数えない」：**満たした** [C]。console も D5（Geargrafx を macOS でも使う）のとおり合格した。
  skip は 1 本（`perf_ys`、理由は上）。
- W-US の条件（上流の全件が合格し、skip が上流と同じ件数）：**満たした** [C]（`11_m3_summary.md` の 5.7）。after の 100 本
  （うち `host_paths` は M3 で足した）のうち 99 本が合格。skip は `perf_ys` の 1 本で、M3 の前（`d0430b1`）と同じ件数。
  M3 の前後で結果は変わっていない。
