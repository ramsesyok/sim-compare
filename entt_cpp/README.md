# entt_cpp

## 概要
EnTTを用いたECS(Entity Component System)構成でシミュレーションを実装するC++版です。CLIでシナリオやログ出力先を指定し、ECSの更新ループを動かします。

## 目的と設計のねらい
- エンティティとコンポーネントを分離し、役割別の処理を整理する
- JSONスキーマに整合する入力とログ出力の流れを、最小構成で明示する
- 他言語実装とCLI互換を保ちながら、ECSの比較検証を進めやすくする

## コード構成
- `src/main.cpp`
  - CLI引数を受け取り、ECSのシミュレーション処理へつなぐ入口です。
  - ECSの考え方を初心者向けに説明するコメントを冒頭に置いています。
- `src/ent_simulation.cpp`
  - EnTTのレジストリを使い、役割ごとに処理を分けた更新ループをまとめています。
  - EnTTはSystem専用の型を用意しないため、更新処理は関数やクラスとして実装しています。

## ECSの最小イメージ
ECSでは「エンティティ = ID」「コンポーネント = 属性」「システム = 処理」という分離で考えます。

```
Entity[1] = {Position, Role, Route}
Entity[2] = {Position, Role, DetectionState}
```

### 短い補足
- 位置更新は`Position`を持つエンティティだけを対象にします。
- 探知は`Role=SCOUT`と`DetectionState`を持つエンティティに限定します。
  - EnTTが提供するのはレジストリとview/groupによるクエリまでで、Systemはアプリ側で設計します。

## ビルド・実行
```
cmake -S . -B build
cmake --build build
./build/entt_cpp_sim --scenario <path> --timeline-log <path> --event-log <path>
```

CLI11の標準ヘルプは次で確認できます。
```
./build/entt_cpp_sim --help
```

## 入出力の注意
シナリオやログのパスは外部入力です。`schemas/`にあるJSONスキーマと整合する前提で利用し、受け取ったパスの取り扱いには注意してください。
