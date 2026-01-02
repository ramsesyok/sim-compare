package main

import (
	"fmt"
	"os"
	"time"

	"github.com/spf13/cobra"
)

// AoSは「1つのオブジェクトに必要な情報をまとめて持つ」設計で、
// ここでは各オブジェクトの状態（位置・経路・検知履歴など）を1つの構造体に詰めています。
// 初心者でも追いやすいように、毎秒すべてのオブジェクトを順番に更新します。

func main() {
	var scenarioPath string
	var timelinePath string
	var eventPath string

	rootCmd := &cobra.Command{
		Use: "aos_go",
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

	objects := buildObjects(scenario)

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

	// シミュレーション全体の処理時間を計測します（ログ出力完了まで）。
	start := time.Now()

	for timeSec := 0; timeSec <= endSec; timeSec++ {
		// まず全オブジェクトの位置を更新します。
		for i := range objects {
			objects[i].Position = positionAtTime(objects[i], float64(timeSec))
		}

		// 空間ハッシュを作り、近傍探索を高速化します。
		spatialHash := buildSpatialHash(objects, detectRange)

		// 斥候の探知イベントを判定してイベントログへ出力します。
		if err := emitDetectionEvents(timeSec, detectRange, spatialHash, objects, eventFile); err != nil {
			return err
		}

		// 攻撃役の爆破イベントを判定してイベントログへ出力します。
		if err := emitDetonationEvents(timeSec, bomRange, objects, eventFile); err != nil {
			return err
		}

		// 全オブジェクトの位置をタイムラインログへ出力します。
		if err := emitTimelineLog(timeSec, objects, timelineFile); err != nil {
			return err
		}
	}

	fmt.Fprintf(os.Stderr, "simulation elapsed: %s\n", time.Since(start))

	return nil
}
