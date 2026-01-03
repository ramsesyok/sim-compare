# aos_rs_p chunk/grid summary (small/middle/large)

## Fastest combinations

### By simulation elapsed
- small: t2 c128 (1.54784s)
- middle: t2 c256 (11.28604s)
- large: t8 c512 (76.67559s)

### By wall time
- small: t2 c128 (1.64s) ※t4 c128 / t4 c256 と同値
- middle: t8 c256 (11.52s)
- large: t8 c512 (74.51s)

## Tables (simulation elapsed, s)

### small
```
threads \ chunk    c128        c256        c512
t2               1.54784    1.56444    1.57128
t4               1.55461    1.54939    1.57420
t8               1.57686    1.59628    1.58140
```

### middle
```
threads \ chunk    c128        c256        c512
t2              16.16294   11.28604   11.39566
t4              18.66726   11.42384   11.87763
t8              22.30627   11.32638   11.42361
```

### large
```
threads \ chunk    c128        c256        c512
t2              85.78973   86.45361   78.78336
t4              86.40169   78.09597   80.00109
t8              95.99258   92.72383   76.67559
```

## Tables (wall time, s)

### small
```
threads \ chunk    c128        c256        c512
t2                 1.64          1.65          1.66
t4                 1.64          1.64          1.66
t8                 1.67          1.68          1.66
```

### middle
```
threads \ chunk    c128        c256        c512
t2                17.48         12.80         13.92
t4                18.86         11.61         12.09
t8                23.64         11.52         16.20
```

### large
```
threads \ chunk    c128        c256        c512
t2                86.22         86.92         79.23
t4                86.83         78.56         77.65
t8                92.96         90.77         74.51
```

# soa_rs_p chunk/grid summary (small/middle/large)

## Fastest combinations

### By simulation elapsed
- small: t8 c512 (1.65257s)
- middle: t8 c256 (12.00170s)
- large: t2 c128 (75.47888s)

### By wall time
- small: t8 c512 (1.66s)
- middle: t8 c256 (10.58s)
- large: t2 c128 (72.85s)

## Tables (simulation elapsed, s)

### small
```
threads \ chunk    c128        c256        c512
t2              1.70277       1.74423       1.79169
t4              1.84940       1.82943       1.88454
t8              1.84370       1.82635       1.65257
```

### middle
```
threads \ chunk    c128        c256        c512
t2             18.37066      12.41610      13.26032
t4             20.86925      12.08298      17.55867
t8             23.07382      12.00170      14.43335
```

### large
```
threads \ chunk    c128        c256        c512
t2             75.47888      86.93175      77.07637
t4             80.11752      84.53753      88.63156
t8             94.67739      90.78439      83.57254
```

## Tables (wall time, s)

### small
```
threads \ chunk    c128        c256        c512
t2                 1.71          1.75          1.80
t4                 1.85          1.83          1.89
t8                 1.85          1.83          1.66
```

### middle
```
threads \ chunk    c128        c256        c512
t2                18.43         12.47         11.88
t4                19.47         12.14         17.62
t8                21.67         10.58         14.49
```

### large
```
threads \ chunk    c128        c256        c512
t2                72.85         83.53         74.48
t4                77.53         80.33         84.58
t8                90.38         86.68         80.82
```

# hecs_rs_p chunk/grid summary (small/middle/large)

## Fastest combinations

### By simulation elapsed
- small: t2 c256 (2.58074s)
- middle: t8 c256 (15.47505s)
- large: t2 c512 (88.70329s)

### By wall time
- small: t2 c256 (2.59s)
- middle: t8 c256 (13.91s)
- large: t2 c128 (85.21s)

## Tables (simulation elapsed, s)

### small
```
threads \ chunk    c128        c256        c512
t2              2.59402       2.58074       2.63368
t4              2.64713       2.63426       2.61714
t8              2.64178       2.74585       2.76185
```

### middle
```
threads \ chunk    c128        c256        c512
t2             22.59643      16.35518      16.53909
t4             24.90737      15.59962      15.55812
t8             27.73325      15.47505      15.56769
```

### large
```
threads \ chunk    c128        c256        c512
t2             89.51954      90.25044      88.70329
t4             93.82506      93.00607      90.54073
t8            111.63240     102.97902      93.11873
```

## Tables (wall time, s)

### small
```
threads \ chunk    c128        c256        c512
t2                 2.60          2.59          2.64
t4                 2.65          2.64          2.62
t8                 2.65          2.75          2.77
```

### middle
```
threads \ chunk    c128        c256        c512
t2                21.11         16.41         15.04
t4                24.97         14.09         15.62
t8                26.22         13.91         15.62
```

### large
```
threads \ chunk    c128        c256        c512
t2                85.21         86.10         85.96
t4                89.38         88.66         86.11
t8               107.29         96.99         88.77
```

# bevy_rs_p chunk/grid summary (small/middle/large)

## Fastest combinations

### By simulation elapsed
- small: t2 c512 (2.21229s)
- middle: t8 c512 (13.82029s)
- large: t2 c256 (80.33085s)

### By wall time
- small: t2 c512 (2.22s)
- middle: t8 c256 (12.58s)
- large: t2 c256 (77.82s)

## Tables (simulation elapsed, s)

### small
```
threads \ chunk    c128        c256        c512
t2              2.49882       2.53316       2.21229
t4              2.61781       3.43406       3.73164
t8              3.30741       3.45010       2.92104
```

### middle
```
threads \ chunk    c128        c256        c512
t2             23.67551      14.87578      14.48185
t4             23.39566      14.12666      14.22311
t8             25.88860      14.18367      13.82029
```

### large
```
threads \ chunk    c128        c256        c512
t2             85.27728      80.33085      87.45210
t4             83.23209      83.54123      82.68768
t8            101.36767      93.49407      83.93206
```

## Tables (wall time, s)

### small
```
threads \ chunk    c128        c256        c512
t2                 2.51          2.54          2.22
t4                 2.63          3.44          3.74
t8                 3.31          3.46          2.93
```

### middle
```
threads \ chunk    c128        c256        c512
t2                22.22         14.93         12.94
t4                21.86         14.18         14.28
t8                24.31         12.58         13.87
```

### large
```
threads \ chunk    c128        c256        c512
t2                80.95         77.82         83.00
t4                78.71         80.74         78.22
t8                96.93         89.02         79.45
```
