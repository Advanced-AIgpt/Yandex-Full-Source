#pragma once

#include <util/generic/ptr.h>
#include <util/datetime/base.h>


struct redisContext;
struct redisReply;


namespace NRedis {


class TReply {
public:
    TReply();

    TReply(void*, bool release=true);

    TReply(TReply&& other)
        : reply(other.reply)
        , release(other.release)
    {
        other.release = false;
        other.reply = nullptr;
    }

    ~TReply();

    inline TReply& operator=(TReply&& p) {
        reply = p.reply;
        release = p.release;
        p.reply = nullptr;
        p.release = false;
        return *this;
    }

    bool IsValid() const;

    inline operator bool() const {
        return IsValid();
    }

    bool IsStatus() const;

    bool IsError() const;

    bool IsString() const;

    bool IsNil() const;

    bool IsArray() const;

    TString GetData() const;

    TReply GetValueAt(size_t size) const;

    size_t GetArraySize() const;

private:
    redisReply *reply { nullptr };
    bool release { true };
};


class TContext : public TThrRefBase {
public:
    TContext() = delete;

    TContext(const TContext&) = delete;

    TContext(TContext&& other);

    TContext(const TString& ip, uint16_t port);

    TContext(const TString& ip, uint16_t port, TInstant deadline);

    ~TContext();

    bool IsValid() const;

    inline operator bool() const {
        return IsValid();
    }

    TReply Execute(const char *fmt, ...);

protected:

private:
    redisContext *context { nullptr };
};


using TContextPtr = TIntrusivePtr<TContext>;


inline TContextPtr MakeContext(const TString& ip, uint16_t port) {
    return MakeIntrusive<TContext>(ip, port);
}


inline TContextPtr MakeContext(const TString& ip, uint16_t port, TInstant deadline) {
    return MakeIntrusive<TContext>(ip, port, deadline);
}

}   // namespace NRedis
