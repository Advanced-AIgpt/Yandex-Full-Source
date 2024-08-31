package tools

func ContainsInt(element int, slice []int) bool {
	for _, sliceElement := range slice {
		if sliceElement == element {
			return true
		}
	}

	return false
}

func ContainsAllInt(main, toBeContained []int) bool {
	if main == nil || toBeContained == nil {
		return false
	}

	mainMap := make(map[int]bool)
	for _, elem := range main {
		mainMap[elem] = true
	}

	for _, elem := range toBeContained {
		if _, found := mainMap[elem]; !found {
			return false
		}
	}
	return true
}

//address of int
func AOI(i int) *int {
	return &i
}
func AOI8(i int8) *int8 {
	return &i
}
func AOI16(i int16) *int16 {
	return &i
}
func AOI32(i int32) *int32 {
	return &i
}
func AOI64(i int64) *int64 {
	return &i
}

func AOUI(i uint) *uint {
	return &i
}
func AOUI8(i uint8) *uint8 {
	return &i
}
func AOUI16(i uint16) *uint16 {
	return &i
}
func AOUI32(i uint32) *uint32 {
	return &i
}
func AOUI64(i uint64) *uint64 {
	return &i
}

func IntSliceMin(slice []int) int {
	min := slice[0]
	for _, x := range slice {
		if x < min {
			min = x
		}
	}

	return min
}

func IntSliceMax(slice []int) int {
	max := slice[0]
	for _, x := range slice {
		if x > max {
			max = x
		}
	}

	return max
}
