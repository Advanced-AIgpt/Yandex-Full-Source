#pragma once

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

#include <cmath>

namespace NBASS {
namespace NSmallGeo {

template <typename T>
struct T2DTreeMetric {
    double Distance(double x1, double y1, double x2, double y2);
};

class I2DTreePoint {
public:
    virtual ~I2DTreePoint() = default;

    virtual double GetX() const = 0;
    virtual double GetY() const = 0;
};

/*
 * T2DTree works with classes which implement I2DTreePoint.
 * Also T2DTreeMetric<T> should be implemented.
 */
template <typename T>
class T2DTree {
public:
    T2DTree(TVector<T>& regions) {
        Root = Build(regions.begin(), regions.end(), ECmp::ByY);
    }
    T2DTree() {
        TVector<T> regions;
        Root = Build(regions.begin(), regions.end(), ECmp::ByY);
    }

private:
    const double INF = std::numeric_limits<double>::infinity();

    enum class ECmp {
        ByX,
        ByY
    };

    struct TPoint {
        TPoint()
            : X(0)
            , Y(0)
        {
        }

        TPoint(double x, double y)
            : X(x)
            , Y(y)
        {
        }

        static TPoint Min(TPoint a, TPoint b) {
            return {std::min(a.X, b.X), std::min(a.Y, b.Y)};
        }

        static TPoint Max(TPoint a, TPoint b) {
            return {std::max(a.X, b.X), std::max(a.Y, b.Y)};
        }

        double DistanceTo(TPoint point) {
            return T2DTreeMetric<T>{}.Distance(X, Y, point.X, point.Y);
        }

        double X;
        double Y;
    };

    struct TBoundingBox {
        TBoundingBox(TBoundingBox first, TBoundingBox second) {
            DownLeft = TPoint::Min(first.DownLeft, second.DownLeft);
            UpRight = TPoint::Max(first.UpRight, second.UpRight);
        }
        explicit TBoundingBox(TPoint coordinates)
            : DownLeft(coordinates)
            , UpRight(coordinates)
        {
        }

        void Update(TPoint coordinates) {
            DownLeft = TPoint::Min(DownLeft, coordinates);
            UpRight = TPoint::Max(UpRight, coordinates);
        }

        void Update(TBoundingBox boundingBox) {
            DownLeft = TPoint::Min(DownLeft, boundingBox.DownLeft);
            UpRight = TPoint::Max(UpRight, boundingBox.UpRight);
        }

        TPoint DownLeft;
        TPoint UpRight;
    };

    struct TNode : public TThrRefBase {
        TNode() = default;
        TNode(TIntrusivePtr<T> region, TBoundingBox boundingBox)
            : Region(region)
            , BoundingBox(boundingBox)
        {
        }
        TIntrusivePtr<T> Region;
        TBoundingBox BoundingBox;
        TIntrusivePtr<TNode> Left;
        TIntrusivePtr<TNode> Right;
    };

public:
    TIntrusivePtr<T> GetNearest(double x, double y) const {
        return GetNearest(TPoint(x, y), Root, INF).second;
    }

private:
    static bool LessY(const T& A, const T& B) {
        return A.GetY() < B.GetY();
    }

    static bool LessX(const T& A, const T& B) {
        return A.GetX() < B.GetX();
    }

    ECmp GetNext(ECmp curr) {
        return curr == ECmp::ByX ? ECmp::ByY : ECmp::ByX;
    }

    bool (*GetLess(ECmp curr))(const T&, const T&) {
        return curr == ECmp::ByX ? LessX : LessY;
    }

    TIntrusivePtr<TNode> Build(typename TVector<T>::iterator begin, typename TVector<T>::iterator end, ECmp cmp) {
        if (begin == end) {
            return {};
        }
        auto mid = (end - begin) / 2 + begin;
        TIntrusivePtr<TNode> node;

        std::nth_element(begin, mid, end, GetLess(cmp));
        node = MakeIntrusive<TNode>(MakeIntrusive<T>(*mid), TBoundingBox(TPoint(mid->GetX(), mid->GetY())));
        node->Left = Build(begin, mid, GetNext(cmp));
        node->Right = Build(mid + 1, end, GetNext(cmp));

        if (node->Left) {
            node->BoundingBox.Update(node->Left->BoundingBox);
        }
        if (node->Right) {
            node->BoundingBox.Update(node->Right->BoundingBox);
        }

        return node;
    }

    double LowerBound(TBoundingBox boundingBox, TPoint position) const {
        bool xBetween = boundingBox.DownLeft.X <= position.X && position.X <= boundingBox.UpRight.X;
        bool yBetween = boundingBox.DownLeft.Y <= position.Y && position.Y <= boundingBox.UpRight.Y;
        if (xBetween && yBetween) {
            return 0;
        }
        if (xBetween) {
            return Min(position.DistanceTo({position.X, boundingBox.DownLeft.Y}),
                       position.DistanceTo({position.X, boundingBox.UpRight.Y}));
        }
        if (yBetween) {
            return Min(position.DistanceTo({boundingBox.DownLeft.X, position.Y}),
                       position.DistanceTo({boundingBox.UpRight.X, position.Y}));
        }
        double minDistance = Min(position.DistanceTo({boundingBox.UpRight.X, boundingBox.UpRight.Y}),
                                 position.DistanceTo({boundingBox.DownLeft.X, boundingBox.DownLeft.Y}));
        minDistance = Min(minDistance, position.DistanceTo({boundingBox.DownLeft.X, boundingBox.UpRight.Y}));
        minDistance = Min(minDistance, position.DistanceTo({boundingBox.UpRight.X, boundingBox.DownLeft.Y}));
        return minDistance;
    }

    std::pair<double, TIntrusivePtr<T>> GetNearest(TPoint position, TIntrusivePtr<TNode> root, double bestDistance) const {
        if (!root) {
            return {INF, nullptr};
        }
        auto lowerBound = LowerBound(root->BoundingBox, position);
        if (lowerBound >= bestDistance) {
            return {INF, nullptr};
        }

        std::pair<double, TIntrusivePtr<T>> answer = {position.DistanceTo({root->Region->GetX(), root->Region->GetY()}), root->Region};

        double distToLeftBoundingBox = INF;
        if (root->Left)
            distToLeftBoundingBox = LowerBound(root->Left->BoundingBox, position);
        double distToRightBoundingBox = INF;
        if (root->Right)
            distToRightBoundingBox = LowerBound(root->Right->BoundingBox, position);

        auto firstChild = root->Left;
        auto secondChild = root->Right;
        if (distToLeftBoundingBox > distToRightBoundingBox) {
            std::swap(firstChild, secondChild);
        }

        if (Min(distToLeftBoundingBox, distToRightBoundingBox) > Min(bestDistance, answer.first)) {
            return answer;
        }

        auto answerFirst = GetNearest(position, firstChild, Min(bestDistance, answer.first));
        if (answerFirst.second && answerFirst.first < answer.first) {
            answer = answerFirst;
        }

        if (Max(distToLeftBoundingBox, distToRightBoundingBox) > Min(bestDistance, answer.first)) {
            return answer;
        }

        auto answerSecond = GetNearest(position, secondChild, Min(bestDistance, answer.first));
        if (answerSecond.second && answerSecond.first < answer.first) {
            answer = answerSecond;
        }

        return answer;
    }

private:
    TIntrusivePtr<TNode> Root;
};

} // namespace NSmallGeo
} // namespace NBASS
