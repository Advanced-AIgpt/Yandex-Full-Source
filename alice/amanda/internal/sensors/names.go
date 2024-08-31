package sensors

const (
	AppPrefix     = "app."
	WebhookPrefix = "webhook."
	ServicePrefix = "service."
	ProductPrefix = "product."

	MonthlyActiveUsers = "monthly_active_users"
	WeeklyActiveUsers  = "weekly_active_users"
	DailyActiveUsers   = "daily_active_users"
	TotalUsers         = "total_users"

	ServiceName = "service_name"
	RequestType = "type"

	RequestsCountPerSecond = "requests_" + CountPerSecondPostfix
	ErrorsCountPerSecond   = "errors_" + CountPerSecondPostfix

	SizeBytesPostfix      = "size_bytes"
	TimeSecondsPostfix    = "time_seconds"
	CountPerSecondPostfix = "count_per_second"

	RequestSizeBytes    = "request_" + SizeBytesPostfix
	ResponseTimeSeconds = "response_" + TimeSecondsPostfix

	_sessionLoad = "session_load_"
	_sessionSave = "session_save_"

	SessionLoadSizeBytes            = _sessionLoad + SizeBytesPostfix
	SessionLoadTimeSeconds          = _sessionLoad + TimeSecondsPostfix
	SessionLoadErrorsCountPerSecond = _sessionLoad + ErrorsCountPerSecond
	SessionSaveSizeBytes            = _sessionSave + SizeBytesPostfix
	SessionSaveTimeSeconds          = _sessionSave + TimeSecondsPostfix
	SessionSaveErrorsCountPerSecond = _sessionSave + ErrorsCountPerSecond
)
