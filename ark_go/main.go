package main

import (
	"fmt"
	"os"
	"time"

	"github.com/mlange-42/ark-tools/app"
	"github.com/spf13/cobra"
)

// Ark-ToolsのSystemを使い、バッチ処理として秒刻みでSystemを順番に実行します。
// 実時間の待機は行わず、できるだけ速く処理を完了させます。

func main() {
	var scenarioPath string
	var timelinePath string
	var eventPath string

	rootCmd := &cobra.Command{
		Use: "ark_go",
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

	app := app.New()
	registerResources(app.World, scenario)
	spawnEntities(app.World, scenario)

	app.AddSystem(&PositionUpdateSystem{})
	app.AddSystem(&SnapshotSystem{})
	app.AddSystem(&DetectionSystem{})
	app.AddSystem(&DetonationSystem{})
	app.AddSystem(&TimelineSystem{})

	app.Initialize()
	defer app.Finalize()

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

	timelineEncoder := newEncoder(timelineFile)
	eventEncoder := newEncoder(eventFile)

	endSec := 24 * 60 * 60

	// シミュレーション全体の処理時間を計測します（ログ出力完了まで）。
	start := time.Now()

	for timeSec := 0; timeSec <= endSec; timeSec++ {
		setSimTime(app.World, timeSec)

		// Updateはスケジューラの待機を行わず、Systemを順に呼び出します。
		app.Update()

		if err := flushEventBuffer(app.World, eventEncoder); err != nil {
			return err
		}
		if err := flushTimelineBuffer(app.World, timelineEncoder); err != nil {
			return err
		}
	}

	fmt.Fprintf(os.Stderr, "simulation elapsed: %s\n", time.Since(start))

	return nil
}
