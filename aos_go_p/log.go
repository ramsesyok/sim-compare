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

func emitTimelineLog(timeSec int, objects []ObjectState, positions []TimelinePosition, encoder *json.Encoder) ([]TimelinePosition, error) {
	positions = positions[:0]

	// 1秒ごとの全オブジェクト位置をまとめて1行に出力します。
	for _, obj := range objects {
		lat, lon, alt := ecefToGeodetic(obj.Position)
		positions = append(positions, TimelinePosition{
			ObjectID: obj.ID,
			TeamID:   obj.TeamID,
			Role:     string(obj.Role),
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
