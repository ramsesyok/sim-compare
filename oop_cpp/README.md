# oop_cpp

## 概要
OOP(オブジェクト指向)でシミュレーションを実装したC++版の実装です。`Simulation`が中心となり、シナリオ(JSON)の読み込み、オブジェクトの生成、ログの出力を行います。

## 目的と設計のねらい
- 役割(司令・偵察・伝令・攻撃)ごとの振る舞いをクラスで分離し、責務の違いを明確にする
- JSONスキーマに対応した入出力を、専用のJSONオブジェクト層(`jsonobj`名前空間)に集約する
- 1ステップずつの進行とログ生成を中心に、後から最適化や別アーキテクチャ比較がしやすい構成にする

## コード構成
cppソースは`src/`配下にまとめています。
- `src/main.cpp`
  - CLI引数を受け取り、`Simulation`に処理を委譲する入口です。
- `src/simulation.cpp` / `include/simulation.hpp`
  - シミュレーションの中核です。シナリオ読み込み、オブジェクト構築、時間進行、ログ生成を担当します。
- `src/sim_object.cpp` / `include/sim_object.hpp`
  - 全オブジェクト共通の基底クラスです。位置・速度・所属などの共通データと基本動作を定義します。
- `src/fixed_object.cpp` / `include/fixed_object.hpp`
  - 固定オブジェクトの基底クラスです。司令役の派生元になります。
- `src/movable_object.cpp` / `include/movable_object.hpp`
  - 移動オブジェクトの基底クラスです。偵察・伝令・攻撃の派生元になります。
- `src/commander_object.cpp` / `include/commander_object.hpp`
  - 司令役の振る舞いを定義します。
- `src/scout_object.cpp` / `include/scout_object.hpp`
  - 偵察役の検知処理とイベント生成を担当します。
- `src/messenger_object.cpp` / `include/messenger_object.hpp`
  - 伝令役の通信処理を担当します。
- `src/attacker_object.cpp` / `include/attacker_object.hpp`
  - 攻撃役の爆破処理とイベント生成を担当します。
- `src/logging.cpp` / `include/logging.hpp`
  - タイムラインログとイベントログをJSONとして書き出します。
- `src/route.cpp` / `include/route.hpp`
  - ルート上の移動計算を補助します。
- `src/geo.cpp` / `include/geo.hpp`
  - 座標や距離計算をまとめたユーティリティです。
- `src/spatial_hash.cpp` / `include/spatial_hash.hpp`
  - 近傍検索を効率化する空間ハッシュです。
- `include/jsonobj/`
  - シナリオやログのJSONオブジェクト定義です。`jsonobj`名前空間にまとめています。
- `tests/`
  - Catch2のアマルガメーションを使ったテストです。

## 主要なクラス関係(概要)
`SimObject`を基底に、固定物体(`FixedObject`)と移動物体(`MovableObject`)に分岐し、役割ごとのクラスが派生します。詳細は`class_design.md`のクラス図を参照してください。

## ビルド・実行
```
cmake -S . -B build
cmake --build build
./build/oop_cpp_sim --scenario <path> --timeline-log <path> --event-log <path>
```

## テスト
```
cmake --build build
./build/oop_cpp_tests
```

## 依存OSS(同梱)
- nlohmann/json
  - JSONのシリアライズ/デシリアライズに使用しています。
  - `3rdparty/include/nlohmann/json.hpp` に同梱しています。
- Catch2 (amalgamated)
  - 単体テストに使用しています。
  - `tests/catch_amalgamated.hpp` と `tests/catch_amalgamated.cpp` を同梱しています。

## 入出力の注意
シナリオやログのパスは外部入力です。`schemas/`にあるJSONスキーマと整合する前提で利用し、受け取ったパスの取り扱いには注意してください。
