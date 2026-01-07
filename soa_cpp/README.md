# soa_cpp

## 概要
SoA(Structure of Arrays)でシミュレーションを実装するC++版のひな形です。現時点ではCLI引数を受け取り、将来の実装に向けた入口のみを用意しています。

## 目的と設計のねらい
- 役割や属性ごとに配列を分離し、キャッシュ効率を意識した実装へ発展させる
- JSONスキーマに整合する入力とログ出力の流れを、最小構成で明示する
- 他言語実装とCLI互換を保ちながら、SoAの比較検証を進めやすくする

## コード構成
- `src/main.cpp`
  - CLI引数を受け取り、SoAのシミュレーション処理へつなぐ入口です。
  - SoAの考え方を初心者向けに説明するコメントを冒頭に置いています。
- `src/soa_simulation.cpp`
  - SoAの配列をまとめて走査し、キャッシュ効率と分岐予測の安定性を意識した更新処理をまとめています。

## SoAとAoSの簡易比較
SoA(Structure of Arrays)とAoS(Array of Structures)の違いを、最小限の図でまとめます。

### AoSのイメージ
```
Object[0] = {id, team, role, x, y, z}
Object[1] = {id, team, role, x, y, z}
Object[2] = {id, team, role, x, y, z}
```

### SoAのイメージ
```
ids   = [id0, id1, id2]
teams = [t0,  t1,  t2 ]
roles = [r0,  r1,  r2 ]
xs    = [x0,  x1,  x2 ]
ys    = [y0,  y1,  y2 ]
zs    = [z0,  z1,  z2 ]
```

### 短い補足
- 位置更新だけを行う場合、SoAは`xs/ys/zs`だけを連続して触れるため、不要な属性を読み込みにくくなります。
- AoSは「1個体の情報がまとまっている」ため、個体単位の処理が書きやすいという利点があります。

## ビルド・実行
```
cmake -S . -B build
cmake --build build
./build/soa_cpp_sim --scenario <path> --timeline-log <path> --event-log <path>
```

CLI11の標準ヘルプは次で確認できます。
```
./build/soa_cpp_sim --help
```

## 入出力の注意
シナリオやログのパスは外部入力です。`schemas/`にあるJSONスキーマと整合する前提で利用し、受け取ったパスの取り扱いには注意してください。
