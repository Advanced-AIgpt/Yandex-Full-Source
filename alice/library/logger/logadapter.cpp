#include "logadapter.h"

using namespace NAlice;

TLogAdapterElement TLogAdapter::Log(const TSourceLocation& location, ELogAdapterType type) const {
    return TLogAdapterElement{*this, location, type};
}
