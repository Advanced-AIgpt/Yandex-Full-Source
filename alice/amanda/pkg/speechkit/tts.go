package speechkit

type TTSSpeak struct {
	Format                     string `json:"format"`
	EnableBargin               bool   `json:"enable_bargin"`
	LazyTtsStreaming           bool   `json:"lazy_tts_streaming"`
	FromCache                  bool   `json:"from_cache"`
	DisableInterruptionSpotter bool   `json:"disableInterruptionSpotter"`
}

type TTSResponse struct {
	TTSSpeak
	Data []byte `json:"-"`
}
