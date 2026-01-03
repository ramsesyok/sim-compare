package main

import (
	"os"
	"runtime"
	"strconv"
	"sync"
)

const defaultChunkSize = 256

type positionUpdateJob struct {
	start   int
	end     int
	timeSec float64
	wait    *sync.WaitGroup
}

type PositionUpdater struct {
	objects    []ObjectState
	chunkSize  int
	jobs       chan positionUpdateJob
	workerWait sync.WaitGroup
}

func NewPositionUpdater(objects []ObjectState, chunkSize int, workerCount int) *PositionUpdater {
	if chunkSize <= 0 {
		chunkSize = defaultChunkSize
	}
	if workerCount <= 0 {
		workerCount = 1
	}

	updater := &PositionUpdater{
		objects:   objects,
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
					updater.objects[index].Position = positionAtTime(updater.objects[index], job.timeSec)
				}
				job.wait.Done()
			}
		}()
	}

	return updater
}

func (u *PositionUpdater) Update(timeSec float64) {
	if len(u.objects) == 0 {
		return
	}

	var wg sync.WaitGroup
	for start := 0; start < len(u.objects); start += u.chunkSize {
		end := start + u.chunkSize
		if end > len(u.objects) {
			end = len(u.objects)
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
