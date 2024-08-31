package tools

func CopyBytes(input []byte) []byte {
	if input == nil {
		return nil
	}
	output := make([]byte, len(input))
	copy(input, output)
	return output
}
