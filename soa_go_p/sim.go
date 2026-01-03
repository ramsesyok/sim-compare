package main

import (
	"encoding/json"
	"math"
)

type DetectionInfo struct {
	LatDeg    float64
	LonDeg    float64
	AltM      float64
	DistanceM int
}

func updatePositions(state *SoaState, timeSec float64) {
	for i := range state.IDs {
		state.Positions[i] = positionAtTime(state, i, timeSec)
	}
}

func positionAtTime(state *SoaState, index int, timeSec float64) Ecef {
	// 司令官は移動しないので、最初の経路点に固定します。
	if state.Roles[index] == RoleCommander {
		if len(state.Routes[index]) > 0 {
			return state.Routes[index][0].Ecef
		}
		return Ecef{}
	}

	if len(state.Routes[index]) == 0 {
		return Ecef{}
	}

	// 移動開始前は最初の経路点に待機します。
	if timeSec < float64(state.StartSecs[index]) {
		return state.Routes[index][0].Ecef
	}

	// 経路が1点だけのときは、その位置に固定します。
	if len(state.SegmentEndSecs[index]) == 0 {
		return state.Routes[index][len(state.Routes[index])-1].Ecef
	}

	// 全区間を移動し終えた場合は最終点に固定します。
	elapsed := timeSec - float64(state.StartSecs[index])
	if elapsed >= state.TotalDurations[index] {
		return state.Routes[index][len(state.Routes[index])-1].Ecef
	}

	// 経過時間から現在の区間を探し、線形補間で位置を計算します。
	segmentIndex := 0
	for segmentIndex < len(state.SegmentEndSecs[index]) && elapsed > state.SegmentEndSecs[index][segmentIndex] {
		segmentIndex++
	}
	if segmentIndex >= len(state.SegmentEndSecs[index]) {
		return state.Routes[index][len(state.Routes[index])-1].Ecef
	}

	segmentEnd := state.SegmentEndSecs[index][segmentIndex]
	segmentStart := 0.0
	if segmentIndex > 0 {
		segmentStart = state.SegmentEndSecs[index][segmentIndex-1]
	}
	segmentDuration := segmentEnd - segmentStart
	if segmentDuration <= 0 {
		return state.Routes[index][segmentIndex+1].Ecef
	}

	t := (elapsed - segmentStart) / segmentDuration
	a := state.Routes[index][segmentIndex].Ecef
	b := state.Routes[index][segmentIndex+1].Ecef

	return Ecef{
		X: a.X + (b.X-a.X)*t,
		Y: a.Y + (b.Y-a.Y)*t,
		Z: a.Z + (b.Z-a.Z)*t,
	}
}

func emitDetectionEvents(timeSec int, detectRange float64, spatialHash map[CellKey][]int, state *SoaState, encoder *json.Encoder) error {
	if detectRange <= 0 {
		return nil
	}

	for i := range state.IDs {
		if state.Roles[i] != RoleScout {
			continue
		}

		scoutPos := state.Positions[i]
		scoutTeam := state.TeamIDs[i]
		scoutID := state.IDs[i]

		currentDetected := make(map[string]DetectionInfo)

		baseKey := cellKey(scoutPos, detectRange)
		for dx := -1; dx <= 1; dx++ {
			for dy := -1; dy <= 1; dy++ {
				for dz := -1; dz <= 1; dz++ {
					key := CellKey{X: baseKey.X + dx, Y: baseKey.Y + dy, Z: baseKey.Z + dz}
					indices, ok := spatialHash[key]
					if !ok {
						continue
					}

					for _, otherIndex := range indices {
						if otherIndex == i {
							continue
						}
						if state.TeamIDs[otherIndex] == scoutTeam {
							continue
						}

						// 探知範囲内かどうかを最終的に距離で判定します。
						distance := distanceECEF(scoutPos, state.Positions[otherIndex])
						if distance > detectRange {
							continue
						}

						lat, lon, alt := ecefToGeodetic(state.Positions[otherIndex])
						info := DetectionInfo{
							LatDeg:    lat,
							LonDeg:    lon,
							AltM:      alt,
							DistanceM: int(math.Round(distance)),
						}
						currentDetected[state.IDs[otherIndex]] = info
					}
				}
			}
		}

		previousIDs := make(map[string]struct{})
		for id := range state.DetectState[i] {
			previousIDs[id] = struct{}{}
		}
		currentIDs := make(map[string]struct{})
		for id := range currentDetected {
			currentIDs[id] = struct{}{}
		}

		for id := range currentIDs {
			if _, exists := previousIDs[id]; exists {
				continue
			}
			info := currentDetected[id]
			event := DetectionEvent{
				EventType:       "detection",
				DetectionAction: "found",
				TimeSec:         timeSec,
				ScountID:        scoutID,
				LatDeg:          info.LatDeg,
				LonDeg:          info.LonDeg,
				AltM:            info.AltM,
				DistanceM:       info.DistanceM,
				DetectID:        id,
			}
			if err := encoder.Encode(event); err != nil {
				return err
			}
		}

		for id := range previousIDs {
			if _, exists := currentIDs[id]; exists {
				continue
			}
			info := state.DetectState[i][id]
			event := DetectionEvent{
				EventType:       "detection",
				DetectionAction: "lost",
				TimeSec:         timeSec,
				ScountID:        scoutID,
				LatDeg:          info.LatDeg,
				LonDeg:          info.LonDeg,
				AltM:            info.AltM,
				DistanceM:       info.DistanceM,
				DetectID:        id,
			}
			if err := encoder.Encode(event); err != nil {
				return err
			}
		}

		state.DetectState[i] = currentDetected
	}

	return nil
}

func emitDetonationEvents(timeSec int, bomRange int, state *SoaState, encoder *json.Encoder) error {
	// 攻撃役が最終地点に到達した時点で1回だけ爆破イベントを出します。
	for i := range state.IDs {
		if state.Roles[i] != RoleAttacker {
			continue
		}
		if state.HasDetonated[i] {
			continue
		}

		endTime := float64(state.StartSecs[i]) + state.TotalDurations[i]
		if float64(timeSec) >= endTime {
			lat, lon, alt := ecefToGeodetic(state.Positions[i])
			event := DetonationEvent{
				EventType:  "detonation",
				TimeSec:    timeSec,
				AttackerID: state.IDs[i],
				LatDeg:     lat,
				LonDeg:     lon,
				AltM:       alt,
				BomRangeM:  bomRange,
			}
			if err := encoder.Encode(event); err != nil {
				return err
			}
			state.HasDetonated[i] = true
		}
	}

	return nil
}
