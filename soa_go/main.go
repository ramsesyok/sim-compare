package main

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"
)

// SoAは「同じ種類のデータを配列でまとめる」設計で、
// ここではID配列・役割配列・位置配列などを個別に保持しています。
// 初心者でも追いやすいように、毎秒すべての要素を順番に更新します。

func main() {
	var scenarioPath string
	var timelinePath string
	var eventPath string

	rootCmd := &cobra.Command{
		Use: "soa_go",
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

	state := buildState(scenario)

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

	detectRange := float64(scenario.Performance.Scout.DetectRangeM)
	bomRange := scenario.Performance.Attacker.BomRangeM

	endSec := 24 * 60 * 60

	for timeSec := 0; timeSec <= endSec; timeSec++ {
		// まず全オブジェクトの位置を更新します。
		updatePositions(&state, float64(timeSec))

		// 空間ハッシュを作り、近傍探索を高速化します。
		spatialHash := buildSpatialHash(state.Positions, detectRange)

		// 斥候の探知イベントを判定してイベントログへ出力します。
		if err := emitDetectionEvents(timeSec, detectRange, spatialHash, &state, eventFile); err != nil {
			return err
		}

		// 攻撃役の爆破イベントを判定してイベントログへ出力します。
		if err := emitDetonationEvents(timeSec, bomRange, &state, eventFile); err != nil {
			return err
		}

		// 全オブジェクトの位置をタイムラインログへ出力します。
		if err := emitTimelineLog(timeSec, &state, timelineFile); err != nil {
			return err
		}
	}

	return nil
}
