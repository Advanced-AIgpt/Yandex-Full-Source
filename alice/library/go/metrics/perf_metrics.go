package metrics

import (
	"runtime"

	"a.yandex-team.ru/library/go/core/metrics"
)

type PerfMetrics struct {
	// -- General statistics.
	// Number of goroutines that currently exist
	GoroutineNums metrics.Gauge
	// TotalHeapAlloc is cumulative bytes allocated for heap objects.
	TotalHeapAlloc metrics.Gauge
	// Sys is the total bytes of memory obtained from the OS.
	Sys metrics.Gauge
	// Lookups is the number of pointer lookups performed by the runtime.
	Lookups metrics.Gauge
	// Mallocs is the cumulative count of heap objects allocated.
	// The number of live objects is Mallocs - Frees.
	Mallocs metrics.Gauge
	// Frees is the cumulative count of heap objects freed.
	Frees metrics.Gauge

	// -- Heap memory statistics.
	// HeapAlloc is bytes of allocated heap objects.
	HeapAlloc metrics.Gauge
	// HeapSys is bytes of heap memory obtained from the OS.
	HeapSys metrics.Gauge
	// HeapIdle is bytes in idle (unused) spans.
	HeapIdle metrics.Gauge
	// HeapInUse is bytes in in-use spans.
	HeapInUse metrics.Gauge
	// HeapReleased is bytes of physical memory returned to the OS.
	HeapReleased metrics.Gauge
	// HeapObjects is the number of allocated heap objects.
	HeapObjects metrics.Gauge

	// -- Stack memory statistics.
	// StackInUse is bytes in stack spans.
	StackInUse metrics.Gauge
	// StackSys is bytes of stack memory obtained from the OS.
	StackSys metrics.Gauge

	// -- Off-heap memory statistics.
	// MSpanInUse is bytes of allocated mspan structures.
	MSpanInUse metrics.Gauge
	// MSpanSys is bytes of memory obtained from the OS for mspan
	// structures.
	MSpanSys metrics.Gauge
	// MCacheInUse is bytes of allocated mcache structures.
	MCacheInUse metrics.Gauge
	// MCacheSys is bytes of memory obtained from the OS for
	// mcache structures.
	MCacheSys metrics.Gauge
	// BuckHashSys is bytes of memory in profiling bucket hash tables.
	BuckHashSys metrics.Gauge
	// GCSys is bytes of memory in garbage collection metadata.
	GCSys metrics.Gauge
	// OtherSys is bytes of memory in miscellaneous off-heap
	// runtime allocations.
	OtherSys metrics.Gauge

	// -- Garbage collector statistics.
	// NextGC is the target heap size of the next GC cycle.
	NextGC metrics.Gauge
	// PauseTotalNs is the cumulative nanoseconds in GC
	// stop-the-world pauses since the program started.
	// During a stop-the-world pause, all goroutines are paused
	// and only the garbage collector can run.
	PauseTotalNs metrics.Gauge
	// NumGC is the number of completed GC cycles.
	NumGC metrics.Gauge
	// NumForcedGC is the number of GC cycles that were forced by
	// the application calling the GC function.
	NumForcedGC metrics.Gauge
}

func NewPerfMetrics(registry metrics.Registry) *PerfMetrics {
	perfMetrics := &PerfMetrics{}

	// general statistics
	generalPerfRegistry := registry.WithPrefix("general")
	perfMetrics.GoroutineNums = generalPerfRegistry.Gauge("goroutines")
	perfMetrics.TotalHeapAlloc = generalPerfRegistry.Gauge("total_heap_alloc_bytes")
	perfMetrics.Sys = generalPerfRegistry.Gauge("total_sys_bytes")
	perfMetrics.Lookups = generalPerfRegistry.Gauge("lookups")
	perfMetrics.Mallocs = generalPerfRegistry.Gauge("mallocs")
	perfMetrics.Frees = generalPerfRegistry.Gauge("frees")

	// memory statistics
	memoryPerfRegistry := registry.WithPrefix("memory")
	// heap memory statistics
	heapMemoryPerfRegistry := memoryPerfRegistry.WithTags(map[string]string{"memory": "heap"})
	perfMetrics.HeapAlloc = heapMemoryPerfRegistry.Gauge("alloc_bytes")
	perfMetrics.HeapIdle = heapMemoryPerfRegistry.Gauge("idle_bytes")
	perfMetrics.HeapReleased = heapMemoryPerfRegistry.Gauge("released_bytes")
	perfMetrics.HeapObjects = heapMemoryPerfRegistry.Gauge("objects")
	perfMetrics.HeapSys = heapMemoryPerfRegistry.Gauge("sys_bytes")
	perfMetrics.HeapInUse = heapMemoryPerfRegistry.Gauge("in_use_bytes")

	//stack memory statistics
	stackMemoryPerfRegistry := memoryPerfRegistry.WithTags(map[string]string{"memory": "stack"})
	perfMetrics.StackSys = stackMemoryPerfRegistry.Gauge("sys_bytes")
	perfMetrics.StackInUse = stackMemoryPerfRegistry.Gauge("in_use_bytes")

	//mspan memory statistics
	mspanMemoryPerfRegistry := memoryPerfRegistry.WithTags(map[string]string{"memory": "mspan"})
	perfMetrics.MSpanInUse = mspanMemoryPerfRegistry.Gauge("in_use_bytes")
	perfMetrics.MSpanSys = mspanMemoryPerfRegistry.Gauge("sys_bytes")

	//mcache memory statistics
	mcacheMemoryPerfRegistry := memoryPerfRegistry.WithTags(map[string]string{"memory": "mcache"})
	perfMetrics.MCacheInUse = mcacheMemoryPerfRegistry.Gauge("in_use_bytes")
	perfMetrics.MCacheSys = mcacheMemoryPerfRegistry.Gauge("sys_bytes")

	//buckhash memory statistics
	buckHashMemoryPerfRegistry := memoryPerfRegistry.WithTags(map[string]string{"memory": "buckhash"})
	perfMetrics.BuckHashSys = buckHashMemoryPerfRegistry.Gauge("sys_bytes")

	//gc memory statistics
	gcMemoryPerfRegistry := memoryPerfRegistry.WithTags(map[string]string{"memory": "gc"})
	perfMetrics.GCSys = gcMemoryPerfRegistry.Gauge("sys_bytes")

	//other memory statistics
	otherPerfRegistry := memoryPerfRegistry.WithTags(map[string]string{"memory": "other"})
	perfMetrics.OtherSys = otherPerfRegistry.Gauge("sys_bytes")

	// gc statistics
	gcPerfRegistry := registry.WithPrefix("gc")
	perfMetrics.NextGC = gcPerfRegistry.Gauge("target_heap_size")
	perfMetrics.PauseTotalNs = gcPerfRegistry.Gauge("pause_total_ns")
	perfMetrics.NumGC = gcPerfRegistry.Gauge("cycles")
	perfMetrics.NumForcedGC = gcPerfRegistry.Gauge("forced_cycles")

	return perfMetrics
}

func (perfMetrics *PerfMetrics) UpdateCurrentState() {
	var rtm runtime.MemStats
	runtime.ReadMemStats(&rtm)

	// general statistics
	perfMetrics.GoroutineNums.Set(float64(runtime.NumGoroutine()))
	perfMetrics.TotalHeapAlloc.Set(float64(rtm.TotalAlloc))
	perfMetrics.Sys.Set(float64(rtm.Sys))
	perfMetrics.Lookups.Set(float64(rtm.Lookups))
	perfMetrics.Mallocs.Set(float64(rtm.Mallocs))
	perfMetrics.Frees.Set(float64(rtm.Frees))

	// heap memory statistics
	perfMetrics.HeapAlloc.Set(float64(rtm.HeapAlloc))
	perfMetrics.HeapSys.Set(float64(rtm.HeapSys))
	perfMetrics.HeapIdle.Set(float64(rtm.HeapIdle))
	perfMetrics.HeapInUse.Set(float64(rtm.HeapInuse))
	perfMetrics.HeapReleased.Set(float64(rtm.HeapReleased))
	perfMetrics.HeapObjects.Set(float64(rtm.HeapObjects))

	// stack memory statistics
	perfMetrics.StackInUse.Set(float64(rtm.StackInuse))
	perfMetrics.StackSys.Set(float64(rtm.StackSys))

	// mspan memory statistics
	perfMetrics.MSpanInUse.Set(float64(rtm.MSpanInuse))
	perfMetrics.MSpanSys.Set(float64(rtm.MSpanSys))

	// mcache memory statistics
	perfMetrics.MCacheInUse.Set(float64(rtm.MCacheInuse))
	perfMetrics.MCacheSys.Set(float64(rtm.MCacheSys))

	// buckhash memory statistics
	perfMetrics.BuckHashSys.Set(float64(rtm.BuckHashSys))

	// gc memory statistics
	perfMetrics.GCSys.Set(float64(rtm.GCSys))

	// other memory statistics
	perfMetrics.OtherSys.Set(float64(rtm.OtherSys))

	// gc statistics
	perfMetrics.NextGC.Set(float64(rtm.NextGC))
	perfMetrics.PauseTotalNs.Set(float64(rtm.PauseTotalNs))
	perfMetrics.NumGC.Set(float64(rtm.NumGC))
	perfMetrics.NumForcedGC.Set(float64(rtm.NumForcedGC))
}
