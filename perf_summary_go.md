# aos_go_p chunk/grid summary (small/middle/large)

## Fastest combinations (simulation elapsed)
- small: t8 c512 (4.704943s)
- middle: t2 c256 (17.190698s)
- large: t4 c512 (97.151816s)

## Tables (simulation elapsed, s)

### small
```
threads \ chunk    c128        c256        c512
t2               4.815430    4.725154    4.749072
t4               4.710377    5.156232    4.725965
t8               4.831258    4.823600    4.704943
```

### middle
```
threads \ chunk    c128        c256        c512
t2              18.372806   17.190698   17.199223
t4              20.073609   17.238672   17.346894
t8              17.890811   18.044524   20.105665
```

### large
```
threads \ chunk    c128        c256        c512
t2             105.851900  102.119460  101.159088
t4              97.301292  103.302937   97.151816
t8              99.068787  108.548409   99.052316
```

# soa_go chunk/grid summary (small/middle/large)

## Fastest combinations (simulation elapsed)
- small: t2 c512 (3.727755s)
- middle: t4 c128 (15.592521s)
- large: t8 c512 (86.997164s)

## Tables (simulation elapsed, s)

### small
```
threads \ chunk    c128        c256        c512
t2               3.799456    3.802229    3.727755
t4               3.754573    3.741675    3.763199
t8               3.798941    3.781614    3.794669
```

### middle
```
threads \ chunk    c128        c256        c512
t2              17.254772   17.048625   16.451145
t4              15.592521   15.792409   15.726858
t8              15.881334   15.608241   15.843131
```

### large
```
threads \ chunk    c128        c256        c512
t2              89.838451   90.111049   93.607213
t4              91.419864   93.737705   94.651445
t8              92.782936   88.895059   86.997164
```

# donburi_go chunk/grid summary (small/middle/large)

## Fastest combinations (simulation elapsed)
- small: t2 c512 (4.363026s)
- middle: t4 c256 (19.192974s)
- large: t8 c256 (102.839537s)

## Tables (simulation elapsed, s)

### small
```
threads \ chunk    c128        c256        c512
t2               4.572332    4.388580    4.363026
t4               4.401286    4.415828    4.469275
t8               4.573273    4.503647    4.442114
```

### middle
```
threads \ chunk    c128        c256        c512
t2              19.233988   20.223213   19.610042
t4              19.612253   19.192974   19.755624
t8              19.234902   19.306066   19.210723
```

### large
```
threads \ chunk    c128        c256        c512
t2             103.827746  104.821962  103.946872
t4             106.555006  107.207234  106.634238
t8             105.591100  102.839537  103.168810
```

# ark_go chunk/grid summary (small/middle/large)

## Fastest combinations (simulation elapsed)
- small: t8 c256 (3.755306s)
- middle: t4 c512 (15.983206s)
- large: t8 c512 (88.142054s)

## Tables (simulation elapsed, s)

### small
```
threads \ chunk    c128        c256        c512
t2               4.114457    3.936989    3.943124
t4               3.780266    3.759906    4.020619
t8               3.824241    3.755306    3.788027
```

### middle
```
threads \ chunk    c128        c256        c512
t2              16.190034   16.101260   16.135491
t4              16.044156   16.243447   15.983206
t8              16.127448   16.068842   16.595680
```

### large
```
threads \ chunk    c128        c256        c512
t2              88.506099   89.505491   88.397919
t4              88.351286   88.873070   89.069206
t8              88.258355   88.346561   88.142054
```

# soa_go_p chunk/grid summary (small/middle/large)

## Fastest combinations (simulation elapsed)
- small: t8 c512 (4.636836s)
- middle: t2 c128 (16.775818s)
- large: t2 c512 (89.452314s)

## Tables (simulation elapsed, s)

### small
```
threads \ chunk    c128        c256        c512
t2               4.742808    4.794460    4.724792
t4               4.704786    4.725519    4.697656
t8               4.749109    4.667307    4.636836
```

### middle
```
threads \ chunk    c128        c256        c512
t2              16.775818   17.059173   16.925617
t4              16.832558   16.953087   16.993178
t8              16.830909   16.987412   17.001760
```

### large
```
threads \ chunk    c128        c256        c512
t2              91.114964   92.242824   89.452314
t4              95.109879   95.158639   91.309132
t8              94.796889   94.534274   91.114205
```
