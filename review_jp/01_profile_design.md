# Phase 1-01：profile 層（ゲーム版 USA / JP）の設計

- 対象ブランチ：`feature/japanese-disc-support`（M0 完了時点 `b56201d`、PCERecomp `1b9d25a`）
- 作成日：2026-11-19
- 方法：ソースの読み取りと grep のみ。`roms/` は読んでいない。ビルド・コード生成・試験・ダウンロードはしていない。
- **本書は設計である。** 製品コード・CMake・試験・サブモジュールは変更していない。実装は Phase 3A 以降に、ユーザーの承認を得てから行う。
- 関連文書：host 層は `review_portability/05_host_layer_design.md`、CMake・試験・パッケージの構成は `review_jp/02_build_layout.md`。
- 表記：**[C]** 確認済み、**[A]** 推測、**[U]** 不明（`review_jp/00` と同じ）。行番号は `b56201d` 時点。
- **日本版の値（セクタ、アドレス、シグネチャ、RAM、タイル、CRC、フレーム番号）は本書に一つも書かない。** すべて [U] とし、Phase 2 で確定させる（作業規則 v2 の 4）。

---

## 1. ユーザーの決定（2026-11-19）

| ID | 決定 | 本書での扱い |
|---|---|---|
| P1 | 日本版は当面、**実 BIOS 必須**（漢字フォントのため） | 6 章。BIOS なしの日本版は Phase 2 以降に別途検討 |
| P2 | 版の差は**リンク時に選ぶ const 構造体**で表す | 2〜3 章 |
| P3 | 北米版の定数も **profile へ移す**（値は変えない） | 5 章 |
| P4 | 実行ファイルは**版ごとに別名・別フォルダ**（ini、saves、sessions、`*.bin` も別） | 7 章、`02_build_layout.md` |
| P5 | 値が未確認の機能は**機能ごとに無効で起動**し、ログ・文書・メニュー（灰色）に残す | 4 章 |
| P6 | 日本版のオプションメニューとランチャーは**英語のまま** | 7 章 |
| P7 | 日本版のデータは **`games/ys_jp/`** に置く（`games/ys/` は触らない） | `02_build_layout.md` |
| P8 | 文書は版（review_jp）と OS（review_portability）で分ける | 本書と 05、02 |
| D11 | （M0）日米は**別の実行ファイル** | 生成コードの `recomp_lookup` の衝突は起きない（別リンク）[A] |
| P9 | 日本版の実行ファイルの名前は **`YsIandII_JP`**（2026-11-19 追加） | 7 章。`app_id`、ini 名（`YsIandII_JP.ini`）、試験・パッケージの名前もこれに合わせる。ウィンドウのタイトルは Q1 で未決 |

---

## 2. 方針

1. **版の選択はリンク時に 1 回だけ行う。** 実行ファイルごとに `profile_usa.c` か `profile_jp.c` のどちらか一方をリンクする。
   - C のコードに `#ifdef PCE_EDITION_JP` や `if (japanese)` を書かない（作業規則 v2 の 8）。
   - 手本は既存の装備表である [C]：`extern const EquipTable *const g_equip_table;`（`runtime/app/equip.h:68`）を、生成された `equip_frames.c` が定義し、ディスクのない構成では `runtime/app/equip_none.c:5` が NULL で定義する。どちらをリンクするかで決まる。
2. **profile はデータだけを持つ。** 関数ポインタは原則として持たない（処理の違いが必要になった場合は Phase 3 で個別に設計し、ユーザーの承認を得る）。
3. **共通コード（`runtime/app/ys_*.c` など）は profile を読むだけ。** 版を判定する分岐を持たない。値が「ない」ことで機能が止まる（4 章）。
4. **profile は host を知らない。host は profile を知らない。** 版の情報が host に要るときは、main.c が文字列などの素のデータとして渡す（`05_host_layer_design.md` の 3 章）。
5. **PCERecomp は変更しない。** 本設計で使う PCERecomp 側の仕組みは既存のものだけ（`Pce.game_patch`、`on_instruction`、`input_script_*`、`pce_load_cd`）[C]。

---

## 3. 型の設計（案）

### 3.1 置き場所

```
runtime/profile/
  ys_profile.h       types only: plain C (stdint.h), no OS headers, no SDL, no app/ headers
  profile_usa.c      const YsProfile g_ys_profile = { ... }  (values moved from runtime/app, unchanged)
  profile_jp.c       const YsProfile g_ys_profile = { ... }  (Phase 3B: identity only, every feature NULL)
  profile_report.c   ys_profile_report(): one log line per disabled feature (no SDL; prints via a callback)
```

- `profile_report.c` は版によらない共通コード。ログの出力先は呼び出し側（main.c）が関数ポインタで渡す（profile から SDL への依存を作らないため）。

### 3.2 共通の行の型

既存のフック表は、どれもほぼ同じ形をしている [C]：

| 既存の表 | 形 | 場所 |
|---|---|---|
| `ys_engine.c` `k_hooks[]` | `{pc, sig[5]}` | `runtime/app/ys_engine.c:12-20` |
| `ys_windows.c` `k_hooks[]` | `{pc, sig[5], skip}` | `runtime/app/ys_windows.c:12-23` |
| `ys_scroll.c` `k_hooks[]` | `{pc, sig[3]}` | `runtime/app/ys_scroll.c:12-19` |
| `ys_reward.c` `k_hooks[]` | `{pc, bank, kind, n, sig[12], skip}` | `runtime/app/ys_reward.c:10-27` |
| `ys_intro.c` `k_screens[]` | `{addr, screen, op[9]}` | `runtime/app/ys_intro.c:6-29` |
| `tools/trace/ys_actors.c` `k_hooks[]` | ys_engine の 4 行＋2 行 | `tools/trace/ys_actors.c:12-22` |

これを 1 つの型にそろえる（案）：

```c
/* One instruction hook: PC + bank (MPR window), the bytes that must be there, and which bytes to ignore. */
typedef struct {
    uint16_t pc;
    uint8_t  bank;       /* physical bank seen through the MPR; 0xFF = any bank in the module's range */
    uint8_t  kind;       /* module-specific meaning (e.g. reward: kill / gold / exp) */
    uint8_t  n;          /* signature length */
    uint8_t  sig[16];
    uint32_t skip;       /* bit i set = sig[i] is not compared (operands that differ between Ys I and Ys II) */
} YsHookRow;

typedef struct { const YsHookRow *rows; int count; } YsHookSet;
```

- **Ys I と Ys II の対は、今と同じく「行を並べ、合った行が採用される」で表す** [C の現状に合わせる]。ゲーム番号のタグは新しく作らない（`review_jp/00` 10 章：今も「今が Ys II」という状態は持っていない）。
- `sig` の長さ 16 は既存の最大（ys_reward の 12、ys_dialog の 26 は別扱い。3.3 参照）から決める。実装時に最大値を数え直す。

### 3.3 モジュール別のサブ構造体（案）

フィールド名は案。値の出どころ（北米版）は調査済み [C]。日本版の値はすべて [U]。

| サブ構造体 | 持つもの | 北米版の今の場所 |
|---|---|---|
| `YsEngineProfile` | フック 4 行、鉱山の明かり（Ys I）、主人公の生成（Ys I / II）、アクター表と RAM 約 30 か所、バンク範囲 | `ys_engine.c:12-60, 82, 90-188` |
| `YsWindowsProfile` | ウィンドウのフック 6 行、レイアウト切替 4 PC と命令列 2 本、RAM（ウィンドウ、カメラ、文字） | `ys_windows.c:12-65` |
| `YsScrollProfile` | フック 3 行、RAM | `ys_scroll.c:12-29` |
| `YsFadeProfile` | ワイルドカード付きの命令列 2 本（PC 固定なし）、探索するバンク範囲、色の RAM | `ys_fade.c:8, 42-50, 191` |
| `YsScreenProfile` | ローダとブートの PC とシグネチャ、プログラム番号（イントロ、Ys I、Ys II、ロードメニュー） | `ys_screen.c:8-13, 46-58` |
| `YsIntroProfile` | スクリプトとメニューの PC、画面操作の表、RAM | `ys_intro.c:6-29` |
| `YsMusicProfile` | PC 2 つとシグネチャ | `ys_music.c:6-9` |
| `YsDialogProfile` | 速度の RAM、シグネチャ（26 バイト）、テストと待ちの PC、3 つのプログラムの PC | `ys_dialog.c:6-20` |
| `YsPanelProfile` | HUD の VRAM タイル番号、パネルの位置と大きさ、RAM | `ys_panel.h:27-49`、`ys_panel.c:22-24` |
| `YsRewardProfile` | 撃破・GOLD・EXP のフック行、RAM | `ys_reward.c:10-27` |
| `YsEquipProfile` | 装備表の位置（CD RAM）と、Ys I / II を見分ける先頭の並び、装備スロットの RAM、パレット番号 | `equip.c:22, 449-451, 525, 582-586, 650-655` |
| `YsCropProfile` | 画面の幾何（枠、マップ、パネル、ロード画面）。両版で同じ可能性が高いが [A]、ディスク由来なので profile に置く | `crop.c:14-34`、`crop.h:78-89`、`tools/trace/hud_old.c:5-15` |
| `YsMenuFontProfile` | フォントのタイル範囲、矢印タイル、CRC | `menu_font.h:18-22` |
| `YsLauncherSfxProfile` | 効果音 ID 3 つ、RUN を押すフレームの窓、上限フレーム、音源ドライバの RAM | `launcher_sound.c:12, 17-31, 61-78` |
| `YsIntroSkipProfile` | イントロの上限フレーム | `intro_skip.h:35` |
| `YsDiscIdentity` | トラック数、TOC の CRC、データの CRC、タイトル文字列とその位置（なければ NULL） | `disc_id.c:13-15, 104` |
| `YsBiosPolicy` | BIOS の要否（6 章）、受け付ける BIOS の識別値（なければ NULL） | 新設 |
| `YsIdentity` | `edition_id`（`"usa"` / `"jp"`）、`app_id`（書き込み先のフォルダ名など）、ウィンドウのタイトル、ini のファイル名、利用者向けの文言（ディスクの名前、フォルダ選択の見出し、エラー文） | `main.c:161, 182, 218-221, 2555, 485`、`disc_id.c:121-142`、`launcher.c:12` |
| `YsTraceProfile` | トレース拡張だけが使う追加のフックと RAM、HUD の文字の対応（`"H.PEXGOLDM"`） | `tools/trace/ys_actors.c:12-22`、`ys_trace_ext.c:926, 1303-1304, 1374` |

```c
typedef struct {
    const YsIdentity           *identity;    /* never NULL */
    const YsDiscIdentity       *disc;        /* NULL = no disc can be accepted (the player flow stops with a clear message) */
    const YsBiosPolicy         *bios;        /* never NULL */
    const YsEngineProfile      *engine;      /* NULL = feature off (section 4) */
    const YsWindowsProfile     *windows;
    /* ... one pointer per sub-structure in the table above ... */
} YsProfile;

extern const YsProfile g_ys_profile;   /* defined by exactly one of profile_usa.c / profile_jp.c */
```

### 3.4 profile に入れないもの

| 区分 | 例 | 理由 |
|---|---|---|
| コンソールのハードウェアの値 | スプライトの大きさとマスク表（`redraw.c:11-16`、`battle.c:23-24`）、BAT/SAT のマスク、VCE の色マスク、MPR の計算、`RAM()` の $2000 | 版によらない [C]。`PCERecomp/docs/hardware` の事実 |
| PC 側の設定と演出 | crop のモード、HUD の余白、`battle.c` のポップアップの時間、motion の標本化、`launcher_sound.c` の `k_notes` | PC 版の機能そのもの [C] |
| 両作に共通のエンジン特性 | `YS_ACTORS 64`、`YS_WALKERS 48`、`YS_WIN_MAX 5`、`RING 256` | 日本版でも同じかは [U]。**同じと仮定しない**：Phase 2 で確認し、違えば profile へ移す |
| System Card の作業域とエントリ | $2200-$22FF の各変数、`menu_chip.c:6-7` の BIOS エントリ | 北米版 System Card 2.0 の値 [C]。日本版 System Card で同じかは [U]（J10）。確認するまでは日本版では使う機能を無効にする |

---

## 4. 機能の無効化（P5、作業規則 v2 の 10）

### 4.1 しくみ

- **サブ構造体へのポインタが NULL なら、その機能は無効。** 各モジュールの `*_init` か最初のフックで profile を見て、NULL ならフックを登録しない（またはすぐ return する）。
- 今のフックはシグネチャが合わなければ早期 return する [C]（`review_jp/00` 6.1）。そのため**行数 0 の表**でも動作は安全に止まる。ただし「止まったこと」が見えないので、NULL による明示の無効化と 4.2 のログを必ず組み合わせる。
- 既存の「検出できなければ止まる」仕組みはそのまま使う [C]：
  - `g_equip_table == NULL` と `EQUIP_GAME_NONE`（`equip.c:648`、`game_fixes.c:294`）
  - `pce->game_patch = NULL`（`ys_dialog.c:47-50`）
  - `IntroSkip.enabled`、`INTRO_MAX_FRAMES` での打ち切り（`intro_skip.h:33-45`）
  - `menu_font_ready()` と CRC の確認、手描きフォントへの切り替え（`menu_font.c:24`）
  - `LauncherCapture.ok`（`launcher_sound.c:69`）

### 4.2 見えるようにする

| 場所 | 内容 |
|---|---|
| 起動時のログ | `ys_profile_report()` が無効な機能を 1 行ずつ出す。例：`profile jp: feature 'smooth motion' disabled (engine hooks UNKNOWN)` |
| オプションメニュー | 無効な機能の項目を灰色にし、選べないようにする（`runtime/app/menu.c`。設計は Phase 3B） |
| 文書 | `review_jp/` に「日本版で無効な機能」の表を置き、値を確定させるたびに更新する |
| 試験 | 日本版の実行ファイルで、無効な機能の一覧がログに出ることを確かめる試験（ROM 不要。`02_build_layout.md` 4 章） |

### 4.3 機能と必要なサブ構造体の対応（案）

| 機能（ini のキー／メニュー） | 必要なサブ構造体 | 日本版の初期状態 |
|---|---|---|
| 滑らかな動き（`smooth_motion`） | engine、windows、scroll、screen | 無効 |
| クロップ（`crop`、`hud_background`、`crop_hud_names`） | screen、panel、crop、windows | 無効 |
| 滑らかなフェード（`smooth_fades`） | fade | 無効 |
| 装備の表示（`visual_equipment`） | equip ＋ 生成した装備フレーム | 無効（`equip_none.c` をリンク） |
| 文字の一括表示（`instant_dialog`） | dialog | 無効 |
| 撃破のポップアップ | reward、panel | 無効 |
| イントロの短縮（`intro_to_menu`） | intro、intro_skip、screen | 無効（6 章：BIOS ありでは今も無効 [C]） |
| メニュー音楽の扱い（`# menu`） | music | 無効 |
| メニューの効果音（ランチャーで取り出す） | launcher_sfx | 無効（6.3） |
| ゲームのフォントでメニューを描く | menu_font | 無効（手描きの英数字フォントで描く。今の失敗時の動作 [C]） |
| ディスクの確認 | disc | Phase 2 で値が出るまで、日本版はディスクを受け付けない（明示のエラー文を出す） |

- 「無効」にした機能は、ゲーム本体（エミュレートされたコンソール）の動作には影響しない [A]。`instant_dialog` だけはコンソールの状態を変える機能なので、無効が既定でも問題はない。

---

## 5. 北米版の値の移行（P3）

1. **値は一字一句変えない。** 今の `#define`・配列・式の中のリテラルを、そのまま `profile_usa.c` の初期化子へ移す。
2. **モジュールごとに小さく分ける**（上流の作業規則 3：小さなマイルストーン）。順序の案：
   1. 骨格（`ys_profile.h`、`profile_usa.c` が `identity` と `disc` だけを持つ）
   2. `disc_id.c`、`main.c` の文言とタイトル、ini 名
   3. 単純な表から：`ys_music`、`ys_screen`、`ys_scroll`、`ys_reward`
   4. `ys_windows`、`ys_intro`、`ys_dialog`、`ys_fade`
   5. `ys_engine`、`ys_panel`、`crop`、`equip`、`menu_font`、`launcher_sound`、`intro_skip`
   6. トレース拡張（`tools/trace/ys_*.c`、`hud_old.c`）
3. **各段で同一動作を確かめる**（どれも Windows VM で確認が必要）：
   - `.\build.ps1 -TestAll`（北米版の期待値はそのまま。緩めない、skip を足さない）
   - `app_prep_ys`（`--prep-hashes`：準備した各フレームのハッシュ）
   - 移行の前後で、`profile_usa.c` の各値と元のリテラルが同じことを機械的に確かめる案：移行前のソースから値を抜き出す小さな Python スクリプトで一覧を作り、移行後の `profile_usa.c` と突き合わせる（Phase 3A で作る。ROM は使わない）
4. M3 の後は macOS でも同じ試験を回す（M-US が新しい基準）。

---

## 6. BIOS 必須（P1）の流れ

### 6.1 今の動作 [C]

| 点 | 内容 | 場所 |
|---|---|---|
| プレイヤーの流れの条件 | ROM も BIOS も指定がないときだけ、ini・フォルダ探索・ディスク確認・ランチャーが動く | `main.c:2547`（`if (!rom_path && !bios_path)`） |
| BIOS の読み込み | `--bios` があれば `pce_load_cd(bios, cue)`、なければ合成 BIOS と C の BIOS（HLE） | `main.c:126-136` |
| BIOS の識別 | 実 BIOS の CRC や版は確かめない。そのまま読む | `PCERecomp/runtime/pce/pce.c:96-121` |
| イントロの短縮 | BIOS を指定すると無効 | `main.c:2860-2862` |
| ランチャーの効果音の取り出し | 見えないコンソールを合成 BIOS で動かす | `launcher_sound.c:46`（`pce_load_cd(p, NULL, c->cue)`） |
| 漢字 | HLE の `ex_getfnt` は全角の記号・数字・英字だけ。かなと漢字は空白 | `PCERecomp/runtime/bios/hle_font.c:1-12` |

### 6.2 設計（案）

- `YsBiosPolicy.required = 1`（日本版）／ `0`（北米版。今と同じ）。
- 日本版のプレイヤーの流れ：
  1. ini に新しいキー `bios_file`（ユーザーの System Card のパス）を持つ。
  2. 未設定か読めなければ、ファイル選択のダイアログで BIOS を選ばせる（SDL3 の `SDL_ShowOpenFileDialog`。フォルダ選択の既存の実装 `main.c:163-196` と同じ形）。
  3. `YsBiosPolicy` に識別値があれば照合する。日本版の System Card の種類と CRC は [U]（J10）なので、Phase 2 で値が出るまでは照合せずに読み、**照合していないことをログに出す**。
  4. `pce_load_cd(bios, cue)` で起動する（今の `--bios` と同じ経路）。
- **`--bios` と「プレイヤーの流れ」の結び付きを切り離す**のが main.c 側の主な変更になる。北米版の動作（BIOS なしが既定、`--bios` で流れが外れる）は変えない。
- BIOS のファイルやフォントを**ソースにも生成物にも埋め込まない**（作業規則 v2 の絶対規則 3）。BIOS は実行時にユーザーのファイルから読むだけ。

### 6.3 BIOS 必須で影響を受ける機能

| 機能 | 日本版での扱い |
|---|---|
| イントロの短縮 | 無効（BIOS ありでは今も無効 [C]。System Card の画面が先に出るため） |
| ランチャーの効果音の取り出し | 無効。実 BIOS で取り出す方法は Phase 3 以降に設計する（`launcher_sound.c` は合成 BIOS 固定 [C]） |
| `nobios_*` 系の試験 | 日本版には作らない（P1） |
| BIOS なしの日本版 | Phase 2 以降の課題。案：ユーザーの BIOS ファイルから実行時にフォントだけを読む。これは PCERecomp の HLE に手を入れる可能性が高く、D1（フォークで管理）の手順に従ってユーザーが決める |

---

## 7. 版ごとに分ける利用者向けのもの（P4、P6）

| 項目 | 北米版（今の値 [C]） | 日本版 |
|---|---|---|
| 実行ファイル名 | `YsRecomp` | `YsIandII_JP`（ユーザーの決定 P9） |
| ウィンドウのタイトル | `"Ys Book I & II"`（`main.c:161`） | 案：`"Ys I & II (Japan)"`（ASCII。ユーザーの確認が必要） |
| ini のファイル名 | `YsRecomp.ini`（`main.c:2555`） | `YsIandII_JP.ini`（P9 の名前に合わせた案） |
| 書き込み先 | 実行ファイルの隣（Windows）。macOS は `Application Support/<app_id>`（D3） | 同じ規則で別のフォルダ。版の名前は host に文字列で渡す（`05` の 3 章） |
| `saves/`、`sessions/`、`menu_font.bin`、`launcher_sfx.bin` | ini のあるフォルダ（`main.c:245, 2592, 2596, 2019, 2761`） | 日本版の ini のあるフォルダ。北米版とは混ざらない |
| ディスク・BIOS を選ぶときの文言 | `"(USA)"` を含む英語（`main.c:182, 218-220`、`disc_id.c:121-142`） | 英語（P6）。文言は `YsIdentity` に置く |
| メニュー・ランチャーの言語 | 英語（`docs/ROADMAP.md:17`） | 英語（P6）。日本語化は将来の課題として分ける |
| ランチャーの表示 | 3 作のカバーと発売年（`launcher.c:9-14`） | 同じでよいか、日本版の表示にするかは Phase 3B で決める（画像はユーザーのもので、Git には入れない） |
| `.rc`／Info.plist の製品名・アイコン | `ysrecomp.rc:5, 17-27` | CMake の `configure_file` に版の変数を渡して作る（`05` の 5 章） |
| セッションのヘッダー | `# Session recorded by YsRecomp ...`、`# game:`、`# bios:`（`main.c:484-487`） | 8 章 |

---

## 8. セッションと版

- 録画のヘッダーは YsRecomp 側（`main.c:484-487` の `g_rec.header`）で作っている [C]。ここに `# edition: <edition_id>` の 1 行を加える（PCERecomp の `input_script_*` は変えない）。
  - 既存の北米版のセッションには、この行がない。**行がなければ `usa` とみなす**（既存のセッションは書き換えない。M0 の D7 と同じ考え方）。
- 再生（`--input`）とツール（`check_session.py`、`check_all_sessions.py` など）は、セッションの版と実行ファイルの版が違えば**拒否する**（黙って進めない）。
- 日本版のセッションは `games/ys_jp/sessions/` に置く（P7）。中身はボタンの入力と BRAM で、ROM 由来のデータは含まない [A]（Phase 3D で確認する）。

---

## 9. 依存の規則と機械的な確認

| 禁止 | 理由 |
|---|---|
| `runtime/profile/` → `app/`、`host/`、SDL、OS のヘッダー | profile はデータだけ。版の差は OS に依存しない |
| `runtime/app/`、`main.c` の中の `edition_id` の文字列比較や版の分岐 | 作業規則 v2 の 8。違いは値で表す |
| `#ifdef` による版の選択 | 選択はリンクで行う |
| `runtime/host/` → `runtime/profile/` | host は版を知らない |
| PCERecomp への Ys の持ち込み | 上流の規則 |

確認の案（Phase 3A で quick ラベルの試験にする候補。今回は作らない）：
- `runtime/profile/` に `grep -E '#include *[<"](SDL|windows|app/|host/)'` → 0 件
- `runtime/app/`、`runtime/main.c` に `grep -E 'edition_id *[=!]=|strcmp\([^)]*edition|PCE_EDITION'` → 0 件
- `runtime/host/` に `grep -E 'profile/|ys_profile|g_ys_profile'` → 0 件
- `external/PCERecomp` に `grep -E '\bYs\b|ys_|app/|Adol'` → 0 件（上流の規則の確認）

---

## 10. UNKNOWN（Phase 2 で確定させるもの）

| # | 内容 | 使う profile のフィールド | 関係する `review_jp/00` の項目 |
|---|---|---|---|
| U1 | 日本版の CUE と BIN の構成、FILE 名の文字コード | `disc` | J1、R16 |
| U2 | ディスクの識別値（トラック数、TOC の CRC、データの CRC）と、タイトル文字列の有無と位置 | `disc` | J1、J12 |
| U3 | IPL、ローダ、ファイル表、far call、family（`games/ys_jp/disc.toml` の全値） | （disc.toml） | J2、J3 |
| U4 | 3.3 章の各フックの PC、バンク、シグネチャ、RAM | `engine`、`windows`、`scroll`、`fade`、`screen`、`intro`、`music`、`dialog`、`reward` | J4 |
| U5 | テキストエンジン、文字コード、使う BIOS のフォント | （BIOS なしの検討） | J5 |
| U6 | HUD のタイル配置、メニュー用 8×8 フォントのタイルと CRC | `panel`、`menu_font`、`crop` | J6 |
| U7 | プログラム番号と Ys I → II の移行 | `screen`、`equip` | J7 |
| U8 | CDDA と ADPCM の割り当て、効果音 ID とフレーム | `music`、`launcher_sfx` | J8 |
| U9 | BRAM のエントリ名 | （セーブの確認） | J9 |
| U10 | 日本版 System Card の種類・版・CRC・エントリ点。$2200-$22FF の作業域と BIOS エントリが北米版 2.0 と同じか | `bios`、`menu_chip`、`launcher_sfx` | J10 |
| U11 | `Pce.region_japan`（`PCERecomp/runtime/pce/pce.h:62`。読まれる `pce.c:254` が、どこからも設定されない）を日本版で 1 にする必要があるか。1 にするなら、どこで設定するか（コンソールの構成であり、PC 側の機能ではない点を `PCERecomp/docs/hardware` で確かめる） | `bios` か新しいフィールド | J11 |
| U12 | エンジン特性（`YS_ACTORS`、`YS_WALKERS`、`YS_WIN_MAX`、`RING`）と画面の幾何が日本版でも同じか | `engine`、`crop` | ― |
| U13 | `fix_window`（北米版で文が長くなったことへの対処）が日本版で要るか | （game_fixes） | 7.3 |

## 11. ユーザーに確認が必要な事項（Phase 2 の前後で）

| ID | 内容 |
|---|---|
| Q1 | 日本版のウィンドウのタイトル（7 章の案：`"Ys I & II (Japan)"`）。実行ファイル名は `YsIandII_JP` に決定（P9） |
| Q2 | 日本版のランチャーの表示（北米版と同じ 3 作のカバーか、日本版向けか） |
| Q3 | BIOS なしの日本版を目標に残すか（6.3。PCERecomp の変更を伴う可能性が高い → D1 の手順） |
