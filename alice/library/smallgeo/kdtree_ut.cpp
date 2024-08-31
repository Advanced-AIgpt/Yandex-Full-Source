#include "kdtree.h"

#include <library/cpp/testing/unittest/registar.h>

#include <cmath>

using namespace NAlice::NSmallGeo;

class T2DTreeTest : public I2DTreePoint, public TThrRefBase {
public:
    T2DTreeTest(double x, double y)
        : X(x)
        , Y(y)
    {
    }

    double GetX() const override {
        return X;
    }

    double GetY() const override {
        return Y;
    }

private:
    double X;
    double Y;
};

template <>
struct T2DTreeMetric<T2DTreeTest> {
    double Distance(double x1, double y1, double x2, double y2) {
        return hypot(x1 - x2, y1 - y2);
    }
};

std::pair<double, TIntrusivePtr<T2DTreeTest>> MinDistance(TVector<T2DTreeTest> points, T2DTreeTest query) {
    double answerDistance = std::numeric_limits<double>::infinity();
    TIntrusivePtr<T2DTreeTest> answerPtr;
    for (auto& point: points) {
        double distance = T2DTreeMetric<T2DTreeTest>{}.Distance(point.GetX(), point.GetY(), query.GetX(), query.GetY());
        if (answerDistance > distance) {
            answerDistance = distance;
            answerPtr = MakeIntrusive<T2DTreeTest>(point);
        }
    }
    return {answerDistance, answerPtr};
}

bool Equal(double x, double y) {
    return abs(x - y) < 1e-9;
}

Y_UNIT_TEST_SUITE(T2DTreeTest) {
    Y_UNIT_TEST(EmptyTreeTest) {
        TVector<T2DTreeTest> elements;
        T2DTree<T2DTreeTest> tree(elements);

        UNIT_ASSERT(!tree.GetNearest(0, 0));
    }

    Y_UNIT_TEST(SmallSquareTest) {
        const double len = 100;
        TVector<T2DTreeTest> elements({{0, 0}, {0, len}, {len, len}, {len, 0}});
        // Building tree shuffles vector, so sequence of points changes.
        T2DTree<T2DTreeTest> tree(elements);

        TVector<T2DTreeTest> testPoints({{0, 0}, {2 * len, 2 * len}, {len / 3, 2 * len}, {len / 2, len / 2}});
        for (const auto& point: testPoints) {
            auto answer = tree.GetNearest(point.GetX(), point.GetY());
            auto minDistance = MinDistance(elements, point);
            if (point.GetX() != len / 2 || point.GetY() != len / 2) {
                UNIT_ASSERT_EQUAL(answer->GetX(), minDistance.second->GetX());
                UNIT_ASSERT_EQUAL(answer->GetY(), minDistance.second->GetY());
            }
            auto nearest = tree.GetNearest(point.GetX(), point.GetY());
            UNIT_ASSERT(Equal(T2DTreeMetric<T2DTreeTest>{}.Distance(nearest->GetX(),
                nearest->GetY(), point.GetX(), point.GetY()), minDistance.first));
        }

        auto nearest = tree.GetNearest(len, 2 * len);
        UNIT_ASSERT(Equal(len, T2DTreeMetric<T2DTreeTest>{}.Distance(nearest->GetX(), nearest->GetY(), len, 2 * len)));
        nearest = tree.GetNearest(0, 0);
        UNIT_ASSERT(Equal(0, T2DTreeMetric<T2DTreeTest>{}.Distance(nearest->GetX(), nearest->GetY(), 0, 0)));
        nearest = tree.GetNearest(len / 2, len / 2);
        UNIT_ASSERT(Equal(len / sqrt(2), T2DTreeMetric<T2DTreeTest>{}.Distance(nearest->GetX(), nearest->GetY(), len / 2, len / 2)));
    }

    Y_UNIT_TEST(SamePointsTest) {
        double len = 1000;
        TVector<T2DTreeTest> elements;
        for (int i = 0; i < 10; ++i)
            elements.emplace_back(T2DTreeTest(0, 0));
        for (int i = 0; i < 10; ++i)
            elements.emplace_back(T2DTreeTest(0, len));
        T2DTree<T2DTreeTest> tree(elements);

        TVector<T2DTreeTest> testPoints({{0, 0}, {0, len}, {0, len / 2 - 0.01}, {len / 2, len / 2}});
        for (auto& point: testPoints) {
            auto nearest = tree.GetNearest(point.GetX(), point.GetY());
            UNIT_ASSERT(Equal(T2DTreeMetric<T2DTreeTest>{}.Distance(nearest->GetX(), nearest->GetY(),
                point.GetX(), point.GetY()), MinDistance(elements, point).first));
        }
    }
}
