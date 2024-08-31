package action

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
)

var RetriableErrorCodes = []string{
	string(adapter.DeviceUnreachable),
	string(adapter.DeviceBusy),
	string(adapter.InternalError),
}

const (
	ExponentialRetryPolicyType     RetryPolicyType = "EXPONENTIAL"
	UniformRetryPolicyType         RetryPolicyType = "UNIFORM"
	UniformParallelRetryPolicyType RetryPolicyType = "UNIFORM_PARALLEL"
	ProgressionRetryPolicyType     RetryPolicyType = "PROGRESSION"
)

var KnownRetryPolicyTypes = []string{
	string(ExponentialRetryPolicyType),
	string(UniformRetryPolicyType),
	string(UniformParallelRetryPolicyType),
	string(ProgressionRetryPolicyType),
}
