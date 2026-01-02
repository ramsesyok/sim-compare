package main

import (
	"math"

	"github.com/mlange-42/ark-tools/app"
	"github.com/mlange-42/ark/ecs"
)

type Id struct {
	Value string
}

type TeamId struct {
	Value string
}

type RoleComp struct {
	Value Role
}

type StartSec struct {
	Value int
}

type RoutePoints struct {
	Value []RoutePoint
}

type SegmentEndSecs struct {
	Value []float64
}

type TotalDuration struct {
	Value float64
}

type Position struct {
	Value Ecef
}

type DetectState struct {
	Value map[string]DetectionInfo
}

type HasDetonated struct {
	Value bool
}

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

type EntitySnapshot struct {
	Entity   ecs.Entity
	ID       string
	TeamID   string
	Role     Role
	Position Ecef
}

type PositionUpdateSystem struct {
	filter  *ecs.Filter6[RoleComp, StartSec, RoutePoints, SegmentEndSecs, TotalDuration, Position]
	timeRes ecs.Resource[SimTime]
}

func (s *PositionUpdateSystem) Initialize(w *ecs.World) {
	s.filter = s.filter.New(w)
	s.timeRes = s.timeRes.New(w)
}

func (s *PositionUpdateSystem) Update(w *ecs.World) {
	query := s.filter.Query()
	time := s.timeRes.Get().TimeSec

	for query.Next() {
		role, start, route, segmentEnds, totalDuration, position := query.Get()
		position.Value = positionAtTime(role.Value, start.Value, route.Value, segmentEnds.Value, totalDuration.Value, float64(time))
	}
}

func (s *PositionUpdateSystem) Finalize(w *ecs.World) {}

// SnapshotSystemは探知に必要なスナップショットと空間ハッシュを用意します。
type SnapshotSystem struct {
	filter      *ecs.Filter4[Id, TeamId, RoleComp, Position]
	detectRes   ecs.Resource[DetectRange]
	snapshotRes ecs.Resource[SnapshotCache]
}

func (s *SnapshotSystem) Initialize(w *ecs.World) {
	s.filter = s.filter.New(w)
	s.detectRes = s.detectRes.New(w)
	s.snapshotRes = s.snapshotRes.New(w)
}

func (s *SnapshotSystem) Update(w *ecs.World) {
	snap := s.snapshotRes.Get()
	snap.Snapshots = snap.Snapshots[:0]
	snap.Positions = snap.Positions[:0]

	query := s.filter.Query()
	for query.Next() {
		id, teamID, role, position := query.Get()
		snap.Snapshots = append(snap.Snapshots, EntitySnapshot{
			Entity:   query.Entity(),
			ID:       id.Value,
			TeamID:   teamID.Value,
			Role:     role.Value,
			Position: position.Value,
		})
		snap.Positions = append(snap.Positions, position.Value)
	}

	detectRange := s.detectRes.Get().Value
	// 探知処理のため、位置の配列から空間ハッシュを構築します。
	snap.SpatialHash = buildSpatialHash(snap.Positions, detectRange, snap.SpatialHash)
}

func (s *SnapshotSystem) Finalize(w *ecs.World) {}

// DetectionSystemは斥候の探知イベントを生成します。
type DetectionSystem struct {
	detectRes ecs.Resource[DetectRange]
	snapRes   ecs.Resource[SnapshotCache]
	eventRes  ecs.Resource[EventBuffer]
	timeRes   ecs.Resource[SimTime]
	stateMap  *ecs.Map[DetectState]
}

func (s *DetectionSystem) Initialize(w *ecs.World) {
	s.detectRes = s.detectRes.New(w)
	s.snapRes = s.snapRes.New(w)
	s.eventRes = s.eventRes.New(w)
	s.timeRes = s.timeRes.New(w)
	s.stateMap = ecs.NewMap[DetectState](w)
}

func (s *DetectionSystem) Update(w *ecs.World) {
	detectRange := s.detectRes.Get().Value
	if detectRange <= 0 {
		return
	}

	snap := s.snapRes.Get()
	events := s.eventRes.Get()
	currentTime := s.timeRes.Get().TimeSec

	for _, scout := range snap.Snapshots {
		if scout.Role != RoleScout {
			continue
		}

		detectState := s.stateMap.Get(scout.Entity)
		previous := make(map[string]DetectionInfo, len(detectState.Value))
		for k, v := range detectState.Value {
			previous[k] = v
		}

		currentDetected := make(map[string]DetectionInfo)
		baseKey := cellKey(scout.Position, detectRange)
		for dx := -1; dx <= 1; dx++ {
			for dy := -1; dy <= 1; dy++ {
				for dz := -1; dz <= 1; dz++ {
					key := CellKey{X: baseKey.X + dx, Y: baseKey.Y + dy, Z: baseKey.Z + dz}
					indices, ok := snap.SpatialHash[key]
					if !ok {
						continue
					}

					for _, otherIndex := range indices {
						other := snap.Snapshots[otherIndex]
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
			if _, exists := previous[id]; exists {
				continue
			}
			events.DetectionEvents = append(events.DetectionEvents, DetectionEvent{
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

		for id, info := range previous {
			if _, exists := currentDetected[id]; exists {
				continue
			}
			events.DetectionEvents = append(events.DetectionEvents, DetectionEvent{
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

		detectState.Value = currentDetected
	}
}

func (s *DetectionSystem) Finalize(w *ecs.World) {}

// DetonationSystemは爆破イベントを生成します。
type DetonationSystem struct {
	filter   *ecs.Filter6[RoleComp, StartSec, TotalDuration, Id, Position, HasDetonated]
	bomRes   ecs.Resource[BomRange]
	eventRes ecs.Resource[EventBuffer]
	timeRes  ecs.Resource[SimTime]
}

func (s *DetonationSystem) Initialize(w *ecs.World) {
	s.filter = s.filter.New(w)
	s.bomRes = s.bomRes.New(w)
	s.eventRes = s.eventRes.New(w)
	s.timeRes = s.timeRes.New(w)
}

func (s *DetonationSystem) Update(w *ecs.World) {
	query := s.filter.Query()
	bomRange := s.bomRes.Get().Value
	currentTime := s.timeRes.Get().TimeSec
	events := s.eventRes.Get()

	for query.Next() {
		role, start, totalDuration, id, position, hasDetonated := query.Get()
		if role.Value != RoleAttacker {
			continue
		}
		if hasDetonated.Value {
			continue
		}

		endTime := float64(start.Value) + totalDuration.Value
		if float64(currentTime) < endTime {
			continue
		}

		lat, lon, alt := ecefToGeodetic(position.Value)
		events.DetonationEvents = append(events.DetonationEvents, DetonationEvent{
			EventType:  "detonation",
			TimeSec:    currentTime,
			AttackerID: id.Value,
			LatDeg:     lat,
			LonDeg:     lon,
			AltM:       alt,
			BomRangeM:  bomRange,
		})
		hasDetonated.Value = true
	}
}

func (s *DetonationSystem) Finalize(w *ecs.World) {}

// TimelineSystemはタイムラインログを生成します。
type TimelineSystem struct {
	filter      *ecs.Filter4[Id, TeamId, RoleComp, Position]
	timeRes     ecs.Resource[SimTime]
	timelineRes ecs.Resource[TimelineBuffer]
	positions   []TimelinePosition
}

func (s *TimelineSystem) Initialize(w *ecs.World) {
	s.filter = s.filter.New(w)
	s.timeRes = s.timeRes.New(w)
	s.timelineRes = s.timelineRes.New(w)
}

func (s *TimelineSystem) Update(w *ecs.World) {
	query := s.filter.Query()
	currentTime := s.timeRes.Get().TimeSec
	buffer := s.timelineRes.Get()

	s.positions = s.positions[:0]
	for query.Next() {
		id, teamID, role, position := query.Get()
		lat, lon, alt := ecefToGeodetic(position.Value)
		s.positions = append(s.positions, TimelinePosition{
			ObjectID: id.Value,
			TeamID:   teamID.Value,
			Role:     string(role.Value),
			LatDeg:   lat,
			LonDeg:   lon,
			AltM:     alt,
		})
	}

	buffer.Logs = append(buffer.Logs, TimelineLog{
		TimeSec:   currentTime,
		Positions: s.positions,
	})
}

func (s *TimelineSystem) Finalize(w *ecs.World) {}

func setSimTime(world *ecs.World, timeSec int) {
	res := ecs.NewResource[SimTime](world)
	res.Get().TimeSec = timeSec
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

var _ app.System = (*PositionUpdateSystem)(nil)
var _ app.System = (*SnapshotSystem)(nil)
var _ app.System = (*DetectionSystem)(nil)
var _ app.System = (*DetonationSystem)(nil)
var _ app.System = (*TimelineSystem)(nil)
