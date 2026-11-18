# YsRecomp 日本版対応・クロスプラットフォーム化プロジェクト

## 目的
- 北米版 Ys Book I & II（TurboGrafx-CD）専用の YsRecomp に、
  日本版 PC Engine CD-ROM²「イースI・II」への対応を追加する。
- 単一の共通ソースから macOS（Apple Silicon）と Windows x64 の両方を構築できるようにする。
- 北米版の既存動作を壊さない。

## 開発環境（macOS 先行）
- 主開発機：macOS / Apple Silicon（arm64）、APFS ボリューム、パスに空白なし。
- 上流 YsRecomp は Windows / MSVC / PowerShell 前提。M3 完了まで macOS ではゲーム本体をビルドできない。
  ビルドできない状態で「動作確認済み」と報告してはならない。
- Windows での検証は、ユーザーが Windows VM 上で行う。
  Windows のビルド・試験が必要な場合は、VM で実行するコマンドと期待結果を提示して停止する。
- Python は `.venv/bin/python` を使う。`brew install` と `pip install` は実行前に必ず確認を求める。
- シェルコマンドは macOS（BSD）互換で書く。
  - `sed -i ''` を使う（GNU の `sed -i` は不可）。
  - `find` の括弧は `\(` `\)` とエスケープする。
  - GNU 専用オプション（`grep -P`、`readlink -f` など）を使わない。
- ファイル名の Unicode：macOS は NFD で保存することがある。
  CUE 内の FILE 名とディスク上のファイル名は NFC に正規化して比較する。
- `._*` と `.DS_Store` は、ソース列挙・生成物・パッケージから必ず除外する。

## フェーズ順序（macOS 先行）
1. 調査：Phase 0（日本版）と M0（移植性）。どちらも読み取りのみ。
2. 設計・解析：Phase 1（Plan mode）、Phase 2（日米ディスク解析）。
3. Mac 版を動かす：M1 → M2 → M3。北米版を macOS で動かし、これを新しい基準にする。
4. 日本版の実装：Phase 3A〜3D。
5. 統合試験：Phase 4（macOS と Windows VM の両方）。
6. 監査：Fable 5.2（別の worktree で実施）。

## 役割
- Opus 5.5：調査、設計、実装、試験、監査指摘の再検証、修正。
- Fable 5.2：独立監査のみ。このファイルの方針を前提にしない。
- ユーザー：各 Phase の承認者。コミットはユーザーが行う。

## 絶対規則
1. 指示された Phase だけを実施し、終了条件を満たしたら停止する。次の Phase へは承認なしに進まない。
2. git commit / tag / push / reset --hard / clean を実行しない。
3. roms/ と local_analysis/、および BIN/CUE/ISO/BIOS・音声・画像・文章データを Git に追加しない。
   ROM 由来の文章・フォント・画像をソースコードに埋め込まない。roms/ は読み取り専用として扱う。
4. sector / offset / load address / RAM address / hook target を推測で決めない。
   北米版の値を日本版に流用しない。未確認の値は UNKNOWN として記録し、実装を止める。
5. 確認済みの事実（CONFIRMED）、推測（ASSUMPTION）、不明（UNKNOWN）を必ず区別して書く。
6. テストを通すために期待値を緩めたり、skip を追加したり、例外を握りつぶしたりしない。
7. 変更箇所以外を一括でフォーマットしない。ついでのリファクタリングもしない。
8. ゲーム版の差（USA / Japan）は profile に、OS の差（Windows / macOS）は host 層に置く。
   両者を同じ条件分岐に混ぜない。`if (japanese)` を各所に散在させない。
9. ゲームコアは Objective-C++ にしない。`.mm` は macOS host 層の必要な箇所に限る。

## ディレクトリ
- roms/usa, roms/jp, roms/bios：ユーザー所有のディスク（読み取り専用、Git 管理外）
- local_analysis/：個人的な解析結果やハッシュ値（Git 管理外）
- review_jp/：日本版対応の文書（Phase 0〜4）
- review_portability/：移植・共通化の文書（M0 以降）
- review_fable/、review_fable_final/：Fable の監査報告（Opus は上書きしない）
- review_opus/：Fable の指摘の再検証と修正の記録

## ビルドと試験
<!-- Phase 0 / M0 の調査結果で確定させる。それまでは README の手順に従う -->
- macOS ビルド：（M2 / M3 後に記入）
- Windows ビルド（VM）：（Phase 0 後に記入）
- 試験：（Phase 0 後に記入）
- 基準コミット：local_analysis/BASELINE_COMMIT.txt

## 報告
- 文書と報告は日本語で書く。コード中のコメントと識別子は英語で書く。
- 各 Phase の終了時に、変更ファイル一覧、git diff --stat、実行したコマンド、試験結果、
  未実施の項目（特に Windows VM での確認待ち）、未解決事項を報告する。

## セッション再開時
最初に git status、git log --oneline -15、review_* 配下を確認し、
現在の Phase と次の作業だけを報告する。確認が終わるまでファイルを変更しない。


---

> 以下は feature/japanese-disc-support ブランチの作業規則。上記の上流の記述と矛盾する場合は、以下を優先する。

