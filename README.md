# シミュレーション実装比較
言語・シミュレーションアーキテクチャ[AoS (Array of Structures) , SoA (Structure of Arrays) , ECS]、ライブラリの有無/種類などによるシミュレーションプログラムの実装比較を行う。
シミュレーションの対象は、 simulation_outline.mdに記載されている。
このシミュレーションは、本来のシミュレーションを簡易化したもので実装の比較のためのプロトタイプ仕様である。

# 比較対象
- Rust + AoS 
- Rust + SoA （ECSライブラリなし）
- Rust + [Ralith/hecs](https://github.com/Ralith/hecs)
- Rust + [Bevy](https://github.com/bevyengine/bevy)
- Go言語 + AoS
- Go言語 + SoA （ECSライブラリなし）
- Go言語 + [donburi](https://github.com/yottahmd/donburi-ecs)
    - Systemの実装は、Experimentalだが、他との比較のため利用する。
- Go言語 + [Ark](https://github.com/mlange-42/ark)+[Ark-Tools](https://github.com/mlange-42/ark-tools)
    - Ark-Toolsは、Systemの利用のみに使う。(schedulerは、要件と異なるため使わない)

# 実装時の注意点
- 並列化は行わない（ライブラリ側で自動で行われる場合を除く）
- すべての実装でのコマンドライン引数は、同一とする。
    - 引数リスト
        - `--scenario` シナリオJSONファイルのパスを指定する。
        - `--timeline-log` タイムラインndjsonの出力先を指定する。
        - `--event-log` イベントndjsonの出力先を指定する。
    - CLIライブラリ
        - Rustは、[clap](https://github.com/clap-rs/clap)を用いること。
        - Go言語は、[cobra](https://github.com/spf13/cobra)を用いること。
- コメントはすべて日本語で記載する。
- プログラム初心者でも分かるように、コメントは過剰に記載する。
- AoSとSoAおよびECSに詳しくない人に向け、メイン処理の冒頭には、各シミュレーションアーキテクチャの特徴を分かりやすく記載すること。
- 経路移動など共通の計算処理は、同じ計算ロジックを用いること（共通化は不要）

# 実装先フォルダ
- Rust + AoS: aos_rs
- Rust + SoA: soa_rs
- Rust + hecs: hecs_rs
- Rust + Bevy: bevy_rs
- Go言語 + AoS: aos_go
- Go言語 + SoA: soa_go
- Go言語 + donburi: donburi_go
- Go言語 + ark: ark_go
