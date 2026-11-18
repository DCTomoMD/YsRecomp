# M0-03：macOS リスク登録簿

表記は `00_platform_inventory.md` の 1 章に従う。「Phase」は、そのリスクを扱う段階の案。

---

## 1. リスク一覧

| ID | 内容 | 根拠（file:line） | 影響 | 確度 | 分類 | 対策（案） | Phase |
|---|---|---|---|---|---|---|---|
| R01 | `.venv` の Python が 3.10.7 で、`tomllib` がない。`recomp.py`、disc.toml の読込、Python の試験が動かない | `.venv/bin/python3` → pyenv 3.10.7、`discmap.py:30`、`config.py:29` | macOS での生成と試験がすべて止まる | [C] | **BLOCKER** | 3.11 以上で `.venv` を作り直す（ユーザー判断 D2）。CMake で版を確認する | M1 の段階 0 |
| R02 | CMake が PATH 上の `python` / `python3` を使う。3.11 以上である保証がない | `CMakeLists.txt:144`、`PCERecomp/CMakeLists.txt:92` | 生成が `tomllib` のエラーで失敗する | [C] | CMake差分 | `-DPYTHON_EXE=.venv/bin/python` を指定し、構成時に版を確認する | M1 |
| R03 | ROM と BIOS の固定名が `roms/` 直下。ユーザーの配置（`roms/usa` など）と合わない | `CMakeLists.txt:24-25, 240` | YsRecomp、ys_trace と 67 本の試験が定義されない | [C] | **BLOCKER**（M2 で解消予定） | キャッシュ変数にする | M2 |
| R04 | 生成 C（約 40 MB を 24 本）の Apple Clang でのコンパイル時間とメモリが不明 | `codegen.py:142-208`、`CMakeLists.txt:256-263` | ビルドが現実的な時間で終わらないおそれ。メモリ不足 | [U] | UNKNOWN | M3 で 1 本だけ試し、時間と最大メモリを測る。必要なら `-O1`、PARTS を増やす、並列数を制限する | M3 |
| R05 | 書き込み先が実行ファイルの隣。`.app` の中では書けないか、署名が壊れる | `main.c:1947, 2553-2556, 245, 2592, 2596, 2019` | 設定とセーブが保存されない。署名の検証に失敗する | [C]（.app での挙動は [A]） | macOS hostが必要 | `host_paths` で書き込み先を分ける（D3） | M1（関数の新設）／ M3（macOS の方針） |
| R06 | Apple Clang の arm64 は FMA 縮約が既定で有効。音声の数値と `ys_trace_ext.c` の数値が Windows とずれる可能性 | `psg.c:58, 73, 279`、`mix.c`、`cdaudio.c:114-168`、`ys_trace_ext.c` | 期待値が固定の試験（PASS_REGULAR_EXPRESSION）が macOS だけ失敗する。Windows とのビット一致が崩れる | [A] | CMake差分 | `-ffp-contract=off` を付けるかを決める（D4）。M3 で比べる | M3 |
| R07 | パッケージの試験が macOS では必ず失敗する（アイコン埋め込みの確認、`.exe` の名前、zip を展開すると実行権限が消える） | `make_package.py:19, 37-44, 65, 69`、`run_package_test.py:66-72` | `package` ラベルが通らない | [C] | **BLOCKER**（macOS の package） | パッケージの作り方を OS ごとに分ける（.app、icns、Info.plist）。zip の代わりに権限を保つ形式を使う。macOS 用の試験を別に作る（期待値は緩めない） | M3 以降 |
| R08 | セッションのヘッダーに `roms\…` や `D:\Roms\…` が書かれており、macOS ではパスが解決できない。`bios_c_ys` は常に 77 で「偽の skip」になる | `run_bios_check.py:64-66`、`check_all_sessions.py:60, 66`、`games/ys/sessions/*.txt` | 試験が何も検査しないまま終わる | [C] | **BLOCKER**（macOS の console） | ツール側で `\` を `/` に置き換え、M2 の ROM 変数から解決する（`run_perf_check.py:39` に前例がある）。セッションのファイルは書き換えない（D7） | M2 |
| R09 | `gg_trace`（Geargrafx の参照）が macOS ではビルドされていない。`reference/` も Git の管理外 | `CMakeLists.txt:44`、`run_session_test.py:36-38`、`PCERecomp/tools/reference_trace` | console の試験が macOS ではすべて 77 で skip になる | [C] | macOS hostが必要 ／ UNKNOWN（Geargrafx が macOS でビルドできるかは不明） | macOS では Geargrafx との比較をしない（Windows だけで行う）か、reference をビルドする（D5） | M3 |
| R10 | 2 次スレッドのスタックが小さい（macOS の既定は 512 KB [A]）。game スレッドで巨大な生成関数を動かす | `main.c:2897`、`codegen.py` | スタックの溢れでクラッシュするおそれ | [U]（C の再帰はない [C]） | UNKNOWN | M3 で生成関数のフレームの大きさを測る。必要なら SDL の stacksize プロパティを使う | M3 |
| R11 | vblank の時刻が macOS では得られない（スタブ） | `vblank.c:36-47`、`main.c:1099-1120, 3192-3195`、`pacing.c:57` | 滑らかな動き（60 FPS を超える表示）の pacing の精度が落ちる。動作は継続する | [C] | macOS hostが必要（任意） | まずは null のまま、無効であることをログに残す。M3 以降に CAMetalDisplayLink などを検討する | M3 以降 |
| R12 | GPU 時間の測定（`--frame-timing`）が macOS では無効 | `gpu_time.c:165-187` | 開発用の測定ができない | [C] | macOS hostが必要（任意） | 無効であることをログと文書に残す | M3 以降 |
| R13 | Retina：`SDL_WINDOW_HIGH_PIXEL_DENSITY` を指定していない | `main.c:2709`（`HIGH_PIXEL` は grep で 0 件） | 低い解像度で描いて拡大するので、CRT やシャープなフィルタの見た目が変わる。`--window-size` や `--screen-dump` の画素数が変わる | [A] | macOS hostが必要（調整） | M3 で、画面のダンプと window の試験で確かめる | M3 |
| R14 | SDL_Renderer の Metal バックエンドで、独自ブレンドモードの動作が D3D11 と同じか不明 | `gpu_draw.c:55, 62`、`present.c:308` | 滑らかなフェード、帯、CRT の見た目の違い | [U] | UNKNOWN | M3 で `--prep-hashes` と画面ダンプを Windows と比べる | M3 |
| R15 | `._*` / `.DS_Store` が列挙に混ざる | `recomp.py:68`（PCERecomp）、`code_same.py:49`、`py_same.py:49`、`run_package_test.py:38, 55, 89`、`run_menu_under_test.py:48`、`run_menu_chip_test.py:65` | 試験の失敗とツールの例外（特に Windows VM との共有フォルダ経由の場合） | [C] | PCERecomp側の修正が必要 ／ ツールの修正 | ドットで始まる名前を除外する（YsRecomp 側は M1 の段階 4。PCERecomp 側は D1） | M1 / D1 |
| R16 | 日本語の CUE の FILE 名：Python は latin-1 で読み、C はバイト列のまま比べる。どちらも NFC へ正規化しない | `cdimage.py:45, 50-52`、`cdimage.c:65-72, 197-200` | 日本版の bin が見つからない（日本版の CUE の中身は [U]） | [C]/[U] | PCERecomp側の修正が必要（必要なら） | Phase 2 で日本版の CUE を調べてから決める | Phase 2 / 3 |
| R17 | SDL3 の取得に `URL_HASH` がない | `CMakeLists.txt:51-53` | 取得したものの同一性を検証できない。供給網のリスク | [C] | CMake差分 | SHA256 を固定する（D6。依存の変更ではないが、承認を得てから行う） | M1 |
| R18 | 単一構成のジェネレータで `CMAKE_BUILD_TYPE` を指定しないと、最適化も LTO もない | `CMakeLists.txt:58`、`PCERecomp/CMakeLists.txt:23` | 遅い。実時間の試験が失敗する | [C] | CMake差分 | 既定値を Release にする。build.sh で明示する | M1 |
| R19 | `stb_image.h` を SDL のソースの内部（`src/video`）から使う | `CMakeLists.txt:97`、`launcher_image.c:12-26` | SDL の更新で消えたり移動したりする可能性（OS とは無関係） | [A] | PORTABLE（3.2.30 では [U]） | M3 でコンパイルして確かめる | M3 |
| R20 | `PCE_HIDDEN_WINDOW=1` で、ウィンドウを表示しないまま Metal で描けるか不明 | `CMakeLists.txt:854-857` | macOS で window の試験が動かないおそれ | [U] | UNKNOWN | M3 で確かめる | M3 |
| R21 | `cpu_huc6280` の試験に `reference/huc6280-tests/v1` が必要。ないと 77 ではなく 2（失敗）を返す | `PCERecomp/CMakeLists.txt:96`、`huc6280_tests.c:376` | macOS で `ctest` を全部回すと必ず 1 件失敗する（OS とは無関係） | [C] | **BLOCKER**（全試験の合格） | `reference/` を復元する（D5）。skip にはしない（規則 6） | M3 |
| R22 | `check_all_sessions.py` の `tasklist` が macOS では例外になり、「起動中」と判断されて新しいセッションを飛ばす | `check_all_sessions.py:41-45` | 手動の確認ツールが正しく動かない | [C] | Windows hostへ隔離 | Windows のときだけ `tasklist` を使い、macOS では `pgrep` を使う | M1 の段階 4 |
| R23 | `PCE_DOCS_DIR` にソースの絶対パスを埋め込む | `CMakeLists.txt:301` | 配布した .app の中から開発者のパスを参照する（launcher の画像の予備の場所） | [C] | CMake差分 | パッケージでは Resources を使い、絶対パスには頼らない | M3 |
| R24 | `recomp_disc_ys` に `SKIP_RETURN_CODE` がない | `CMakeLists.txt`（recomp_disc_ys の定義） | ROM がないときに失敗になるかは recomp.py しだい | [U] | UNKNOWN | 規則 6 に従い、skip は追加しない。M2 で ROM のパスを直したあとに確かめる | M2 |
| R25 | 改行：Windows の生成コードとテキスト出力は CRLF、macOS は LF | `codegen.py:283, 294`、`make_package.py:69` | 生成物のハッシュを OS をまたいで比べると一致しない | [C] | PORTABLE（比べ方に注意） | 比べるときは改行を正規化する | M3 |

---

## 2. 重大リスクの上位 10 件

1. **R01**：`.venv` が Python 3.10.7。macOS での生成と試験がすべて止まる。**BLOCKER**
2. **R03**：ROM の固定名。ゲーム本体と試験の大半が定義されない。**BLOCKER**（M2）
3. **R04**：生成 C の Clang でのコンパイル時間とメモリが不明。macOS 版が成り立つかに直結する。UNKNOWN
4. **R05**：書き込み先が実行ファイルの隣。.app ではセーブが保存されない
5. **R06**：FMA 縮約で音声と数値の結果がずれ、期待値が固定の試験が macOS だけ失敗する
6. **R08**：セッションのヘッダーの `\` パスで「偽の skip」になる。**BLOCKER**（検査されないまま終わる）
7. **R07**：パッケージの試験が macOS では成り立たない。**BLOCKER**（package）
8. **R09**：macOS に Geargrafx の参照がなく、console の試験がすべて skip になる
9. **R10**：2 次スレッドのスタック。クラッシュの可能性。UNKNOWN
10. **R14 / R13**：Metal と Retina での描画の違い（滑らかなフェード、CRT、画素数）

---

## 3. BLOCKER の一覧

| ID | 止めるもの | M1 を止めるか | 解消する段階 |
|---|---|---|---|
| R01 | macOS での生成と Python の試験すべて | 止めない（M1 は Windows の動作を変えない整理で、macOS ではビルドしない）。ただし M1 の段階 0 で解消するのが望ましい | M1 の段階 0（ユーザー判断 D2） |
| R03 | ゲーム本体、ys_trace、67 本の試験（全 OS） | 止めない | M2 |
| R07 | macOS の `package` ラベル | 止めない | M3 以降 |
| R08 | macOS の `bios_c_ys` など（偽の skip） | 止めない | M2 |
| R21 | macOS で全試験が合格になること（OS とは無関係） | 止めない | M3（D5） |

**ソースコードの中に、macOS への移植を不可能にする BLOCKER は見つからなかった** [C]（コンパイルでの確認は M3）。

---

## 4. ユーザーが決める必要がある事項

| ID | 決めること | 選択肢 | 判断材料 |
|---|---|---|---|
| D1 | PCERecomp を変える必要が出たときの運用 | A：上流へ提案する ／ B：自分のフォークに切り替える ／ C：パッチとして管理する | `01` の 9.3。M1 の範囲では変更は不要の見込み |
| D2 | `.venv` を作り直す Python の版 | 3.11（`/usr/local/bin/python3.11`）／ 3.14（`/opt/homebrew/bin/python3.14`。上流と同じ）／ そのほか | 上流は 3.14 を前提にしている。`.venv` の作り直しは `pip install` を伴わない（標準ライブラリだけで足りる [A]） |
| D3 | macOS での配布の形と、書き込み先 | ① 素の実行ファイルとフォルダ（Windows と同じポータブル形式）／ ② .app ＋ `~/Library/Application Support/YsRecomp`（`SDL_GetPrefPath`）／ ③ .app の隣に書く（非推奨） | .app にすると署名と Gatekeeper の扱いが必要。ポータブル形式なら変更は少ない |
| D4 | `-ffp-contract=off` を付けて、Windows とのビット一致を優先するか | 付ける ／ 付けない（macOS 専用の期待値を作る） | 規則 6（期待値を緩めない）を考えると、付ける方が試験を共有しやすい [A] |
| D5 | macOS で Geargrafx（`reference/`）との比較試験を行うか | macOS でも `reference/` をビルドする ／ 比較は Windows VM だけで行う | Geargrafx は GPL-3 で参照専用。macOS でビルドできるかは [U] |
| D6 | SDL3 の取得に `URL_HASH`（SHA256）を固定するか | 固定する ／ しない | 依存の追加ではない。ただし CMake の変更なので承認が必要 |
| D7 | セッションのヘッダーにある `\` パスの扱い | ツール側で正規化する（推奨）／ セッションのファイルを書き換える | セッションは試験の資産。書き換えると履歴との比較に影響する |
| D8 | `._*` と `.DS_Store` を、共有される `.gitignore` に追加するか | 追加する ／ `.git/info/exclude` のままにする | 共有すると Windows 側の作業者にも効く |
| D9 | macOS の最低対応バージョン | 例：macOS 13 ／ 14 ／ 15 | CAMetalDisplayLink は 14 以降 [A]。SDL3 の要件も確認が必要 |
| D10 | macOS 用のビルド手段 | `scripts/build.sh` ／ `CMakePresets.json` ／ 両方 | build.ps1 と同じ機能（-Test、-TestAll、-Package）が必要 |
| D11 | （Phase 1）日米を 1 本の実行ファイルにするか、版ごとに分けるか | 1 本（生成器に接頭辞を付ける。PCERecomp の変更 → D1）／ 版ごとに別の実行ファイル | OS の問題ではないが、CMake とパッケージの構成に影響する |

### 4.1 ユーザーの決定（2026-11-19）

| ID | 決定 | 反映 |
|---|---|---|
| D1 | **自分のフォークで管理する**（案 B） | フォークの作成と `.gitmodules` の URL の変更はユーザーが行う。AI は push しない。PCERecomp に Ys のものを入れない規則は続けて守る |
| D2 | **Python 3.14** | `.venv` を `/opt/homebrew/bin/python3.14`（3.14.8）で作り直した（`--clear`）。旧 `.venv` には pip と setuptools しか入っておらず、失ったパッケージはない。`tomllib` の import を確認した。R01 は解消 |
| D3 | **.app ＋ `~/Library/Application Support`** | `host_paths` の macOS の既定値を `SDL_GetPrefPath` にする（M1 の段階 3 で関数を作り、M3 で macOS の既定値を入れる）。Windows は今と同じ |
| D4 | **`-ffp-contract=off` を付ける**（互換性を重視） | 自前のコード（YsRecomp と PCERecomp のターゲット）に付ける。生成コードにも付ける。libm（`sin`、`cos`、`pow`。`psg.c:35, 315-316`、`adpcm.c:79-80`、`synth.c`）の結果は OS で最後の桁が違いうるので、完全一致は M3 で比べて確かめる |
| D5 | **macOS でも Geargrafx と比べる試験を行う** | M3 で `reference/` と `gg_trace` を macOS でビルドする（GPL-3：参照専用で、配布物には入れない）。`GG_TRACE` の `.exe` の固定は M2 で直す |
| D6 | **固定する** | URL を GitHub の自動生成 tar.gz から SDL の公式リリースのファイルに替え、`URL_HASH SHA256=` を付ける。ハッシュの値は M1 の作業の中で、ダウンロードの承認を得てから得る（公式が公開する値を使えるなら、それを使う）。→ **M1 で実施**：公式の `SDL3-3.2.30.tar.gz` と、リリース API の sha256 で固定（`06_m1_host_skeleton.md` 2.2） |
| D7 | **ツール側で正規化する**（必要な修正は行ってよい） | M2 で、ROM のパスの変数化と同時に `run_bios_check.py`、`check_all_sessions.py` を直す。セッションのファイルは書き換えない |
| D8 | **共有の `.gitignore` に追加する** | `.gitignore` に `._*` と `.DS_Store` を追加した |
| D9 | **Apple Silicon のみ**（Intel Mac は対象外）。最低の OS バージョンは使うライブラリで決める | arm64 だけでビルドする（universal にしない）。最低バージョンは M3 で SDL3 などの要件から決める |
| D10 | **開発用のビルドはシェルスクリプト**。ターミナルなしで入れる要望は**プレイヤー向け** | 開発用：M1 の段階 5 で `scripts/build.sh` を作る。プレイヤー向け：.app を zip か DMG で配り、「アプリケーション」フォルダへドラッグして入れる（M3 以降） |
| D12 | **署名なしで配る**（Apple の署名・公証はしない） | 初回は Gatekeeper に止められるため、「右クリック→開く」（またはシステム設定の「このまま開く」）の手順を package の README に書く。コード署名は ad-hoc（`codesign -s -`。arm64 では必須 [A]）にとどめる |
| D11 | **版ごとに別の実行ファイル**。日本版を Win/Mac（特に Mac）で動かすことを優先する | 生成器のシンボルの衝突（`recomp_lookup`）は、版ごとに別のターゲットにすれば起きない [A]。PCERecomp の生成器を変える必要はなくなる見込み |
