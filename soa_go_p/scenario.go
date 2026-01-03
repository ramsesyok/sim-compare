package main

import (
	"encoding/json"
	"os"
)

type Scenario struct {
	Performance Performance `json:"performance"`
	Teams       []Team      `json:"teams"`
}

type Performance struct {
	Scout     ScoutPerformance     `json:"scout"`
	Messenger MessengerPerformance `json:"messenger"`
	Attacker  AttackerPerformance  `json:"attacker"`
}

type ScoutPerformance struct {
	CommRangeM   int `json:"comm_range_m"`
	DetectRangeM int `json:"detect_range_m"`
}

type MessengerPerformance struct {
	CommRangeM int `json:"comm_range_m"`
}

type AttackerPerformance struct {
	BomRangeM int `json:"bom_range_m"`
}

type Team struct {
	ID      string           `json:"id"`
	Name    string           `json:"name"`
	Objects []ObjectScenario `json:"objects"`
}

type ObjectScenario struct {
	ID       string     `json:"id"`
	Role     Role       `json:"role"`
	StartSec int        `json:"start_sec"`
	Route    []Waypoint `json:"route"`
	Network  []string   `json:"network"`
}

type Role string

const (
	RoleCommander Role = "commander"
	RoleScout     Role = "scout"
	RoleMessenger Role = "messenger"
	RoleAttacker  Role = "attacker"
)

type Waypoint struct {
	LatDeg    float64 `json:"lat_deg"`
	LonDeg    float64 `json:"lon_deg"`
	AltM      float64 `json:"alt_m"`
	SpeedsKph float64 `json:"speeds_kph"`
}

func loadScenario(path string) (*Scenario, error) {
	// シナリオJSONを読み込み、シミュレーションの元データを構築します。
	scenarioFile, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer scenarioFile.Close()

	var scenario Scenario
	if err := json.NewDecoder(scenarioFile).Decode(&scenario); err != nil {
		return nil, err
	}

	return &scenario, nil
}
