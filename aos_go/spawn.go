package main

import "math"

type RoutePoint struct {
	LatDeg    float64
	LonDeg    float64
	AltM      float64
	SpeedsKph float64
	Ecef      Ecef
}

type ObjectState struct {
	ID               string
	TeamID           string
	Role             Role
	StartSec         int
	Route            []RoutePoint
	SegmentEndSecs   []float64
	TotalDurationSec float64
	Position         Ecef
	DetectState      map[string]DetectionInfo
	HasDetonated     bool
}

func buildObjects(scenario *Scenario) []ObjectState {
	objects := make([]ObjectState, 0)

	// チームとオブジェクトを走査し、AoSの状態配列を組み立てます。
	for _, team := range scenario.Teams {
		for _, obj := range team.Objects {
			// 経路の緯度経度をECEF座標に変換して保持します。
			route := buildRoute(obj.Route)
			// 区間ごとの移動時間を計算し、後で位置補間に使います。
			segmentEnds, totalDuration := buildSegmentTimes(route)
			position := Ecef{}
			if len(route) > 0 {
				position = route[0].Ecef
			}

			objects = append(objects, ObjectState{
				ID:               obj.ID,
				TeamID:           team.ID,
				Role:             obj.Role,
				StartSec:         obj.StartSec,
				Route:            route,
				SegmentEndSecs:   segmentEnds,
				TotalDurationSec: totalDuration,
				Position:         position,
				DetectState:      make(map[string]DetectionInfo),
				HasDetonated:     false,
			})
		}
	}

	return objects
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
