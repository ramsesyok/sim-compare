package main

import "math"

type RoutePoint struct {
	LatDeg    float64
	LonDeg    float64
	AltM      float64
	SpeedsKph float64
	Ecef      Ecef
}

type SoaState struct {
	IDs            []string
	TeamIDs        []string
	Roles          []Role
	StartSecs      []int
	Routes         [][]RoutePoint
	SegmentEndSecs [][]float64
	TotalDurations []float64
	Positions      []Ecef
	DetectState    []map[string]DetectionInfo
	HasDetonated   []bool
}

func buildState(scenario *Scenario) SoaState {
	var state SoaState

	// SoAは「同じ種類のデータを配列でまとめる」設計です。
	// ここでは、ID配列・役割配列・位置配列などを個別に保持します。
	for _, team := range scenario.Teams {
		for _, obj := range team.Objects {
			route := buildRoute(obj.Route)
			segmentEnds, totalDuration := buildSegmentTimes(route)
			position := Ecef{}
			if len(route) > 0 {
				position = route[0].Ecef
			}

			state.IDs = append(state.IDs, obj.ID)
			state.TeamIDs = append(state.TeamIDs, team.ID)
			state.Roles = append(state.Roles, obj.Role)
			state.StartSecs = append(state.StartSecs, obj.StartSec)
			state.Routes = append(state.Routes, route)
			state.SegmentEndSecs = append(state.SegmentEndSecs, segmentEnds)
			state.TotalDurations = append(state.TotalDurations, totalDuration)
			state.Positions = append(state.Positions, position)
			state.DetectState = append(state.DetectState, make(map[string]DetectionInfo))
			state.HasDetonated = append(state.HasDetonated, false)
		}
	}

	return state
}

func buildRoute(route []Waypoint) []RoutePoint {
	result := make([]RoutePoint, 0, len(route))
	for _, wp := range route {
		// WGS84の緯度経度高度をECEF座標へ変換します。
		ecef := geodeticToECEF(wp.LatDeg, wp.LonDeg, wp.AltM)
		result = append(result, RoutePoint{
			LatDeg:    wp.LatDeg,
			LonDeg:    wp.LonDeg,
			AltM:      wp.AltM,
			SpeedsKph: wp.SpeedsKph,
			Ecef:      ecef,
		})
	}
	return result
}

func buildSegmentTimes(route []RoutePoint) ([]float64, float64) {
	if len(route) < 2 {
		return nil, 0
	}

	segmentEnds := make([]float64, 0, len(route)-1)
	acc := 0.0

	for i := 0; i < len(route)-1; i++ {
		// 区間距離と速度から移動に必要な秒数を算出します。
		a := route[i].Ecef
		b := route[i+1].Ecef
		distance := distanceECEF(a, b)
		speedMps := (route[i].SpeedsKph * 1000.0) / 3600.0
		duration := math.Inf(1)
		if speedMps > 0 {
			duration = distance / speedMps
		}
		acc += duration
		segmentEnds = append(segmentEnds, acc)
	}

	return segmentEnds, acc
}
