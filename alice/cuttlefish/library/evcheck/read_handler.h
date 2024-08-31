#pragma once
#include <contrib/libs/rapidjson/include/rapidjson/reader.h>
#include <util/generic/stack.h>
#include "node.h"


namespace NVoice {

class TJsonReadHandler : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, TJsonReadHandler> {
public:
    TJsonReadHandler(const TNode& rootNode)
        : RootNode(rootNode)
        , ExpectedNode(&RootNode)
    { }

    void Reset() {
        ExpectedNode = &RootNode;
        while (!NodeStack.empty())
            NodeStack.pop();
    }

    bool Null() {
        return OnSimpleValue(NODE_NULL);
    }
    bool Bool(bool) {
        return OnSimpleValue(NODE_BOOLEAN);
    }
    bool Int(int) {
        return OnSimpleValue(NODE_INTEGER);
    }
    bool Int64(int64_t) {
        return OnSimpleValue(NODE_INTEGER);
    }
    bool Uint(unsigned) {
        return OnSimpleValue(NODE_INTEGER);
    }
    bool Uint64(uint64_t) {
        return OnSimpleValue(NODE_INTEGER);
    }
    bool Double(double) {
        return OnSimpleValue(NODE_FLOAT);
    }
    bool String(const char*, rapidjson::SizeType, bool) {
        return OnSimpleValue(NODE_STRING);
    }
    bool StartObject() {
        return OnContainerStart<NODE_MAP>();
    }
    bool EndObject(rapidjson::SizeType) {
        return OnContainerEnd();
    }
    bool StartArray() {
        return OnContainerStart<NODE_ARRAY>();
    }
    bool EndArray(rapidjson::SizeType) {
        return OnContainerEnd();
    }

    bool Key(const char* key, rapidjson::SizeType len, bool) {
        const TNode::ContainerType* expectedMap = NodeStack.top()->SubMap.Get();
        if (expectedMap) {
            auto it = expectedMap->find(TStringBuf(key, len));
            if (it == expectedMap->end())
                return false;
            ExpectedNode = &it->second;
        }
        return true;
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
class TJsonReadHandlerWithCallback : public TJsonReadHandler {
public:
    TJsonReadHandlerWithCallback(const TNode& rootNode, TCallback callback)
        : TJsonReadHandler(rootNode)
        , Callback(callback)
    { }

    inline bool Null() {
        return TJsonReadHandler::Null() && CallReceiver(nullptr);
    }
    inline bool Bool(bool x) {
        return TJsonReadHandler::Bool(x) && CallReceiver(x);
    }
    inline bool Int(int x) {
        return TJsonReadHandler::Int(x) && CallReceiver(x);
    }
    inline bool Int64(int64_t x) {
        return TJsonReadHandler::Int64(x) && CallReceiver(x);
    }
    inline bool Uint(unsigned x) {
        return TJsonReadHandler::Uint(x) && CallReceiver(x);
    }
    inline bool Uint64(uint64_t x) {
        return TJsonReadHandler::Uint64(x) && CallReceiver(x);
    }
    inline bool Double(double x) {
        return TJsonReadHandler::Double(x) && CallReceiver(x);
    }
    inline bool String(const char* str, rapidjson::SizeType len, bool copy) {
        return TJsonReadHandler::String(str, len, copy) && CallReceiver(TStringBuf(str, len));
    }

private:
    template <typename T>
    inline bool CallReceiver(T&& value) {
        return TJsonReadHandler::GetCurrentNode() == nullptr || Callback(*TJsonReadHandler::GetCurrentNode(), std::forward<T>(value));
    }

    TCallback Callback;
};

} // namespace NVoice
