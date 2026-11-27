# YsRecomp — Ys Book I & II recompiled for PC (built on PCERecomp)

## Two projects

- **This folder (`D:\Projetos\YsRecomp`)**: the PC version of Ys (runtime/app, `main.c`, disc map, sessions, game tests,
  package). Paths in this file are relative to it unless they start with `PCERecomp/`.
- **`D:\Projetos\PCERecomp`**: the emulated console (`runtime/{cpu,pce,cd,bios,util}`), the recompiler (`tools/recomp`),
  the generic trace tools (`tools/trace`, `tools/cd`, `tools/reference_trace`), `docs/hardware` and `reference/`.
  The CMake here includes it (`PCERECOMP_DIR`: submodule `external/PCERecomp` or the sibling folder); Python scripts find
  it through `tools/pcerecomp_dir.py`. It builds and tests on its own: `..\PCERecomp\build.ps1 -Test`.
- The old repository (`D:\Projetos\pce_recomp`) is archived (it keeps the full Portuguese history, `docs/historico.md`).
  Both new ones go to the user's personal GitHub. **The AI never commits nor runs `git init` in them**: at the end of each
  step, give the user the commands to commit in both folders.

## Goal

Read the binary code of PC Engine / TurboGrafx-16 games and generate C code that, compiled with a runtime, runs the game
natively on PC (Windows). The generated C mimics the CPU instruction by instruction: it need not be readable, it must be
**correct**.

- Final target: an **enhanced PC version of *Ys Book I & II*** (CD-ROM²). The player only points at the bin/cue folder,
  **no BIOS file** (System Card routines rewritten in C). Enhancements: options menu, XInput, >60 FPS (smooth motion),
  border crop (Fill/Wide/HUD), smooth fades, CRT, equipment visible on Adol, instant dialog, launcher.
- During development the real BIOS also runs (`--bios`), as a reference checked against Geargrafx.

## About the user (important)

- Web developer; **does not write this project's code** (the AI writes everything). Does not know assembly, graphics or
  console hardware. **Speaks Portuguese: always talk to the user in Portuguese**, in plain language. Code, comments,
  docs and commit messages are in English.
- Role: set goals, run the game, describe what was seen, approve milestones.
- When a task is done, tell **exactly what to run and what should be seen**.

## Work rules

1. **Nothing moves forward without automatic verification.** Every milestone ends with a test that proves it works.
2. **Do not invent hardware details.** Check `PCERecomp/docs/hardware/INDEX.md` and `PCERecomp/reference/Geargrafx/src/`;
   if it is not there, tell the user and get the documentation first. Geargrafx is GPL-3: reference only, never copy.
3. **Small milestones.** One `docs/ROADMAP.md` step at a time; update its status at the end.
4. **Never version ROMs, ISOs, BIOS or code generated from them.**
5. One commit per milestone that passes the tests (made by the user, with the commands the AI gives).
6. Correctness before performance.
6b. **Complete solutions, no guessing or hacks** (user directive): every game quirk is solved by reading from the game
   itself what it does (hooks, disassembly, RAM), not by recognizing it in the image or patching case by case.
7. **Token economy:**
   - One conversation per milestone: when a milestone is done (commit commands given), say it is time for a new
     conversation and **always** give the ready-to-paste prompt (in Portuguese). Default: "Leia o CLAUDE.md e o
     docs/ROADMAP.md. Próximo passo: o primeiro marco ainda não concluído. Antes de programar, faça de uma vez as
     perguntas de decisão que precisar. Siga as regras de economia de tokens do CLAUDE.md. Ao terminar: rode os testes do
     grupo certo, atualize o roadmap e o docs/devlog.md, me dê os comandos do commit, me diga exatamente o que rodar e o
     que devo ver/ouvir, e me dê o prompt para a próxima conversa." Everything the next one needs must be in CLAUDE.md,
     ROADMAP.md or `docs/`.
   - Before investigating, look in `docs/` and ROADMAP.md for what was already found. Finished milestones: one or two
     lines each in `docs/devlog.md` (grep it, don't read it whole).
   - Prefer Grep and reading excerpts (offset/limit) over whole files; never re-read a file just edited.
   - Images: look as little as possible; use contact sheets (`PCERecomp/tools/trace/contact_sheet.py`) and numbers
     (`lit_pixels.py`, CRCs, `--hashes`) before opening PNGs.
   - Command output always filtered (tail/grep/Select-String), never the whole log.
   - Ask decision questions BEFORE programming.
   - Text with backslashes: only via Edit/Write (or a Python script written with Write), never via shell heredoc/sed.
   - Never rewrite files with PowerShell 5.1 `Get-Content`/`Set-Content`: it breaks UTF-8. Use Edit/Write or Git Bash `sed`.
   - `LNK1104`/.exe locked in `build\Release` may be the antivirus (Kaspersky) holding the file: find out who holds it,
     don't touch the antivirus.

## Where things are

| What | Where |
|---|---|
| Recompiler (Python) | `PCERecomp/tools/recomp/` |
| Console runtime (CPU, video, sound, CD, BIOS in C) | `PCERecomp/runtime/` |
| PC version (settings, disc, volumes, menu, intro, window) | `runtime/app/`, `runtime/main.c` |
| Game config, generated C (not versioned), recorded sessions | `games/ys/` (`disc.toml`, `sessions/`, `coverage/`) |
| Roadmap and status | `docs/ROADMAP.md`; finished milestones: `docs/devlog.md` |
| Ys research (memory, windows, scroll, fades, masks, sound, intro, disc) | `docs/ys_notes.md`; BIOS routines: `docs/ys_bios.md` |
| Hooks that read the game (engine, windows, scroll, fades, masks) | `runtime/app/ys_*.c`; what is read vs. still guessed: `docs/technical_notes.md` |
| Trace/image tools | generic: `PCERecomp/tools/trace/README.md` (`pce_trace` has an extension hook, `pce_trace.h`); Ys ones: `tools/trace/README.md` (`ys_trace_ext.c` = the Ys options of `pce_trace`/`ys_trace`, `check_session.py`, `check_all_sessions.py`, `run_bios_check.py`, `run_perf_check.py`, `run_fade_test.py`). All answer `--help` |
| Equipment on Adol (Python rules, HTML preview, frame generator; in game: `runtime/app/equip.c`) | `tools/equip/` |
| External sources (Geargrafx, HuC6280 tests, Mednafen) | `PCERecomp/reference/` (outside Git; restore: `PCERecomp/docs/hardware/INDEX.md`) |
| User's ROMs, disc and BIOS | `roms/` (never versioned) |

## Build and test

- Windows 11, PowerShell (Bash also available), Python 3.14, VS 2022 Build Tools (MSVC); the Build Tools CMake is not in
  PATH, `build.ps1` calls it. SDL3 3.2.30 is downloaded by CMake on the first build.
- `.\build.ps1` builds (Release, in `build\`). Quiet: only warnings/errors and the test summary; `-Verbose` shows all.
- `build.bat`（2026-11-22 追加）：ダブルクリックで `build.ps1 -Package` を実行し、最後に窓を止める。引数があれば
  そのまま `build.ps1` に渡す。改行は CRLF（`.gitattributes` の `*.bat text eol=crlf`）。
- **The user plays, records sessions and debugs with `dist\YsRecomp\`**: after any runtime change, run
  `.\build.ps1 -Package` and tell the user to run `dist\YsRecomp\YsRecomp.exe` (their sessions: `dist\YsRecomp\sessions\`).
- `.\build.ps1 -Package` tests (label `package`) and builds the player package in `dist\` (`YsRecomp\` + `.zip`; readme
  in `package/README.txt`, icon from `tools/package/make_icon.py`). Everything builds with `/MT`.
- `.\build.ps1 -Test` = label `quick` (~20 s, every change). `.\build.ps1 -TestAll` = everything but `history`, in
  parallel (~11 min, 88 tests; ~6 min are the real-time ones, one at a time): before committing milestones that touch the
  simulated console or the recompiler. `.\build.ps1 -TestHist` = label `history` (~6 min, 10 tests: old bugs of smooth
  motion, crop and sound in long sessions). During work: `ctest --test-dir build -C Release -L console|-L recomp|-R name`.
  A new test goes in a label (LABELS at the end of `CMakeLists.txt`); tests that measure real time or count what the
  window drew go in `TESTS_TEMPO_REAL` (run alone). No test shows a window or plays sound: ctest sets
  `PCE_HIDDEN_WINDOW=1` and `SDL_AUDIO_DRIVER=dummy`; set both when running an .exe by hand to check. `build.ps1` reruns
  once what fails.
- Geargrafx tests reuse its output while game, BIOS, script and `gg_trace` don't change (`PCERecomp/tools/trace/gg_cache.py`;
  `PCE_GG_CACHE=0` forces a rerun). A session's image and sound are checked in all frames at once (`run_session_test.py`,
  tests `session_ys_*`). The recompiled code is checked in lockstep against the interpreter (and the interpreter against
  Geargrafx): don't repeat traces/images of the recompiled code against Geargrafx. Window tests that don't measure time use
  `--turbo`.
- Changing `PCERecomp/tools/recomp/` or the disc map (`games/ys/disc.toml`) regenerates and recompiles the Ys C (~12 min):
  run it in the background. Runtime-only changes rebuild in ~12 s: the generated C (`ys_recomp`) is kept out of LTCG on
  purpose (with it, each link took ~10 min). Don't turn it back on.

## Running

- Player: `build\Release\YsRecomp.exe` with no arguments reads `YsRecomp.ini` next to the .exe (`game_folder`, volumes,
  `chip_filter`, `intro_to_menu`; display: `fullscreen`, `resolution`, `refresh_rate`, `vsync`, `window_scale`; image:
  `integer_scale`, `filter`, `crt`, `crt_brightness`, `aspect`, `crop`, `hud_background`, `crop_hud_names`; smooth motion:
  `smooth_motion` (1 = on, default: `runtime/app/motion.c`, assembled on the GPU by `runtime/app/gpu_draw.c`, frame redraw
  in `runtime/app/redraw.c`; fixes of game defects in `runtime/app/game_fixes.c`; F6 / `--motion-cpu`: GPU <-> CPU);
  `smooth_fades` (`runtime/app/fade.c`); `visual_equipment` (`runtime/app/equip.c`); `instant_dialog` (the only option
  that changes the game: `runtime/app/ys_dialog.c`, `Pce.game_patch`; the session notes `# dialog`); `pause_in_background`;
  `record_sessions=1` records every session in `sessions\` next to the .ini; its `# sound:` line notes sound failures),
  checks the disc, asks for the folder if needed, and opens the **launcher** (covers of the three Ys CD games: I & II, III, IV; Ys sound effects cached in
  `launcher_sfx.bin` next to the .ini; images in `launcher\` next to the .exe or `docs/`; `--launcher` forces it,
  `--launcher-test FOLDER` presses keys by itself, `--launcher-wav FOLDER` writes the effects and music to WAV; Esc/SELECT
  opens the options menu). Saves in `saves\` next to the .ini. `-Package` keeps the .ini, `saves\` and `sessions\` of
  `dist\YsRecomp\`. Esc = menu (Video; Image; Audio; Controls), blocked during the game's intro (`--menu-press` tests it).
  Alt+Enter = fullscreen. `--config FILE` uses another .ini (window tests: set `fullscreen=0`).
- Development: `YsRecomp.exe "roms\Ys Book I & II (USA).cue"` (no BIOS) or with
  `--bios "roms\[BIOS] TurboGrafx CD System Card (USA) (v2.0).pce"` (real BIOS; also for `pce_runtime`, `pce_trace`,
  `gg_trace`). `--gg-mix` = sound mixed as in Geargrafx. `--vrr` = V-Sync off (G-Sync/FreeSync). `--frame-timing` measures
  how long each frame stayed on screen (the user's monitor: 2560x1440, 180 Hz, locked at 120 Hz in Windows/driver, G-Sync;
  the window reads the real Windows rate, `runtime/host/win/display_timing_dwm.c`; pacing: single clock, `runtime/app/pacing.c`;
  `PCE_PACE_LOG=1`). `--screen-dump FROM COUNT FOLDER` saves what the window showed (BMP; with the options menu open,
  `m<frame>_<n>.bmp`); `--window-size W H` opens the window at that size. `--audio-wav FILE` records the sound sent to the
  device (not with `--turbo`, which is silent). `--audio-hiccup MS` stops the sound device MS ms every 5 s.
- Sessions: `--record games\ys\sessions` records the buttons (rewritten every minute; F12 marks the frame on screen in the
  `# marks (F12):` line); `--input <session>` replays; check all new ones at once: `python tools\trace\check_all_sessions.py`
  (lockstep; with BIOS, also Geargrafx); one against Geargrafx: `python tools\trace\check_session.py <session.txt>`.
  A session that started from a save carries its `.brm` (`# save:` line, `--bram` in the tools). Game save:
  `saves\<cue name>.brm` (`--save FILE`).
- Check the C BIOS routines against the real one: `python tools\trace\run_bios_check.py` (`--routines E009,E012|all`).
- Ys C = **all the code on the disc** through the map (`games/ys/disc.toml`; `docs/ys_notes.md`, "Disc files and loader"),
  independent of the sessions. Coverage (`games/ys/coverage`) only checks: `python ..\PCERecomp\tools\recomp\recomp.py disc
  <bios> --cd <cue> --map games\ys\disc.toml --check games\ys\coverage [-v]` (test `recomp_disc_ys`). Record coverage of a
  new session (only if `--interp-log` shows missing code): `pce_trace --frame --coverage FILE.txt --input <session>
  [--bram <save>] [--bios ...] <cue> <frames> x.ppm`. What falls to the interpreter: `ys_trace ... --interp-log FILE`.
- Variables: `PCE_CD_LOG=1` (SCSI commands, ADPCM fade and rate), `PCE_AUDIO_LOG=1` (sound queue failures),
  `PCE_RING_LOG=FILE` (sprites that vanish around shrunk windows; the windows come from the game, `app/ys_windows.c`).

## Code conventions

- File paths are UTF-8: open with `u8_fopen` (`runtime/util/u8file.h`), not `fopen`.
- **Nothing about Ys in PCERecomp** (no code, no mention in its comments and docs; Ys only in its README, as the project
  that uses it). Game-specific things stay here and plug in through generic hooks: `pce_trace` extension (`TraceExt`),
  session events (`input_script_record_event`/`input_script_event`: `# menu`, `# dialog`), disc-map rules configurable in
  `disc.toml` (`script`, `routine_records`), `Pce.game_patch`, `Vdc.line_log`/`write_hook`. Check with Grep
  (`\bYs\b|ys_|app/|Adol`) in PCERecomp.
- Nothing that is "PC-side" touches the simulated console (exceptions noted in the session: menu music, `# menu`; instant
  dialog, `# dialog`). Sound mix: `Pce.mix` zeroed = Geargrafx sum (traces and tests); the window uses the Mednafen
  balance times the volumes (`PCERecomp/runtime/pce/mix.h`).
- New frame prepared on its own thread (`runtime/main.c`, "New-frame preparation"): fixes, crop, fades, bands, equipment,
  smooth motion and texture images go to a slot (`PrepSlot`); the window only uploads textures and draws reading the slot
  on screen (`ds`). State carried between frames (`g_prep`) only under `g_prep.lock` (order: `g_prep.lock`, `frame_lock`,
  `slot_lock`); no SDL on the prep thread. `--prep-inline` = the window prepares; `--prep-hashes FILE` writes one hash per
  prepared frame (test `app_prep_ys`: both ways equal). With F6 (motion on the CPU) the window prepares.
- Menu (`runtime/app/menu.c`) drawn over the image, game paused (`g_emu.paused`), with the game's 8x8 font copied from VRAM
  (`runtime/app/menu_font.h`, `menu_font.bin` next to the .ini).
- User directive: **the HUD and the menus (including the options menu) always get the image filters** (sharp/smooth,
  CRT) and the options menu has the game menu's scale (with the crop: `present_overlay`/`present_layer`).


---

> 以下は feature/japanese-disc-support ブランチの作業規則。上記の上流の記述と矛盾する場合は、以下を優先する。

# YsRecomp 日本版対応・クロスプラットフォーム化プロジェクト（作業規則 v2）

## 目的
- 北米版 Ys Book I & II（TurboGrafx-CD）専用の YsRecomp に、
  日本版 PC Engine CD-ROM²「イースI・II」への対応を追加する。
- 単一の共通ソースから macOS（Apple Silicon）と Windows x64 の両方を構築できるようにする。
- 北米版の既存動作を壊さない。
- 上記の上流 CLAUDE.md の規則（特に「PCERecomp に Ys 固有のものを入れない」
  「PC 側はエミュレートされたコンソールの状態を変えない」「ファイルは u8_fopen で開く」
  「ロックの順序」）は引き続き守る。開発環境に関する記述（Windows 11 / PowerShell）だけは本規則で置き換える。

## 開発環境（macOS 先行）
- 主開発機：macOS / Apple Silicon（arm64）、APFS ボリューム、パスに空白なし。
- 上流 YsRecomp は Windows / MSVC / PowerShell 前提。M3 完了まで macOS ではゲーム本体をビルドできない。
  ビルドできない状態で「動作確認済み」と報告してはならない。
- Windows での検証は、ユーザーが後日 Windows の実機でまとめて行う（2026-11-28 の方針。それまでは macOS の実機で
  開発を先に進める）。各段では Windows で実行するコマンドと期待結果を文書に残し、「Windows で確認待ち」の一覧に足して、
  止まらずに次の承認を待つ。Windows で確かめていないことを「Windows で確認済み」と書かない。
  確認待ちの一覧：review_jp/05 の 14.4（3A 全体）と、3B 以降の各段の結果の節。push すれば CI（Windows x64、ROM の
  要らない試験だけ）が動くので、その結果があれば報告に書く。
  実機での確認は区切り（3B の終わりなど）でまとめて行うのを勧める。遅らせるほど、壊れた段を探す手間が増える。
- Python は `.venv/bin/python` を使う（tomllib のため 3.11 以上が必要。上流は 3.14 を前提）。
  `brew install` と `pip install` は実行前に必ず確認を求める。
- シェルコマンドは macOS（BSD）互換で書く。
  - `sed -i ''` を使う（GNU の `sed -i` は不可）。
  - `find` の括弧は `\(` `\)` とエスケープする。
  - GNU 専用オプション（`grep -P`、`readlink -f` など）を使わない。
- ファイル名の Unicode：macOS は NFD で保存することがある。
  CUE 内の FILE 名とディスク上のファイル名は NFC に正規化して比較する
  （現状のコードには正規化処理がない。review_jp/00 の 3.1 節と 17 章を参照）。
- `._*` と `.DS_Store` は、ソース列挙・生成物・パッケージから必ず除外する。

## ダウンロードとネットワーク
- 調査だけを行う Phase（Phase 0、Phase 1、Phase 2、M0）では、ダウンロードとビルドを行わない。
- ビルドを行う Phase では、CMake の FetchContent による SDL3（バージョン固定）の取得だけを許可する。
  それ以外の依存の追加・更新は、事前にユーザーの承認を得る。

## サブモジュール external/PCERecomp
- PCERecomp の変更が必要になった場合は、変更せずに停止して報告する。
  運用方法（上流へ提案する／自分のフォークに切り替える／パッチとして管理する）はユーザーが決める。
- 承認された後も、PCERecomp に Ys 固有のコードを入れない（上流の規則）。
- 運用：ユーザーのフォーク `DCTomoMD/PCERecomp` を使う（`.gitmodules` の `url`）。上流 `Fabiano-Gelain/PCERecomp` は
  サブモジュールの remote `upstream`。最初の変更は、`pce_trace` が画像を書けないときに `could not write the image <パス>`
  を出すもの（ブランチ `trace-image-write-message`。M1-6 の調査で、終了コード 1 だけで理由が出ないことが分かったため）。

## roms/ の配置
- ユーザーの配置：roms/usa（北米版）、roms/jp（日本版）、roms/bios（System Card）。読み取り専用、Git 管理外。
- M2 で、ディスクと BIOS をキャッシュ変数 `YS_USA_CUE`・`YS_USA_BIOS` で指定できるようにした（`cmake/ys_edition.cmake` の
  `ys_rom_file`）。空のとき（既定）は、上流の場所（`roms/Ys Book I & II (USA).cue`、
  `roms/[BIOS] TurboGrafx CD System Card (USA) (v2.0).pce`）にあればそれを使い、なければ `roms/usa/` と `roms/bios/` の
  同じ名前のファイルを使う。どれを使ったかは構成時に `-- YS_USA_CUE: ...`（BIOS は SHA-1 付き）と出る。
  Python のツールも同じ規則で選ぶ（`tools/editions.py`）。日本版（3B-1）は `YS_JP_CUE`・`YS_JP_BIOS`。
  空のときは `roms/jp/Ys I & II (Japan).cue` と `roms/bios/syscard3.pce` だけを見る（上流の場所はない）。
- roms/ の中身は移動・リンク・改名しない（読み取り専用）。

## フェーズ順序（macOS 先行）
1. 調査：Phase 0（完了）と M0（移植性）。どちらも読み取りのみ。
2. 設計・解析：Phase 1（Plan mode）、Phase 2（日米ディスク解析）。
3. Mac 版を動かす：M1 → M2 → M3。北米版を macOS で動かし、これを新しい基準にする。
4. 日本版の実装：Phase 3A〜3D。
5. 統合試験：Phase 4（macOS と Windows の実機の両方）。
6. 監査：Fable 5.2（別の worktree で実施）。

## 役割
- Opus 5.5：調査、設計、実装、試験、監査指摘の再検証、修正。
- Fable 5.2：独立監査のみ。この規則の方針を前提にしない。
- ユーザー：各 Phase の承認者。コミットはユーザーが行う。

## 絶対規則
1. 指示された Phase だけを実施し、終了条件を満たしたら停止する。次の Phase へは承認なしに進まない。
2. git commit / tag / push / reset --hard / clean を実行しない。
3. roms/ と local_analysis/、および BIN/CUE/ISO/BIOS・音声・画像・文章データを Git に追加しない。
   ROM と BIOS に由来する文章・フォント・画像をソースコードに埋め込まない
   （日本版の漢字フォントは System Card に由来するため、特に注意する）。roms/ は読み取り専用として扱う。
4. sector / offset / load address / RAM address / hook target を推測で決めない。
   北米版の値を日本版に流用しない。未確認の値は UNKNOWN として記録し、実装を止める。
5. 確認済みの事実（CONFIRMED）、推測（ASSUMPTION）、不明（UNKNOWN）を必ず区別して書く。
6. テストを通すために期待値を緩めたり、skip を追加したり、例外を握りつぶしたりしない。
7. 変更箇所以外を一括でフォーマットしない。ついでのリファクタリングもしない。
8. ゲーム版の差（USA / Japan）は profile に、OS の差（Windows / macOS）は host 層に置く。
   両者を同じ条件分岐に混ぜない。`if (japanese)` を各所に散在させない。
9. ゲームコアは Objective-C++ にしない。`.mm` は macOS host 層の必要な箇所に限る。
10. フックのシグネチャが合わないと機能は黙って無効になる。日本版で無効になった機能は、
    無効であることをログと文書に必ず残す。

## ディレクトリ
- roms/usa, roms/jp, roms/bios：ユーザー所有のディスク（読み取り専用、Git 管理外）
- local_analysis/：個人的な解析結果やハッシュ値（Git 管理外）
- review_jp/：日本版対応の文書（Phase 0〜4）。00_repository_inventory.md が基礎資料
- review_portability/：移植・共通化の文書（M0 以降）
- review_fable/、review_fable_final/：Fable の監査報告（Opus は上書きしない）
- review_opus/：Fable の指摘の再検証と修正の記録

## profile 層（Phase 3A、2026-11-28 に macOS で完了。Windows の確認は後日の実機で、手順は review_jp/05 の 14.4）
- ディスクに由来する値と、版の名前・文言は `runtime/profile/` に置く：`ys_profile.h`（型と、コンパイル時の定数）と
  `profile_usa.c`（`const YsProfile g_ys_profile`。北米版の値）。各モジュールは `g_ys_profile.<module>` を読み、版を尋ねない。
  実行ファイルは profile をちょうど 1 つリンクする（`ys_profile_usa`。日本版の `profile_jp.c` は Phase 3B）。
- サブ構造体：`identity`・`disc`・`music`・`screen`・`scroll`・`reward`・`windows`・`intro`・`dialog`・`fade`・`common`
  （2 つ以上のモジュールが読む RAM）・`engine`・`panel`・`equip`・`menu_font`・`launcher_sfx`・`intro_skip`・`trace`
  （`tools/trace/` だけが読む値）。RAM は CPU から見た番地（`0x2C41` の形）。2 バイト目以降は「先頭 + n」で読む。
- 画面の幾何と配列の大きさは、`ys_profile.h` の末尾の定数（`YS_IMG_*` など）。両方の版で同じと仮定している [A]。
- コードに残した値（System Card の作業域 `$2200-$22FF`、エンジンの特性、ハードウェアなど）と、日本版での確かめ方は
  review_jp/05 の 14.2。
- 値を移したら、T1 で同じことを確かめる：`.venv/bin/python tools/profile_values.py <直前のコミット> [--allow V:N] [--new V:N]
  [--ram OFF:N] [--dim V:N] [--as OLD:NEW:N]`。引数ごとの理由を文書に書く（書き方は review_jp/05 の 10.2〜13.2）。
  値をわざと変えると差として出ることも確かめる。

## ビルドと試験
出典：review_jp/00_repository_inventory.md の 2 章、11 章、18 章（上流の README と build.ps1 から書き出したもの。未検証）。

### Windows（実機、上流の手順）
前提：Windows 10/11 x64、VS 2022 Build Tools、Python 3.14、Git。初回の構成時に SDL3 を取得するためネットワークが必要。
roms/ 直下に北米版の cue と bin 43 本、System Card (USA) v2.0 を上流の固定名で置く。

    git submodule update --init --recursive
    .\build.ps1            # Release ビルド。初回は C 生成に約 12 分
    .\build.ps1 -Test      # quick ラベル（約 20 秒）
    .\build.ps1 -TestAll   # history 以外の全試験（約 11 分）
    .\build.ps1 -TestHist  # history ラベル
    .\build.ps1 -Package   # dist\YsRecomp\ と dist\YsRecomp.zip
    ctest --test-dir build -C Release -R <name> --output-on-failure

- ROM や BIOS がない試験は終了コード 77 で skip になる。skip は合格ではない。
- 試験の期待値（PASS_REGULAR_EXPRESSION、セッション、カバレッジ）は北米版専用。日本版には使えない。

### macOS
- 状態（M3 完了、2026-11-22）：北米版のゲーム本体（生成 C、`ys_trace`、`YsRecomp.app`、パッケージ）は macOS arm64 で
  警告なしにビルドでき、試験 100 本のうち 99 本が合格、skip 1 本（`perf_ys`：最適化の前の基準 `build/perf_ref/hle` が
  ない。Windows でも同じ）、失敗 0。これが macOS の基準 M-US。`app_prep_ys` はまれに失敗する（M3-5-1、原因は未特定。
  失敗すると前後の記録を出す）。まとめ：review_portability/11_m3_summary.md（経過は 10_m3_plan.md）。
- 前提：Apple Silicon、macOS 13 以上（`.app` の最低の版。開発機は macOS 27）、Xcode Command Line Tools（Apple Clang）、
  Homebrew の `cmake` と `ninja`、`.venv` の Python 3.11 以上（開発機は 3.14）。SDL3 3.2.30 は初回の構成で CMake が取る。
  ディスクと BIOS は `roms/usa/`・`roms/bios/`（「roms/ の配置」の節）。
- 手順：`scripts/build.sh`（build.ps1 と同じ手順とオプション）。生成器は Ninja Multi-Config で、実行ファイルは Windows と
  同じ `build/Release/` に出る（拡張子なし。ツールは `tools/build_paths.py` で名前を決める）。開発機での時間の目安：
    scripts/build.sh              # ビルド。初回は生成 C の生成とコンパイルで約 5 分（同時 4 本、最大 2.7 GB）
    scripts/build.sh --test       # quick ラベル
    scripts/build.sh --test-all   # history 以外の全試験（約 19 分、-j 4）
    scripts/build.sh --test-hist  # history ラベル（約 12 分）
    scripts/build.sh --package    # dist/YsRecomp/（YsRecomp.app と README.txt）と dist/YsRecomp.zip
    scripts/build.sh --target app_tests   # そのターゲットだけ（--verbose：すべて表示、--edition usa|jp|all）
- プレイヤー：`build/Release/YsRecomp.app`（`$<TARGET_FILE>` は `Contents/MacOS/YsRecomp`。試験はそれを動かす）。
  設定・セーブ・セッション・`menu_font.bin`・`launcher_sfx.bin` は `~/Library/Application Support/YsRecomp/`
  （`host_paths`。Windows は実行ファイルの隣のまま）。試験は環境変数 `PCE_USER_DIR` で一時フォルダに向ける（本物に触れない）。
  ランチャーの画像とアイコンは `.app` の `Contents/Resources/`。全画面は Cmd+Ctrl+F（Alt+Enter も使える。設定は窓に合わせて
  変わる）。ad-hoc 署名だけなので、配った zip の `.app` は初回に右クリック→「開く」か、システム設定の
  「プライバシーとセキュリティ」で「このまま開く」が要る（`package/README-macOS.txt`）。
- Geargrafx との比較：`external/PCERecomp/reference/` に Geargrafx のタグ `1.8.6`（`3d6345c`。`1.8.7` 以降は PCERecomp の
  ソースの一覧に `load_detector.cpp` がなくリンクできない）、HuC6280 の試験 `4ce378e`、Mednafen 1.32.1 の 3 ファイルを置く
  （取り方：PCERecomp の `docs/hardware/INDEX.md`。Geargrafx は `--branch 1.8.6`）。`gg_trace`・`gg_cdinfo` は
  `external/PCERecomp/tools/reference_trace/build.sh` で 1 回作る（Windows は `build.ps1`。MSVC 以外は `-ffp-contract=off` が
  必須で、CMake が付ける。ないと音の比較が失敗する）。
- macOS の設定は `cmake/platform_macos.cmake`：arm64 だけ、`-ffp-contract=off`（Windows と同じ浮動小数点の結果）、生成 C の
  同時のコンパイルを 4 本まで（job pool）、SDL3 の非推奨 API の警告は SDL3 だけで抑える、`.app` の組み立て（`Info.plist` は
  `runtime/host/mac/Info.plist.in`、`.icns` は `make_icon.py --iconset` と `iconutil`、`codesign --sign -`）。
- host 層の macOS の実装は `runtime/host/mac/paths_mac.c`（書き込み先）だけ。vblank の時刻・GPU 時間・優先度は null のまま
  （R11、R12、M1-3。M3 の範囲外）。
- OS の差は `cmake/platform_{windows,macos}.cmake` と `runtime/host/{win,mac,null,common}` に置く。
  層の規則は試験 `host_layers`（`tools/check_layers.py`、quick）が確かめる。
- 版の差は `cmake/ys_edition.cmake` の `ys_edition()` と `tools/editions.py` に置く。版の試験にはラベル（`usa`）が付く。
  `build.ps1 -Edition usa|jp|all` / `scripts/build.sh --edition ...`（既定 `all`）は、ほかの版の試験を `-LE` で除く
  （`-L` を 2 つ並べると AND になり、版のない共通の試験が外れるため）。`jp`（3B-1）は、ディスクの地図
  （`games/ys_jp/disc.toml`）がまだないので、生成 C も実行ファイルもなく、試験は起動の煙試験 `jp_boot_ysjp`（`console`。
  `tools/trace/run_boot_test.py`：System Card 3.0 で日本版が起動して新しいゲームまで進む）だけ。`--package --edition jp` は
  「まだない」で終わる（`all` は北米版だけを作る）。`ys_edition()` の中のターゲット名・試験名はまだ北米版のもの。

### CI（GitHub Actions、Windows x64）
- `.github/workflows/windows-ci.yml`：試験対象のコミットと基準（`06b62c6`）を同じ runner でビルドし、ROM 不要の試験を
  比べる（`tools/ci/ctest_report.py`。77 の skip は別に数える）。`.rc` は `rc.exe` で単独に確かめる（`tools/ci/check_rc.py`）。
  build.ps1 の `-VsPath` は CI 用（省略時は今と同じ）。詳細：review_portability/08_plan_b_github_actions.md。

### 基準
- 基準コミット：local_analysis/BASELINE_COMMIT.txt（7452c1b）
- PCERecomp：local_analysis/BASELINE_SUBMODULES.txt（1b9d25a）

## 報告
- 文書と報告は日本語で書く。コード中のコメントと識別子は英語で書く。
- 各 Phase の終了時に、変更ファイル一覧、git diff --stat、実行したコマンド、試験結果、
  未実施の項目（特に Windows の実機での確認待ち）、未解決事項を報告する。

## セッション再開時
最初に git status、git log --oneline -15、review_* 配下を確認し、
現在の Phase と次の作業だけを報告する。確認が終わるまでファイルを変更しない。
