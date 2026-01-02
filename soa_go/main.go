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
)

// SoAは「同じ種類のデータを配列でまとめる」設計で、
// ここではID配列・役割配列・位置配列などを個別に保持しています。
// 初心者でも追いやすいように、毎秒すべての要素を順番に更新します。

func main() {
	var scenarioPath string
	var timelinePath string
	var eventPath string
	var cpuProfilePath string
	var memProfilePath string

	rootCmd := &cobra.Command{
		Use: "soa_go",
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

	state := buildState(scenario)

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

	timelineEncoder := json.NewEncoder(timelineWriter)
	eventEncoder := json.NewEncoder(eventWriter)

	detectRange := float64(scenario.Performance.Scout.DetectRangeM)
	bomRange := scenario.Performance.Attacker.BomRangeM

	endSec := 24 * 60 * 60
	spatialHash := make(map[CellKey][]int)
	positionsScratch := make([]TimelinePosition, 0, len(state.IDs))

	// シミュレーション全体の処理時間を計測します（ログ出力完了まで）。
	start := time.Now()

	for timeSec := 0; timeSec <= endSec; timeSec++ {
		// まず全オブジェクトの位置を更新します。
		updatePositions(&state, float64(timeSec))

		// 空間ハッシュを作り、近傍探索を高速化します。
		spatialHash = buildSpatialHash(state.Positions, detectRange, spatialHash)

		// 斥候の探知イベントを判定してイベントログへ出力します。
		if err := emitDetectionEvents(timeSec, detectRange, spatialHash, &state, eventEncoder); err != nil {
			return err
		}

		// 攻撃役の爆破イベントを判定してイベントログへ出力します。
		if err := emitDetonationEvents(timeSec, bomRange, &state, eventEncoder); err != nil {
			return err
		}

		// 全オブジェクトの位置をタイムラインログへ出力します。
		var err error
		positionsScratch, err = emitTimelineLog(timeSec, &state, positionsScratch, timelineEncoder)
		if err != nil {
			return err
		}
	}

	if err := eventWriter.Flush(); err != nil {
		return err
	}
	if err := timelineWriter.Flush(); err != nil {
		return err
	}

	fmt.Fprintf(os.Stderr, "soa_go: simulation elapsed: %fs\n", time.Since(start).Seconds())

	return nil
}
