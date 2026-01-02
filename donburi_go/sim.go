package main

import (
	"math"

	"github.com/yohamta/donburi"
	ecsLib "github.com/yohamta/donburi/ecs"
	"github.com/yohamta/donburi/filter"
)

type SimTime struct {
	TimeSec int
}

type DetectRange struct {
	Value float64
}

type BomRange struct {
	Value int
}

type SnapshotCache struct {
	Snapshots   []EntitySnapshot
	Positions   []Ecef
	SpatialHash map[CellKey][]int
}

type DetectionInfo struct {
	LatDeg    float64
	LonDeg    float64
	AltM      float64
	DistanceM int
}

type DetectState map[string]DetectionInfo

type EntitySnapshot struct {
	Entity   donburi.Entity
	ID       string
	TeamID   string
	Role     Role
	Position Ecef
}

var (
	IdComponent             = donburi.NewComponentType[string]()
	TeamIdComponent         = donburi.NewComponentType[string]()
	RoleComponent           = donburi.NewComponentType[Role]()
	StartSecComponent       = donburi.NewComponentType[int]()
	RoutePointsComponent    = donburi.NewComponentType[[]RoutePoint]()
	SegmentEndSecsComponent = donburi.NewComponentType[[]float64]()
	TotalDurationComponent  = donburi.NewComponentType[float64]()
	PositionComponent       = donburi.NewComponentType[Ecef]()
	DetectStateComponent    = donburi.NewComponentType[DetectState]()
	HasDetonatedComponent   = donburi.NewComponentType[bool]()
	SimTimeComponent        = donburi.NewComponentType[SimTime]()
	DetectRangeComponent    = donburi.NewComponentType[DetectRange]()
	BomRangeComponent       = donburi.NewComponentType[BomRange]()
	SnapshotCacheComponent  = donburi.NewComponentType[SnapshotCache]()
	EventBufferComponent    = donburi.NewComponentType[EventBuffer]()
	TimelineBufferComponent = donburi.NewComponentType[TimelineBuffer]()
)

func setSimTime(world donburiWorld, timeSec int) {
	entry := SimTimeComponent.MustFirst(world)
	SimTimeComponent.SetValue(entry, SimTime{TimeSec: timeSec})
}

func positionUpdateSystem(ecs *ecsLib.ECS) {
	// donburiのSystemは関数として定義し、クエリで対象を絞り込みます。
	query := donburi.NewQuery(filter.Contains(
		RoleComponent,
		StartSecComponent,
		RoutePointsComponent,
		SegmentEndSecsComponent,
		TotalDurationComponent,
		PositionComponent,
	))

	for entry := range query.Iter(ecs.World) {
		role := RoleComponent.GetValue(entry)
		startSec := StartSecComponent.GetValue(entry)
		route := RoutePointsComponent.GetValue(entry)
		segmentEnds := SegmentEndSecsComponent.GetValue(entry)
		totalDuration := TotalDurationComponent.GetValue(entry)
		pos := positionAtTime(role, startSec, route, segmentEnds, totalDuration, float64(getTimeSec(ecs.World)))
		PositionComponent.SetValue(entry, pos)
	}
}

func snapshotSystem(ecs *ecsLib.ECS) {
	detectRange := DetectRangeComponent.GetValue(DetectRangeComponent.MustFirst(ecs.World)).Value
	cacheEntry := SnapshotCacheComponent.MustFirst(ecs.World)
	cache := SnapshotCacheComponent.Get(cacheEntry)
	cache.Snapshots = cache.Snapshots[:0]
	cache.Positions = cache.Positions[:0]

	query := donburi.NewQuery(filter.Contains(IdComponent, TeamIdComponent, RoleComponent, PositionComponent))
	for entry := range query.Iter(ecs.World) {
		cache.Snapshots = append(cache.Snapshots, EntitySnapshot{
			Entity:   entry.Entity(),
			ID:       IdComponent.GetValue(entry),
			TeamID:   TeamIdComponent.GetValue(entry),
			Role:     RoleComponent.GetValue(entry),
			Position: PositionComponent.GetValue(entry),
		})
		cache.Positions = append(cache.Positions, PositionComponent.GetValue(entry))
	}

	// 探知処理のため、位置の配列から空間ハッシュを構築します。
	cache.SpatialHash = buildSpatialHash(cache.Positions, detectRange, cache.SpatialHash)
}

func detectionSystem(ecs *ecsLib.ECS) {
	detectRange := DetectRangeComponent.GetValue(DetectRangeComponent.MustFirst(ecs.World)).Value
	if detectRange <= 0 {
		return
	}

	cache := SnapshotCacheComponent.Get(SnapshotCacheComponent.MustFirst(ecs.World))
	eventBuffer := EventBufferComponent.Get(EventBufferComponent.MustFirst(ecs.World))
	currentTime := getTimeSec(ecs.World)

	for _, scout := range cache.Snapshots {
		if scout.Role != RoleScout {
			continue
		}

		entry := ecs.World.Entry(scout.Entity)
		detectState := DetectStateComponent.Get(entry)
		previousState := make(map[string]DetectionInfo, len(*detectState))
		for k, v := range *detectState {
			previousState[k] = v
		}

		currentDetected := make(map[string]DetectionInfo)
		baseKey := cellKey(scout.Position, detectRange)
		for dx := -1; dx <= 1; dx++ {
			for dy := -1; dy <= 1; dy++ {
				for dz := -1; dz <= 1; dz++ {
					key := CellKey{X: baseKey.X + dx, Y: baseKey.Y + dy, Z: baseKey.Z + dz}
					indices, ok := cache.SpatialHash[key]
					if !ok {
						continue
					}

					for _, otherIndex := range indices {
						other := cache.Snapshots[otherIndex]
						if other.Entity == scout.Entity {
							continue
						}
						if other.TeamID == scout.TeamID {
							continue
						}

						// 探知範囲内かどうかを最終的に距離で判定します。
						distance := distanceECEF(scout.Position, other.Position)
						if distance > detectRange {
							continue
						}

						lat, lon, alt := ecefToGeodetic(other.Position)
						currentDetected[other.ID] = DetectionInfo{
							LatDeg:    lat,
							LonDeg:    lon,
							AltM:      alt,
							DistanceM: int(math.Round(distance)),
						}
					}
				}
			}
		}

		for id, info := range currentDetected {
			if _, exists := previousState[id]; exists {
				continue
			}
			eventBuffer.DetectionEvents = append(eventBuffer.DetectionEvents, DetectionEvent{
				EventType:       "detection",
				DetectionAction: "found",
				TimeSec:         currentTime,
				ScountID:        scout.ID,
				LatDeg:          info.LatDeg,
				LonDeg:          info.LonDeg,
				AltM:            info.AltM,
				DistanceM:       info.DistanceM,
				DetectID:        id,
			})
		}

		for id, info := range previousState {
			if _, exists := currentDetected[id]; exists {
				continue
			}
			eventBuffer.DetectionEvents = append(eventBuffer.DetectionEvents, DetectionEvent{
				EventType:       "detection",
				DetectionAction: "lost",
				TimeSec:         currentTime,
				ScountID:        scout.ID,
				LatDeg:          info.LatDeg,
				LonDeg:          info.LonDeg,
				AltM:            info.AltM,
				DistanceM:       info.DistanceM,
				DetectID:        id,
			})
		}

		*detectState = currentDetected
	}
}

func detonationSystem(ecs *ecsLib.ECS) {
	bomRange := BomRangeComponent.GetValue(BomRangeComponent.MustFirst(ecs.World)).Value
	currentTime := getTimeSec(ecs.World)
	eventBuffer := EventBufferComponent.Get(EventBufferComponent.MustFirst(ecs.World))

	query := donburi.NewQuery(filter.Contains(
		RoleComponent,
		StartSecComponent,
		TotalDurationComponent,
		IdComponent,
		PositionComponent,
		HasDetonatedComponent,
	))

	for entry := range query.Iter(ecs.World) {
		role := RoleComponent.GetValue(entry)
		if role != RoleAttacker {
			continue
		}

		hasDetonated := HasDetonatedComponent.GetValue(entry)
		if hasDetonated {
			continue
		}

		startSec := StartSecComponent.GetValue(entry)
		totalDuration := TotalDurationComponent.GetValue(entry)
		endTime := float64(startSec) + totalDuration
		if float64(currentTime) < endTime {
			continue
		}

		pos := PositionComponent.GetValue(entry)
		lat, lon, alt := ecefToGeodetic(pos)
		eventBuffer.DetonationEvents = append(eventBuffer.DetonationEvents, DetonationEvent{
			EventType:  "detonation",
			TimeSec:    currentTime,
			AttackerID: IdComponent.GetValue(entry),
			LatDeg:     lat,
			LonDeg:     lon,
			AltM:       alt,
			BomRangeM:  bomRange,
		})

		HasDetonatedComponent.SetValue(entry, true)
	}
}

func timelineSystem(ecs *ecsLib.ECS) {
	buffer := TimelineBufferComponent.Get(TimelineBufferComponent.MustFirst(ecs.World))
	currentTime := getTimeSec(ecs.World)

	buffer.PositionsScratch = buffer.PositionsScratch[:0]
	query := donburi.NewQuery(filter.Contains(IdComponent, TeamIdComponent, RoleComponent, PositionComponent))
	for entry := range query.Iter(ecs.World) {
		pos := PositionComponent.GetValue(entry)
		lat, lon, alt := ecefToGeodetic(pos)
		buffer.PositionsScratch = append(buffer.PositionsScratch, TimelinePosition{
			ObjectID: IdComponent.GetValue(entry),
			TeamID:   TeamIdComponent.GetValue(entry),
			Role:     string(RoleComponent.GetValue(entry)),
			LatDeg:   lat,
			LonDeg:   lon,
			AltM:     alt,
		})
	}

	buffer.Logs = append(buffer.Logs, TimelineLog{
		TimeSec:   currentTime,
		Positions: buffer.PositionsScratch,
	})
}

func getTimeSec(world donburiWorld) int {
	entry := SimTimeComponent.MustFirst(world)
	return SimTimeComponent.GetValue(entry).TimeSec
}

func positionAtTime(role Role, startSec int, route []RoutePoint, segmentEndSecs []float64, totalDurationSec float64, timeSec float64) Ecef {
	// 司令官は移動しないので、最初の経路点に固定します。
	if role == RoleCommander {
		if len(route) > 0 {
			return route[0].Ecef
		}
		return Ecef{}
	}

	if len(route) == 0 {
		return Ecef{}
	}

	// 移動開始前は最初の経路点に待機します。
	if timeSec < float64(startSec) {
		return route[0].Ecef
	}

	// 経路が1点だけのときは、その位置に固定します。
	if len(segmentEndSecs) == 0 {
		return route[len(route)-1].Ecef
	}

	// 全区間を移動し終えた場合は最終点に固定します。
	elapsed := timeSec - float64(startSec)
	if elapsed >= totalDurationSec {
		return route[len(route)-1].Ecef
	}

	// 経過時間から現在の区間を探し、線形補間で位置を計算します。
	segmentIndex := 0
	for segmentIndex < len(segmentEndSecs) && elapsed > segmentEndSecs[segmentIndex] {
		segmentIndex++
	}
	if segmentIndex >= len(segmentEndSecs) {
		return route[len(route)-1].Ecef
	}

	segmentEnd := segmentEndSecs[segmentIndex]
	segmentStart := 0.0
	if segmentIndex > 0 {
		segmentStart = segmentEndSecs[segmentIndex-1]
	}
	segmentDuration := segmentEnd - segmentStart
	if segmentDuration <= 0 {
		return route[segmentIndex+1].Ecef
	}

	t := (elapsed - segmentStart) / segmentDuration
	a := route[segmentIndex].Ecef
	b := route[segmentIndex+1].Ecef

	return Ecef{
		X: a.X + (b.X-a.X)*t,
		Y: a.Y + (b.Y-a.Y)*t,
		Z: a.Z + (b.Z-a.Z)*t,
	}
}
