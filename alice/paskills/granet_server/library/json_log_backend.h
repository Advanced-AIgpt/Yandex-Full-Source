#include <library/cpp/logger/backend.h>


class TJsonLogBackend : public TLogBackend {
public:

    TJsonLogBackend();

    ~TJsonLogBackend() override = default;

    void WriteData(const TLogRecord& rec) override;

    void ReopenLog() override;

};
