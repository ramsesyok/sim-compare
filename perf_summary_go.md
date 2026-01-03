# aos_go_p chunk/grid summary (small/middle/large)

## Fastest combinations

### By simulation elapsed
- small: t8 c512 (4.704943s)
- middle: t2 c256 (17.190698s)
- large: t4 c512 (97.151816s)

### By wall time
- small: N/A (wall time not measured)
- middle: N/A (wall time not measured)
- large: N/A (wall time not measured)

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

## Tables (wall time, s)

### small
```
N/A (wall time not measured)
```

### middle
```
N/A (wall time not measured)
```

### large
```
N/A (wall time not measured)
```

# soa_go_p chunk/grid summary (small/middle/large)

## Fastest combinations

### By simulation elapsed
- small: t8 c512 (4.636836s)
- middle: t2 c128 (16.775818s)
- large: t2 c512 (89.452314s)

### By wall time
- small: N/A (wall time not measured)
- middle: N/A (wall time not measured)
- large: N/A (wall time not measured)

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

## Tables (wall time, s)

### small
```
N/A (wall time not measured)
```

### middle
```
N/A (wall time not measured)
```

### large
```
N/A (wall time not measured)
```

# ark_go_p chunk/grid summary (small/middle/large)

## Fastest combinations

### By simulation elapsed
- small: t2 c128 (5.277533s)
- middle: t8 c128 (16.948616s)
- large: t2 c128 (90.340079s)

### By wall time
- small: N/A (wall time not measured)
- middle: N/A (wall time not measured)
- large: N/A (wall time not measured)

## Tables (simulation elapsed, s)

### small
```
threads \ chunk    c128        c256        c512
t2               5.277533    5.350201    5.452276
t4               5.366950    5.315946    5.303296
t8               5.359615    5.333825    5.399478
```

### middle
```
threads \ chunk    c128        c256        c512
t2              18.573085   18.890863   18.406580
t4              18.904000   18.603305   17.741434
t8              16.948616   17.217464   17.168454
```

### large
```
threads \ chunk    c128        c256        c512
t2              90.340079   94.309112   96.711629
t4              97.717125   95.978179   94.929832
t8              98.421881   99.907542   98.798515
```

## Tables (wall time, s)

### small
```
N/A (wall time not measured)
```

### middle
```
N/A (wall time not measured)
```

### large
```
N/A (wall time not measured)
```

# donburi_go_p chunk/grid summary (small/middle/large)

## Fastest combinations

### By simulation elapsed
- small: t4 c256 (5.123185s)
- middle: t2 c128 (18.673906s)
- large: t2 c128 (96.780278s)

### By wall time
- small: N/A (wall time not measured)
- middle: N/A (wall time not measured)
- large: N/A (wall time not measured)

## Tables (simulation elapsed, s)

### small
```
threads \ chunk    c128        c256        c512
t2               5.289403    5.269899    5.230271
t4               5.141562    5.123185    5.154047
t8               5.177190    5.161694    5.218627
```

### middle
```
threads \ chunk    c128        c256        c512
t2              18.673906   18.871983   18.887487
t4              18.748826   18.951915   18.751302
t8              18.853966   19.541131   18.853951
```

### large
```
threads \ chunk    c128        c256        c512
t2              96.780278   98.683023  100.879250
t4              98.635174   98.832140  100.962501
t8              98.879376   99.882168  100.865095
```

## Tables (wall time, s)

### small
```
N/A (wall time not measured)
```

### middle
```
N/A (wall time not measured)
```

### large
```
N/A (wall time not measured)
```

# Serial vs Parallel (simulation elapsed)

Parallelは `*_go_p` の最速値、Serialは `*_go` の最速値（aos_goのみ今回の再計測）です。

```
name        scenario  serial_s   parallel_s  speedup
aos_go      small     3.522976    4.704943     0.75x
aos_go      middle   15.255151   17.190698     0.89x
aos_go      large    81.912350   97.151816     0.84x
soa_go      small     3.727755    4.636836     0.80x
soa_go      middle   15.592521   16.775818     0.93x
soa_go      large    86.997164   89.452314     0.97x
donburi_go  small     4.363026    5.123185     0.85x
donburi_go  middle   19.192974   18.673906     1.03x
donburi_go  large   102.839537   96.780278     1.06x
ark_go      small     3.755306    5.277533     0.71x
ark_go      middle   15.983206   16.948616     0.94x
ark_go      large    88.142054   90.340079     0.98x
```
