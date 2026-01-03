package main

import (
	"os"
	"runtime"
	"strconv"
	"sync"

	"github.com/yohamta/donburi"
	"github.com/yohamta/donburi/filter"
)

const defaultChunkSize = 256

type positionUpdateJob struct {
	start   int
	end     int
	timeSec float64
	wait    *sync.WaitGroup
}

type PositionUpdater struct {
	world      donburiWorld
	entities   []donburi.Entity
	chunkSize  int
	jobs       chan positionUpdateJob
	workerWait sync.WaitGroup
}

func NewPositionUpdater(world donburiWorld, chunkSize int, workerCount int) *PositionUpdater {
	if chunkSize <= 0 {
		chunkSize = defaultChunkSize
	}
	if workerCount <= 0 {
		workerCount = 1
	}

	query := donburi.NewQuery(filter.Contains(
		RoleComponent,
		StartSecComponent,
		RoutePointsComponent,
		SegmentEndSecsComponent,
		TotalDurationComponent,
		PositionComponent,
	))
	entities := make([]donburi.Entity, 0, 1024)
	for entry := range query.Iter(world) {
		entities = append(entities, entry.Entity())
	}

	updater := &PositionUpdater{
		world:     world,
		entities:  entities,
		chunkSize: chunkSize,
		jobs:      make(chan positionUpdateJob),
	}

	// 位置更新を同時に進めるための作業者を用意し、チャンク単位の仕事を順番に受け取って処理します。
	updater.workerWait.Add(workerCount)
	for i := 0; i < workerCount; i++ {
		go func() {
			defer updater.workerWait.Done()
			for job := range updater.jobs {
				for index := job.start; index < job.end; index++ {
					entry := updater.world.Entry(updater.entities[index])
					role := RoleComponent.GetValue(entry)
					startSec := StartSecComponent.GetValue(entry)
					route := RoutePointsComponent.GetValue(entry)
					segmentEnds := SegmentEndSecsComponent.GetValue(entry)
					totalDuration := TotalDurationComponent.GetValue(entry)
					pos := positionAtTime(role, startSec, route, segmentEnds, totalDuration, job.timeSec)
					PositionComponent.SetValue(entry, pos)
				}
				job.wait.Done()
			}
		}()
	}

	return updater
}

func (u *PositionUpdater) Update(timeSec float64) {
	if len(u.entities) == 0 {
		return
	}

	var wg sync.WaitGroup
	for start := 0; start < len(u.entities); start += u.chunkSize {
		end := start + u.chunkSize
		if end > len(u.entities) {
			end = len(u.entities)
		}
		wg.Add(1)
		u.jobs <- positionUpdateJob{
			start:   start,
			end:     end,
			timeSec: timeSec,
			wait:    &wg,
		}
	}
	wg.Wait()
}

func (u *PositionUpdater) Close() {
	close(u.jobs)
	u.workerWait.Wait()
}

func resolveChunkSize() int {
	// チャンクサイズは環境変数で外から変えられるようにし、計測や比較をしやすくします。
	if value, ok := parsePositiveEnvInt("CHUNK_SIZE"); ok {
		return value
	}
	return defaultChunkSize
}

func resolveParallelism() int {
	// 並列数を環境変数で指定したいときに備え、指定された値をGOMAXPROCSにも反映します。
	if value, ok := parsePositiveEnvInt("PARALLELISM"); ok {
		runtime.GOMAXPROCS(value)
		return value
	}
	// 既存のGOMAXPROCS設定（環境変数GOMAXPROCSを含む）をそのまま使います。
	current := runtime.GOMAXPROCS(0)
	if current < 1 {
		return 1
	}
	return current
}

func parsePositiveEnvInt(name string) (int, bool) {
	value := os.Getenv(name)
	if value == "" {
		return 0, false
	}
	parsed, err := strconv.Atoi(value)
	if err != nil || parsed <= 0 {
		return 0, false
	}
	return parsed, true
}
