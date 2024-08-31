#pragma once
#include <library/cpp/json/fast_sax/parser.h>
#include <util/generic/stack.h>
#include "node.h"


namespace NVoice {

class TReadJsonFastCallbacks : public NJson::TJsonCallbacks {
public:
    TReadJsonFastCallbacks(const TNode& rootNode)
        : RootNode(rootNode)
        , ExpectedNode(&RootNode)
    { }

    void Reset() {
        ExpectedNode = &RootNode;
        while (!NodeStack.empty())
            NodeStack.pop();
    }

    bool OnNull() override {
        return OnSimpleValue(NODE_NULL);
    }
    bool OnBoolean(bool) override {
        return OnSimpleValue(NODE_BOOLEAN);
    }
    bool OnInteger(long long) override {
        return OnSimpleValue(NODE_INTEGER);
    }
    bool OnUInteger(unsigned long long) override {
        return OnSimpleValue(NODE_INTEGER);
    }
    bool OnDouble(double) override {
        return OnSimpleValue(NODE_FLOAT);
    }
    bool OnString(const TStringBuf&) override {
        return OnSimpleValue(NODE_STRING);
    }
    bool OnStringNoCopy(const TStringBuf& x) override {
        return OnString(x);
    }
    bool OnOpenMap() override {
        return OnContainerStart<NODE_MAP>();
    }
    bool OnCloseMap() override {
        return OnContainerEnd();
    }
    bool OnOpenArray() override {
        return OnContainerStart<NODE_ARRAY>();
    }
    bool OnCloseArray() override {
        return OnContainerEnd();
    }
    bool OnMapKey(const TStringBuf& key) override {
        const TNode::ContainerType* expectedMap = NodeStack.top()->SubMap.Get();
        if (expectedMap) {
            auto it = expectedMap->find(key);
            if (it == expectedMap->end())
                return false;
            ExpectedNode = &it->second;
        }
        return true;
    }
    bool OnMapKeyNoCopy(const TStringBuf& x) override {
        return OnMapKey(x);
    }

protected:
    inline const TNode* GetCurrentNode() const {
        return ExpectedNode;
    }

private:
    inline bool OnSimpleValue(ENodeType valueType) {
        return (ExpectedNode == nullptr) || (valueType & ExpectedNode->TypeMask);
    }

    template <ENodeType ValueType>
    inline bool OnContainerStart() {
        static_assert(ValueType == NODE_MAP || ValueType == NODE_ARRAY);
        static const TNode ANYTHING{ValueType};

        if (ExpectedNode != nullptr) {
            if (!(ValueType & ExpectedNode->TypeMask))
                return false;
            NodeStack.push(ExpectedNode);
        } else {
            NodeStack.push(&ANYTHING);
        }
        ExpectedNode = nullptr;
        return true;
    }

    inline bool OnContainerEnd() {
        NodeStack.pop();
        return true;
    }

    const TNode& RootNode;
    TStack<const TNode*> NodeStack;
    const TNode* ExpectedNode = nullptr;
};


template <class TCallback>
class TReadJsonFastCallbacksWithCallback : public TReadJsonFastCallbacks {
public:
    TReadJsonFastCallbacksWithCallback(const TNode& rootNode, TCallback callback)
        : TReadJsonFastCallbacks(rootNode)
        , Callback(callback)
    { }

    inline bool OnNull() override {
        return TReadJsonFastCallbacks::OnNull() && CallReceiver(nullptr);
    }
    inline bool OnBoolean(bool x) override {
        return TReadJsonFastCallbacks::OnBoolean(x) && CallReceiver(x);
    }
    inline bool OnInteger(long long x) override {
        return TReadJsonFastCallbacks::OnInteger(x) && CallReceiver(x);
    }
    inline bool OnUInteger(unsigned long long x) override {
        return TReadJsonFastCallbacks::OnUInteger(x) && CallReceiver(x);
    }
    inline bool OnDouble(double x) override {
        return TReadJsonFastCallbacks::OnDouble(x) && CallReceiver(x);
    }
    inline bool OnString(const TStringBuf& x) override {
        return TReadJsonFastCallbacks::OnString(x) && CallReceiver(x);
    }

private:
    template <typename T>
    inline bool CallReceiver(T&& value) {
        return TReadJsonFastCallbacks::GetCurrentNode() == nullptr
            || Callback(*TReadJsonFastCallbacks::GetCurrentNode(), std::forward<T>(value));
    }

    TCallback Callback;
};

} // namespace NVoice
