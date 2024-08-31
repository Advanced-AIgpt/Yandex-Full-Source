#pragma once

#include <alice/tests/difftest/shooter/library/core/fwd.h>
#include <alice/tests/difftest/shooter/library/core/status.h>

#include <util/generic/noncopyable.h>
#include <util/system/shellcommand.h>
#include <util/stream/file.h>

#include <library/cpp/neh/neh.h>
#include <library/cpp/uri/uri.h>

namespace NAlice::NShooter {

/**
 * Base interface for apps
 */
class IApp : public TThrRefBase {
public:
    virtual ~IApp() = default;

    /**
     * Name/alias of the app, used for distinguishing between ports
     */
    virtual TStringBuf Name() const = 0;

    /**
     * Init work (creating streams, files, etc)
     */
    virtual void Init() = 0;

    /**
     * Run the app
     */
    virtual TStatus Run() = 0;

    /**
     * App health check
     */
    virtual TStatus Ping() = 0;

    /**
     * Stop the app
     */
    virtual void Stop() = 0;
};

} // namespace NAlice::NShooter
