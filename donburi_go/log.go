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

type TimelineBuffer struct {
	Logs             []TimelineLog
	PositionsScratch []TimelinePosition
}

type EventBuffer struct {
	DetectionEvents  []DetectionEvent
	DetonationEvents []DetonationEvent
}

func writeNDJSON(encoder *json.Encoder, value any) error {
	return encoder.Encode(value)
}

func flushEventBuffer(world donburiWorld, encoder *json.Encoder) error {
	entry := EventBufferComponent.MustFirst(world)
	buffer := EventBufferComponent.Get(entry)

	for _, event := range buffer.DetectionEvents {
		if err := writeNDJSON(encoder, event); err != nil {
			return err
		}
	}
	for _, event := range buffer.DetonationEvents {
		if err := writeNDJSON(encoder, event); err != nil {
			return err
		}
	}

	buffer.DetectionEvents = buffer.DetectionEvents[:0]
	buffer.DetonationEvents = buffer.DetonationEvents[:0]
	return nil
}

func flushTimelineBuffer(world donburiWorld, encoder *json.Encoder) error {
	entry := TimelineBufferComponent.MustFirst(world)
	buffer := TimelineBufferComponent.Get(entry)

	for _, log := range buffer.Logs {
		if err := writeNDJSON(encoder, log); err != nil {
			return err
		}
	}

	buffer.Logs = buffer.Logs[:0]
	return nil
}
