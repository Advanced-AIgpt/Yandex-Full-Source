package slices

// DedupUint64sKeepOrder removes duplicate values from uint64 slice.
// It keeps input order and removes last duplicates
func DedupUint64sKeepOrder(a []uint64) []uint64 {
	result := make([]uint64, 0, len(a))
	dict := make(map[uint64]bool)
	for _, val := range a {
		if _, ok := dict[val]; !ok {
			dict[val] = true
			result = append(result, val)
		}
	}
	return result
}
