package main

import (
	"math"

	"github.com/yohamta/donburi"
)

type RoutePoint struct {
	LatDeg    float64
	LonDeg    float64
	AltM      float64
	SpeedsKph float64
	Ecef      Ecef
}

func newWorld() donburiWorld {
	return donburi.NewWorld()
}

func registerResources(world donburiWorld, scenario *Scenario) {
	entity := world.Create(
		SimTimeComponent,
		DetectRangeComponent,
		BomRangeComponent,
		SnapshotCacheComponent,
		EventBufferComponent,
		TimelineBufferComponent,
	)
	entry := world.Entry(entity)
	SimTimeComponent.SetValue(entry, SimTime{TimeSec: 0})
	DetectRangeComponent.SetValue(entry, DetectRange{Value: float64(scenario.Performance.Scout.DetectRangeM)})
	BomRangeComponent.SetValue(entry, BomRange{Value: scenario.Performance.Attacker.BomRangeM})
	SnapshotCacheComponent.SetValue(entry, SnapshotCache{
		Snapshots:   nil,
		Positions:   nil,
		SpatialHash: make(map[CellKey][]int),
	})
	EventBufferComponent.SetValue(entry, EventBuffer{})
	TimelineBufferComponent.SetValue(entry, TimelineBuffer{})
}

func spawnEntities(world donburiWorld, scenario *Scenario) {
	// ECSでは「要素ごとにコンポーネントを分ける」設計です。
	// ここでは各オブジェクトをエンティティとして生成します。
	for _, team := range scenario.Teams {
		for _, obj := range team.Objects {
			route := buildRoute(obj.Route)
			segmentEnds, totalDuration := buildSegmentTimes(route)
			position := Ecef{}
			if len(route) > 0 {
				position = route[0].Ecef
			}

			entity := world.Create(
				IdComponent,
				TeamIdComponent,
				RoleComponent,
				StartSecComponent,
				RoutePointsComponent,
				SegmentEndSecsComponent,
				TotalDurationComponent,
				PositionComponent,
				DetectStateComponent,
				HasDetonatedComponent,
			)
			entry := world.Entry(entity)
			IdComponent.SetValue(entry, obj.ID)
			TeamIdComponent.SetValue(entry, team.ID)
			RoleComponent.SetValue(entry, obj.Role)
			StartSecComponent.SetValue(entry, obj.StartSec)
			RoutePointsComponent.SetValue(entry, route)
			SegmentEndSecsComponent.SetValue(entry, segmentEnds)
			TotalDurationComponent.SetValue(entry, totalDuration)
			PositionComponent.SetValue(entry, position)
			DetectStateComponent.SetValue(entry, DetectState{})
			HasDetonatedComponent.SetValue(entry, false)
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

type donburiWorld = donburi.World
