package main

import (
	"encoding/json"
	"fmt"
	"os"

	"github.com/spf13/cobra"
	ecsLib "github.com/yohamta/donburi/ecs"
)

// donburiはSystemを関数として登録するECSで、
// ここではバッチ処理として秒刻みでSystemを順番に実行します。

func main() {
	var scenarioPath string
	var timelinePath string
	var eventPath string

	rootCmd := &cobra.Command{
		Use: "donburi_go",
		RunE: func(cmd *cobra.Command, args []string) error {
			if scenarioPath == "" || timelinePath == "" || eventPath == "" {
				return fmt.Errorf("--scenario --timeline-log --event-log を指定してください")
			}
			return runSimulation(scenarioPath, timelinePath, eventPath)
		},
	}

	rootCmd.Flags().StringVar(&scenarioPath, "scenario", "", "シナリオJSONファイルのパス")
	rootCmd.Flags().StringVar(&timelinePath, "timeline-log", "", "タイムラインndjsonの出力先")
	rootCmd.Flags().StringVar(&eventPath, "event-log", "", "イベントndjsonの出力先")

	if err := rootCmd.Execute(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}

func runSimulation(scenarioPath, timelinePath, eventPath string) error {
	scenario, err := loadScenario(scenarioPath)
	if err != nil {
		return err
	}

	world, ecs := buildWorldAndECS(scenario)

	timelineFile, err := os.Create(timelinePath)
	if err != nil {
		return err
	}
	defer timelineFile.Close()

	eventFile, err := os.Create(eventPath)
	if err != nil {
		return err
	}
	defer eventFile.Close()

	eventEncoder := json.NewEncoder(eventFile)
	timelineEncoder := json.NewEncoder(timelineFile)

	endSec := 24 * 60 * 60

	for timeSec := 0; timeSec <= endSec; timeSec++ {
		// Systemが参照する時刻リソースを更新します。
		setSimTime(world, timeSec)

		// 登録したSystemを順に実行します（待機やフレーム制御はありません）。
		ecs.Update()

		// Systemが溜めたイベントをログへ書き出します。
		if err := flushEventBuffer(world, eventEncoder); err != nil {
			return err
		}

		// Systemが溜めたタイムラインをログへ書き出します。
		if err := flushTimelineBuffer(world, timelineEncoder); err != nil {
			return err
		}
	}

	return nil
}

func buildWorldAndECS(scenario *Scenario) (donburiWorld, *ecsLib.ECS) {
	world := newWorld()
	ecs := ecsLib.NewECS(world)

	registerResources(world, scenario)
	spawnEntities(world, scenario)

	ecs.AddSystem(positionUpdateSystem)
	ecs.AddSystem(snapshotSystem)
	ecs.AddSystem(detectionSystem)
	ecs.AddSystem(detonationSystem)
	ecs.AddSystem(timelineSystem)

	return world, ecs
}
