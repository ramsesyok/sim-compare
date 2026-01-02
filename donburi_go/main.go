package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"os"
	"runtime"
	"runtime/pprof"
	"time"

	"github.com/spf13/cobra"
	ecsLib "github.com/yohamta/donburi/ecs"
)

// donburiはSystemを関数として登録するECSで、
// ここではバッチ処理として秒刻みでSystemを順番に実行します。

func main() {
	var scenarioPath string
	var timelinePath string
	var eventPath string
	var cpuProfilePath string
	var memProfilePath string

	rootCmd := &cobra.Command{
		Use: "donburi_go",
		RunE: func(cmd *cobra.Command, args []string) error {
			if scenarioPath == "" || timelinePath == "" || eventPath == "" {
				return fmt.Errorf("--scenario --timeline-log --event-log を指定してください")
			}
			// CPUプロファイルは「処理中にどこで時間を使っているか」を記録するため、
			// 指定されたときだけ開始し、終了時に必ず停止してファイルへ書き込みます。
			if cpuProfilePath != "" {
				cpuFile, err := os.Create(cpuProfilePath)
				if err != nil {
					return err
				}
				defer cpuFile.Close()
				if err := pprof.StartCPUProfile(cpuFile); err != nil {
					return err
				}
				defer pprof.StopCPUProfile()
			}

			if err := runSimulation(scenarioPath, timelinePath, eventPath); err != nil {
				return err
			}

			// メモリプロファイルは「終了時点のヒープの内訳」を記録するため、
			// いったんGCを走らせてからスナップショットを保存します。
			if memProfilePath != "" {
				memFile, err := os.Create(memProfilePath)
				if err != nil {
					return err
				}
				defer memFile.Close()
				runtime.GC()
				if err := pprof.WriteHeapProfile(memFile); err != nil {
					return err
				}
			}

			return nil
		},
	}

	rootCmd.Flags().StringVar(&scenarioPath, "scenario", "", "シナリオJSONファイルのパス")
	rootCmd.Flags().StringVar(&timelinePath, "timeline-log", "", "タイムラインndjsonの出力先")
	rootCmd.Flags().StringVar(&eventPath, "event-log", "", "イベントndjsonの出力先")
	rootCmd.Flags().StringVar(&cpuProfilePath, "cpu-profile", "", "CPUプロファイルの出力先")
	rootCmd.Flags().StringVar(&memProfilePath, "mem-profile", "", "メモリプロファイルの出力先")

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

	eventWriter := bufio.NewWriter(eventFile)
	timelineWriter := bufio.NewWriter(timelineFile)
	eventEncoder := json.NewEncoder(eventWriter)
	timelineEncoder := json.NewEncoder(timelineWriter)

	endSec := 24 * 60 * 60

	// シミュレーション全体の処理時間を計測します（ログ出力完了まで）。
	start := time.Now()

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

	if err := eventWriter.Flush(); err != nil {
		return err
	}
	if err := timelineWriter.Flush(); err != nil {
		return err
	}

	fmt.Fprintf(os.Stderr, "donburi_go: simulation elapsed: %s\n", time.Since(start))

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
