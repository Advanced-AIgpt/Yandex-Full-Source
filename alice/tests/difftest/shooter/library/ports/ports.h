#pragma once

#include <util/generic/hash_set.h>
#include <util/generic/map.h>
#include <util/system/rwlock.h>
#include <util/system/tls.h>

namespace NAlice::NShooter {

class IPorts {
public:
    virtual ~IPorts() = default;

    /** Register a port for an app
    */
    virtual ui16 Add(TStringBuf appName) = 0;

    /** Obtain the port of the app
    */
    virtual ui16 Get(TStringBuf appName) const = 0;

    /** Check whether the app is registered
    */
    virtual bool Has(TStringBuf appName) const = 0;
};

class TPorts : public IPorts {
public:
    ui16 Add(TStringBuf appName) override;
    ui16 Get(TStringBuf appName) const override;
    bool Has(TStringBuf appName) const override;

private:
    ui16 FreePort();

private:
    THashMap<TStringBuf, ui16> Map_;
    static THashSet<ui16> Ports_;
    static TRWMutex Lock_;
};

} // namespace NAlice::NShooter
