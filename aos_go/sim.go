package main

import (
	"encoding/json"
	"math"
	"os"
)

type DetectionInfo struct {
	LatDeg    float64
	LonDeg    float64
	AltM      float64
	DistanceM int
}

func positionAtTime(object ObjectState, timeSec float64) Ecef {
	// 司令官は移動しないので、最初の経路点に固定します。
	if object.Role == RoleCommander {
		if len(object.Route) > 0 {
			return object.Route[0].Ecef
		}
		return Ecef{}
	}

	if len(object.Route) == 0 {
		return Ecef{}
	}

	// 移動開始前は最初の経路点に待機します。
	if timeSec < float64(object.StartSec) {
		return object.Route[0].Ecef
	}

	// 経路が1点だけのときは、その位置に固定します。
	if len(object.SegmentEndSecs) == 0 {
		return object.Route[len(object.Route)-1].Ecef
	}

	// 全区間を移動し終えた場合は最終点に固定します。
	elapsed := timeSec - float64(object.StartSec)
	if elapsed >= object.TotalDurationSec {
		return object.Route[len(object.Route)-1].Ecef
	}

	// 経過時間から現在の区間を探し、線形補間で位置を計算します。
	segmentIndex := 0
	for segmentIndex < len(object.SegmentEndSecs) && elapsed > object.SegmentEndSecs[segmentIndex] {
		segmentIndex++
	}
	if segmentIndex >= len(object.SegmentEndSecs) {
		return object.Route[len(object.Route)-1].Ecef
	}

	segmentEnd := object.SegmentEndSecs[segmentIndex]
	segmentStart := 0.0
	if segmentIndex > 0 {
		segmentStart = object.SegmentEndSecs[segmentIndex-1]
	}
	segmentDuration := segmentEnd - segmentStart
	if segmentDuration <= 0 {
		return object.Route[segmentIndex+1].Ecef
	}

	t := (elapsed - segmentStart) / segmentDuration
	a := object.Route[segmentIndex].Ecef
	b := object.Route[segmentIndex+1].Ecef

	return Ecef{
		X: a.X + (b.X-a.X)*t,
		Y: a.Y + (b.Y-a.Y)*t,
		Z: a.Z + (b.Z-a.Z)*t,
	}
}

func emitDetectionEvents(timeSec int, detectRange float64, spatialHash map[CellKey][]int, objects []ObjectState, eventFile *os.File) error {
	if detectRange <= 0 {
		return nil
	}

	encoder := json.NewEncoder(eventFile)

	for i := range objects {
		if objects[i].Role != RoleScout {
			continue
		}

		scoutPos := objects[i].Position
		scoutTeam := objects[i].TeamID
		scoutID := objects[i].ID

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
						other := objects[otherIndex]
						if other.TeamID == scoutTeam {
							continue
						}

						// 探知範囲内かどうかを最終的に距離で判定します。
						distance := distanceECEF(scoutPos, other.Position)
						if distance > detectRange {
							continue
						}

						lat, lon, alt := ecefToGeodetic(other.Position)
						info := DetectionInfo{
							LatDeg:    lat,
							LonDeg:    lon,
							AltM:      alt,
							DistanceM: int(math.Round(distance)),
						}
						currentDetected[other.ID] = info
					}
				}
			}
		}

		previousIDs := make(map[string]struct{})
		for id := range objects[i].DetectState {
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
			info := objects[i].DetectState[id]
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

		objects[i].DetectState = currentDetected
	}

	return nil
}

func emitDetonationEvents(timeSec int, bomRange int, objects []ObjectState, eventFile *os.File) error {
	encoder := json.NewEncoder(eventFile)

	// 攻撃役が最終地点に到達した時点で1回だけ爆破イベントを出します。
	for i := range objects {
		if objects[i].Role != RoleAttacker {
			continue
		}
		if objects[i].HasDetonated {
			continue
		}

		endTime := float64(objects[i].StartSec) + objects[i].TotalDurationSec
		if float64(timeSec) >= endTime {
			lat, lon, alt := ecefToGeodetic(objects[i].Position)
			event := DetonationEvent{
				EventType:  "detonation",
				TimeSec:    timeSec,
				AttackerID: objects[i].ID,
				LatDeg:     lat,
				LonDeg:     lon,
				AltM:       alt,
				BomRangeM:  bomRange,
			}
			if err := encoder.Encode(event); err != nil {
				return err
			}
			objects[i].HasDetonated = true
		}
	}

	return nil
}
