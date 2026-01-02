package main

import "math"

type CellKey struct {
	X int
	Y int
	Z int
}

func buildSpatialHash(positions []Ecef, cellSize float64, reuse map[CellKey][]int) map[CellKey][]int {
	result := reuse
	if result == nil {
		result = make(map[CellKey][]int)
	} else {
		for key, indices := range result {
			result[key] = indices[:0]
		}
	}
	if cellSize <= 0 {
		return result
	}

	// 各オブジェクトをセルに割り当て、同じセルの候補を高速に取得できるようにします。
	for i, pos := range positions {
		key := cellKey(pos, cellSize)
		result[key] = append(result[key], i)
	}

	return result
}

func cellKey(pos Ecef, cellSize float64) CellKey {
	return CellKey{
		X: int(math.Floor(pos.X / cellSize)),
		Y: int(math.Floor(pos.Y / cellSize)),
		Z: int(math.Floor(pos.Z / cellSize)),
	}
}
