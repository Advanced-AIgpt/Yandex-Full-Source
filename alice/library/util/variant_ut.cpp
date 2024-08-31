#include "variant.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/variant.h>

using namespace NAlice;

namespace {

class T1 {};
class T2 {};
class T3 {};
using TTestVariant = std::variant<T1, T2, T3>;

Y_UNIT_TEST_SUITE(Variant) {
    Y_UNIT_TEST(VisitNonVoid) {
        enum class ERet { First, Second, Third };
        auto visitor = MakeLambdaVisitor([](const T1&) { return ERet::First; }, [](const T2&) { return ERet::Second; },
                                         [](const T3&) { return ERet::Third; });
        {
            TTestVariant value{T1{}};
            UNIT_ASSERT(std::visit(visitor, value) == ERet::First);
        }
        {
            TTestVariant value{T2{}};
            UNIT_ASSERT(std::visit(visitor, value) == ERet::Second);
        }
        {
            TTestVariant value{T3{}};
            UNIT_ASSERT(std::visit(visitor, value) == ERet::Third);
        }
    }
    Y_UNIT_TEST(VisitVoid) {
        enum class ERet { First, Second, Third };
        ERet result;
        auto visitor = MakeLambdaVisitor([&result](const T1&) { result = ERet::First; },
                                         [&result](const T2&) { result = ERet::Second; },
                                         [&result](const T3&) { result = ERet::Third; });
        {
            TTestVariant value{T1{}};
            std::visit(visitor, value);
            UNIT_ASSERT(result == ERet::First);
        }
        {
            TTestVariant value{T2{}};
            std::visit(visitor, value);
            UNIT_ASSERT(result == ERet::Second);
        }
        {
            TTestVariant value{T3{}};
            std::visit(visitor, value);
            UNIT_ASSERT(result == ERet::Third);
        }
    }
}

} // namespace
