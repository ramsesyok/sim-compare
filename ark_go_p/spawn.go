package main

import (
	"math"

	"github.com/mlange-42/ark/ecs"
)

type RoutePoint struct {
	LatDeg    float64
	LonDeg    float64
	AltM      float64
	SpeedsKph float64
	Ecef      Ecef
}

func registerResources(world *ecs.World, scenario *Scenario) {
	// Systemで参照するリソースを登録します。
	resTime := ecs.NewResource[SimTime](world)
	resTime.Add(&SimTime{TimeSec: 0})

	resDetect := ecs.NewResource[DetectRange](world)
	resDetect.Add(&DetectRange{Value: float64(scenario.Performance.Scout.DetectRangeM)})

	resBom := ecs.NewResource[BomRange](world)
	resBom.Add(&BomRange{Value: scenario.Performance.Attacker.BomRangeM})

	resSnapshot := ecs.NewResource[SnapshotCache](world)
	resSnapshot.Add(&SnapshotCache{
		Snapshots:   nil,
		Positions:   nil,
		SpatialHash: make(map[CellKey][]int),
	})

	resEvent := ecs.NewResource[EventBuffer](world)
	resEvent.Add(&EventBuffer{})

	resTimeline := ecs.NewResource[TimelineBuffer](world)
	resTimeline.Add(&TimelineBuffer{})
}

func spawnEntities(world *ecs.World, scenario *Scenario) {
	// ECSでは「要素ごとにコンポーネントを分ける」設計です。
	// ここでは各オブジェクトをエンティティとして生成します。
	mapper := ecs.NewMap10[
		Id,
		TeamId,
		RoleComp,
		StartSec,
		RoutePoints,
		SegmentEndSecs,
		TotalDuration,
		Position,
		DetectState,
		HasDetonated,
	](world)

	for _, team := range scenario.Teams {
		for _, obj := range team.Objects {
			route := buildRoute(obj.Route)
			segmentEnds, totalDuration := buildSegmentTimes(route)
			position := Ecef{}
			if len(route) > 0 {
				position = route[0].Ecef
			}

			mapper.NewEntity(
				&Id{Value: obj.ID},
				&TeamId{Value: team.ID},
				&RoleComp{Value: obj.Role},
				&StartSec{Value: obj.StartSec},
				&RoutePoints{Value: route},
				&SegmentEndSecs{Value: segmentEnds},
				&TotalDuration{Value: totalDuration},
				&Position{Value: position},
				&DetectState{Value: map[string]DetectionInfo{}},
				&HasDetonated{Value: false},
			)
		}
	}
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
