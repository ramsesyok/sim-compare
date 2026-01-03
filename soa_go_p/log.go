package main

import "encoding/json"

type TimelineLog struct {
	TimeSec   int                `json:"time_sec"`
	Positions []TimelinePosition `json:"positions"`
}

type TimelinePosition struct {
	ObjectID string  `json:"object_id"`
	TeamID   string  `json:"team_id"`
	Role     string  `json:"role"`
	LatDeg   float64 `json:"lat_deg"`
	LonDeg   float64 `json:"lon_deg"`
	AltM     float64 `json:"alt_m"`
}

type DetectionEvent struct {
	EventType       string  `json:"event_type"`
	DetectionAction string  `json:"detection_action"`
	TimeSec         int     `json:"time_sec"`
	ScountID        string  `json:"scount_id"`
	LatDeg          float64 `json:"lat_deg"`
	LonDeg          float64 `json:"lon_deg"`
	AltM            float64 `json:"alt_m"`
	DistanceM       int     `json:"distance_m"`
	DetectID        string  `json:"detect_id"`
}

type DetonationEvent struct {
	EventType  string  `json:"event_type"`
	TimeSec    int     `json:"time_sec"`
	AttackerID string  `json:"attacker_id"`
	LatDeg     float64 `json:"lat_deg"`
	LonDeg     float64 `json:"lon_deg"`
	AltM       float64 `json:"alt_m"`
	BomRangeM  int     `json:"bom_range_m"`
}

func emitTimelineLog(timeSec int, state *SoaState, positions []TimelinePosition, encoder *json.Encoder) ([]TimelinePosition, error) {
	positions = positions[:0]

	// 1秒ごとの全オブジェクト位置をまとめて1行に出力します。
	for i := range state.IDs {
		lat, lon, alt := ecefToGeodetic(state.Positions[i])
		positions = append(positions, TimelinePosition{
			ObjectID: state.IDs[i],
			TeamID:   state.TeamIDs[i],
			Role:     string(state.Roles[i]),
			LatDeg:   lat,
			LonDeg:   lon,
			AltM:     alt,
		})
	}

	log := TimelineLog{
		TimeSec:   timeSec,
		Positions: positions,
	}

	if err := encoder.Encode(log); err != nil {
		return positions, err
	}
	return positions, nil
}
