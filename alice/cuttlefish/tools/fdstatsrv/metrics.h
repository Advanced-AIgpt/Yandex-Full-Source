#pragma once

#include <library/cpp/unistat/unistat.h>
#include <library/cpp/unistat/raii.h>


class IMetric {
public:
    virtual ~IMetric() = default;

    virtual void Push(int64_t value) = 0;
};

using IMetricPtr = THolder<IMetric>;


class TComplexMetric : public IMetric {
public:
    TComplexMetric(const TString& sensor) {
        RateSensor_ = TUnistat::Instance().DrillFloatHole(sensor + "_diff", "", "summ", NUnistat::TPriority(0));
        AbsMaxSensor_ = TUnistat::Instance().DrillFloatHole(sensor + "_abs", "", "txxx", NUnistat::TPriority(0), NUnistat::TStartValue(0), EAggregationType::LastValue);
        AbsSumSensor_ = TUnistat::Instance().DrillFloatHole(sensor + "_sum", "", "tmmv", NUnistat::TPriority(0), NUnistat::TStartValue(0), EAggregationType::LastValue);
    }

    virtual void Push(int64_t value) override {
        RateSensor_->PushSignal(value - LastValue_);
        AbsMaxSensor_->PushSignal(value);
        AbsSumSensor_->PushSignal(value);
        LastValue_ = value;
    }

private:
    NUnistat::IHolePtr  RateSensor_;
    NUnistat::IHolePtr  AbsMaxSensor_;
    NUnistat::IHolePtr  AbsSumSensor_;
    int64_t LastValue_ { 0 };
};


class TAbsMetric : public IMetric {
public:
    TAbsMetric(const TString& sensor) {
        AbsMaxSensor_ = TUnistat::Instance().DrillFloatHole(sensor + "_abs", "", "txxx", NUnistat::TPriority(0), NUnistat::TStartValue(0), EAggregationType::LastValue);
        AbsSumSensor_ = TUnistat::Instance().DrillFloatHole(sensor + "_sum", "", "tmmv", NUnistat::TPriority(0), NUnistat::TStartValue(0), EAggregationType::LastValue);
    }

    void Push(int64_t value) {
        AbsMaxSensor_->PushSignal(value);
        AbsSumSensor_->PushSignal(value);
    }

private:
    NUnistat::IHolePtr  AbsMaxSensor_;
    NUnistat::IHolePtr  AbsSumSensor_;
};


class TMetrics {
public:
    TMetrics() {
        FhAllocated_ = MakeHolder<TComplexMetric>("file_handlers_allocated");
        FhUnused_ = MakeHolder<TComplexMetric>("file_handlers_unused");
        FhMax_ = MakeHolder<TAbsMetric>("file_handlers_max");

        NsTotalUsed_ = MakeHolder<TComplexMetric>("sock_total_used");
        NsTcpInUse_ = MakeHolder<TComplexMetric>("sock_tcp_in_use");
        NsTcpOrphan_ = MakeHolder<TComplexMetric>("sock_tcp_orhaned");
        NsTcpTimeWait_ = MakeHolder<TComplexMetric>("sock_tcp_time_wait");
        NsTcpAllocated_ = MakeHolder<TComplexMetric>("sock_tcp_allocated");
        NsTcpKernelPages_ = MakeHolder<TAbsMetric>("sock_tcp_kernel_pages");

        FdMax_ = MakeHolder<TAbsMetric>("fd_limit");
    }

    //  from /proc/sys/fs/file-nr
    void SetFileHandlersAllocated(uint64_t value) {
        FhAllocated_->Push(value);
    }

    void SetFileHandlersUnused(uint64_t value) {
        FhUnused_->Push(value);
    }

    void SetFileHandlersMax(uint64_t value) {
        FhMax_->Push(value);
    }

    //  from /proc/net/sockstat
    void SetSocketsTotal(uint64_t value) {
        NsTotalUsed_->Push(value);
    }
    void SetTcpSocketsAllocated(uint64_t value) {
        NsTcpAllocated_->Push(value);
    }

    void SetTcpSocketsInUse(uint64_t value) {
        NsTcpInUse_->Push(value);
    }

    void SetTcpSocketsOrphan(uint64_t value) {
        NsTcpOrphan_->Push(value);
    }

    void SetTcpSocketsTimeWait(uint64_t value) {
        NsTcpTimeWait_->Push(value);
    }

    void SetTcpSocketsKernelPages(uint64_t value) {
        NsTcpKernelPages_->Push(value);
    }

    //  from /ulimit
    void SetFileDescriptorsMax(uint64_t value) {
        FdMax_->Push(value);
    }

    TString DumpJson() const {
        return TUnistat::Instance().CreateJsonDump(0);
    }

private:
    IMetricPtr FhAllocated_;
    IMetricPtr FhUnused_;
    IMetricPtr FhMax_;

    IMetricPtr NsTotalUsed_;
    IMetricPtr NsTcpInUse_;
    IMetricPtr NsTcpOrphan_;
    IMetricPtr NsTcpTimeWait_;
    IMetricPtr NsTcpAllocated_;
    IMetricPtr NsTcpKernelPages_;

    IMetricPtr FdMax_;
};  // class TMetrics
