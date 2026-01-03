# aos_go_p chunk/grid summary (small/middle/large)

## Fastest combinations

### By simulation elapsed
- small: t8 c512 (4.505649s)
- middle: t8 c128 (16.139700s)
- large: t2 c128 (86.223287s)

### By wall time
- small: t8 c512 (4.58s)
- middle: t8 c128 (16.22s)
- large: t2 c128 (86.31s)

## Tables (simulation elapsed, s)

### small
```
threads \ chunk    c128        c256        c512
t2               4.826286    4.756157    4.765642
t4               4.662612    4.584432    4.878068
t8               4.654104    4.564934    4.505649
```

### middle
```
threads \ chunk    c128        c256        c512
t2              16.249638   16.268907   16.213202
t4              16.275890   16.288002   16.259036
t8              16.139700   16.224216   16.291759
```

### large
```
threads \ chunk    c128        c256        c512
t2              86.223287   88.501079   88.333708
t4              90.443359   89.903209   89.645977
t8              90.557557   90.185898   88.820411
```

## Tables (wall time, s)

### small
```
threads \ chunk    c128        c256        c512
t2                   5.04        4.84        4.86
t4                   4.74        4.67        4.96
t8                   4.75        4.65        4.58
```

### middle
```
threads \ chunk    c128        c256        c512
t2                  16.33       16.35       16.29
t4                  16.35       16.37       16.33
t8                  16.22       16.31       16.37
```

### large
```
threads \ chunk    c128        c256        c512
t2                  86.31       88.60       88.43
t4                  90.54       90.01       89.76
t8                  90.69       90.29       88.91
```

# soa_go_p chunk/grid summary (small/middle/large)

## Fastest combinations

### By simulation elapsed
- small: t4 c512 (4.485167s)
- middle: t4 c128 (15.822070s)
- large: t2 c128 (84.463561s)

### By wall time
- small: t4 c512 (4.56s)
- middle: t4 c128 (15.91s)
- large: t2 c128 (84.56s)

## Tables (simulation elapsed, s)

### small
```
threads \ chunk    c128        c256        c512
t2               4.651182    4.775716    4.757735
t4               4.742579    4.539365    4.485167
t8               4.759348    4.724836    4.500969
```

### middle
```
threads \ chunk    c128        c256        c512
t2              16.003341   15.944192   15.977173
t4              15.822070   15.987782   15.959153
t8              15.866268   15.894121   16.148310
```

### large
```
threads \ chunk    c128        c256        c512
t2              84.463561   86.958742   85.280207
t4              88.071108   87.264880   86.332243
t8              87.856682   87.246580   86.055475
```

## Tables (wall time, s)

### small
```
threads \ chunk    c128        c256        c512
t2                   4.86        4.85        4.84
t4                   4.82        4.62        4.56
t8                   4.84        4.82        4.57
```

### middle
```
threads \ chunk    c128        c256        c512
t2                  16.08       16.02       16.06
t4                  15.91       16.07       16.04
t8                  15.94       15.98       16.23
```

### large
```
threads \ chunk    c128        c256        c512
t2                  84.56       87.08       85.38
t4                  88.16       87.36       86.43
t8                  87.95       87.34       86.15
```

# ark_go_p chunk/grid summary (small/middle/large)

## Fastest combinations

### By simulation elapsed
- small: t8 c128 (4.575689s)
- middle: t4 c128 (16.216457s)
- large: t2 c128 (87.242914s)

### By wall time
- small: t8 c128 (4.65s)
- middle: t4 c128 (16.30s)
- large: t2 c128 (87.34s)

## Tables (simulation elapsed, s)

### small
```
threads \ chunk    c128        c256        c512
t2               4.865901    4.870219    4.869138
t4               4.736615    4.639171    4.635376
t8               4.575689    4.971710    4.603435
```

### middle
```
threads \ chunk    c128        c256        c512
t2              16.540193   16.382518   16.298282
t4              16.216457   16.434612   16.371378
t8              16.269510   16.351950   16.340202
```

### large
```
threads \ chunk    c128        c256        c512
t2              87.242914   89.240133   89.083352
t4              91.559434   91.297282   89.389663
t8              90.807411   90.364885   89.825960
```

## Tables (wall time, s)

### small
```
threads \ chunk    c128        c256        c512
t2                   5.26        4.95        4.95
t4                   4.82        4.72        4.71
t8                   4.65        5.05        4.68
```

### middle
```
threads \ chunk    c128        c256        c512
t2                  16.63       16.47       16.37
t4                  16.30       16.52       16.45
t8                  16.35       16.43       16.42
```

### large
```
threads \ chunk    c128        c256        c512
t2                  87.34       89.34       89.19
t4                  91.65       91.39       89.51
t8                  90.90       90.45       89.92
```

# donburi_go_p chunk/grid summary (small/middle/large)

## Fastest combinations

### By simulation elapsed
- small: t8 c512 (5.041604s)
- middle: t2 c512 (18.245327s)
- large: t2 c128 (94.583817s)

### By wall time
- small: t8 c512 (5.12s)
- middle: t2 c512 (18.33s)
- large: t2 c128 (94.68s)

## Tables (simulation elapsed, s)

### small
```
threads \ chunk    c128        c256        c512
t2               5.345565    5.316019    5.378027
t4               5.184654    5.082156    5.241496
t8               5.293421    5.134941    5.041604
```

### middle
```
threads \ chunk    c128        c256        c512
t2              18.430840   18.965995   18.245327
t4              18.276722   18.324363   18.294636
t8              18.262759   18.897982   18.345695
```

### large
```
threads \ chunk    c128        c256        c512
t2              94.583817   96.512759   98.651639
t4              98.233922   98.265760   99.753736
t8              98.290569   98.373376   99.918097
```

## Tables (wall time, s)

### small
```
threads \ chunk    c128        c256        c512
t2                   5.73        5.40        5.46
t4                   5.26        5.16        5.31
t8                   5.38        5.21        5.12
```

### middle
```
threads \ chunk    c128        c256        c512
t2                  18.51       19.05       18.33
t4                  18.36       18.40       18.37
t8                  18.34       18.98       18.42
```

### large
```
threads \ chunk    c128        c256        c512
t2                  94.68       96.63       98.77
t4                  98.33       98.36       99.86
t8                  98.39       98.47      100.01
```

# Serial vs Parallel (simulation elapsed)

Parallelは `*_go_p` の最速値、Serialは `*_go` の再計測値です。

```
name        scenario  serial_s   parallel_s  speedup
aos_go      small     3.546891   4.505649    0.79x
aos_go      middle   15.124489  16.139700    0.94x
aos_go      large    81.806781  86.223287    0.95x
soa_go      small     3.602506   4.485167    0.80x
soa_go      middle   15.746071  15.822070    1.00x
soa_go      large    80.510072  84.463561    0.95x
donburi_go  small     4.306608   5.041604    0.85x
donburi_go  middle   19.012519  18.245327    1.04x
donburi_go  large    97.712258  94.583817    1.03x
ark_go      small     3.789329   4.575689    0.83x
ark_go      middle   16.350298  16.216457    1.01x
ark_go      large    84.901124  87.242914    0.97x
```

# Serial vs Parallel (wall time)

Parallelは `*_go_p` の最速wall time、Serialは `*_go` の再計測wall timeです。

```
name        scenario  serial_s   parallel_s  speedup
aos_go      small         7.74       4.58    1.69x
aos_go      middle       15.21      16.22    0.94x
aos_go      large        81.92      86.31    0.95x
soa_go      small         3.82       4.56    0.84x
soa_go      middle       15.82      15.91    0.99x
soa_go      large        80.60      84.56    0.95x
donburi_go  small         4.84       5.12    0.95x
donburi_go  middle       19.10      18.33    1.04x
donburi_go  large        97.80      94.68    1.03x
ark_go      small         4.45       4.65    0.96x
ark_go      middle       16.44      16.30    1.01x
ark_go      large        85.02      87.34    0.97x
```