#include "context.h"

#include <contrib/libs/hiredis/hiredis.h>


namespace NRedis {

TReply::TReply(void* ptr, bool release)
    : reply(reinterpret_cast<redisReply*>(ptr))
    , release(release)
{ }


TReply::~TReply() {
    if (reply && release) {
        freeReplyObject(reply);
    }
}


bool TReply::IsValid() const {
    return reply && reply->type != REDIS_REPLY_ERROR;
}


bool TReply::IsStatus() const {
    return reply && reply->type == REDIS_REPLY_STATUS;
}


bool TReply::IsError() const {
    return reply && reply->type == REDIS_REPLY_ERROR;
}


bool TReply::IsString() const {
    return reply && reply->type == REDIS_REPLY_STRING;
}


bool TReply::IsNil() const {
    return reply && reply->type == REDIS_REPLY_NIL;
}


bool TReply::IsArray() const {
    return reply && reply->type == REDIS_REPLY_ARRAY;
}


TString TReply::GetData() const {
    return reply
        ? TString(reply->str, reply->len)
        : TString()
    ;
}


TReply TReply::GetValueAt(size_t index) const {
    return reply && IsArray() && index < reply->elements
        ? TReply(reply->element[index], false)
        : TReply(nullptr, false)
    ;
}


size_t TReply::GetArraySize() const {
    return reply && IsArray()
        ? reply->elements
        : 0
    ;
}



TContext::TContext(TContext&& other)
    : context(other.context)
{ }


TContext::TContext(const TString& ip, uint16_t port)
    : context(redisConnect(ip.c_str(), port))
{ }


TContext::TContext(const TString& ip, uint16_t port, TInstant deadline)
    : context(redisConnectWithTimeout(ip.c_str(), port, deadline.TimeVal()))
{ }


TContext::~TContext() {
    if (context) {
        redisFree(context);
    }
}


bool TContext::IsValid() const {
    return context && !context->err;
}


TReply TContext::Execute(const char *fmt, ...) {
    if (!context) {
        return TReply(nullptr, false);
    }

    redisReply *r = nullptr;

    va_list args;
    va_start(args, fmt);
    r = reinterpret_cast<redisReply*>(redisvCommand(context, fmt, args));
    va_end(args);

    return TReply(r);
}


}   // namespace NRedis
