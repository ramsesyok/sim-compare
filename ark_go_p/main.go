package main

import (
	"bufio"
	"fmt"
	"os"
	"runtime"
	"runtime/pprof"
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
	var cpuProfilePath string
	var memProfilePath string

	rootCmd := &cobra.Command{
		Use: "ark_go",
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
	// ECSは「エンティティとコンポーネントを組み合わせる」方式で、
	// ここでは位置コンポーネントだけを並列更新して性能差を測ります。
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
	timelineWriter := bufio.NewWriter(timelineFile)

	eventFile, err := os.Create(eventPath)
	if err != nil {
		return err
	}
	defer eventFile.Close()
	eventWriter := bufio.NewWriter(eventFile)

	timelineEncoder := newEncoder(timelineWriter)
	eventEncoder := newEncoder(eventWriter)

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

	if err := eventWriter.Flush(); err != nil {
		return err
	}
	if err := timelineWriter.Flush(); err != nil {
		return err
	}

	fmt.Fprintf(os.Stderr, "ark_go: simulation elapsed: %fs\n", time.Since(start).Seconds())

	return nil
}
