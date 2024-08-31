#include "item_adapter.h"

#include <alice/megamind/library/apphost_request/ut/test.pb.h>

#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/util/status.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/deque.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;

Y_UNIT_TEST_SUITE(AppHostItemAdapter) {
    constexpr TStringBuf ItemName{"test"};
    const TString TestString{"test_string"};
    TRTLogger& Logger{TRTLogger::NullLogger()};

    Y_UNIT_TEST_F(GetNonStreamingItems, TAppHostFixture) {
        auto ahCtx = CreateAppHostContext();
        auto& ctx = ahCtx.TestCtx();

        {
            TTestProto proto;
            proto.SetTestString(TestString);
            ctx.AddProtobufItem(proto, ItemName, NAppHost::EContextItemKind::Input);
        }

        TItemProxyAdapter& adapter = ahCtx.ItemProxyAdapter();
        {
            TTestProto dstProto;
            auto err = adapter.GetFromContext<TTestProto>(ItemName).MoveTo(dstProto);
            UNIT_ASSERT(!err.Defined());
            UNIT_ASSERT_VALUES_EQUAL(dstProto.GetTestString(), TestString);
        }

        {
            TTestProto dstProto;
            auto err = adapter.GetFromContext<TTestProto>("non_existent_item").MoveTo(dstProto);
            UNIT_ASSERT(err.Defined());
            UNIT_ASSERT(dstProto.GetTestString().empty());
        }
    }

    Y_UNIT_TEST_F(PutItemToContext, TAppHostFixture) {
        auto ahCtx = CreateAppHostContext();
        auto& ctx = ahCtx.TestCtx();

        TItemProxyAdapter& adapter = ahCtx.ItemProxyAdapter();

        {
            TTestProto proto;
            proto.SetTestString(TestString);
            adapter.PutIntoContext(proto, ItemName);
        }

        auto items = ctx.GetProtobufItemRefs(ItemName);
        UNIT_ASSERT(items.size() == 1);
        TTestProto proto;
        UNIT_ASSERT(items.front().Fill(&proto));
        UNIT_ASSERT_VALUES_EQUAL(proto.GetTestString(), TestString);
    }

    Y_UNIT_TEST_F(StreamingNotFoundSmoke, TAppHostFixture) {
        auto ahCtx = CreateAppHostContext();
        auto& ctx = ahCtx.TestCtx();

        {
            TTestProto proto;
            proto.SetTestString("0");
            ctx.AddProtobufItem(proto, ItemName, NAppHost::EContextItemKind::Input);

            proto.SetTestString("1");
            ctx.AddProtobufItem(proto, ItemName, NAppHost::EContextItemKind::Input);
        }

        TItemProxyAdapter& adapter = ahCtx.ItemProxyAdapter();

        {
            TTestProto proto;
            auto err = adapter.GetFromContext<TTestProto>("not_found_item").MoveTo(proto);
            UNIT_ASSERT(err.Defined());
            UNIT_ASSERT_VALUES_EQUAL(err->Type, TError::EType::NotFound);
        }

        {
            TTestProto proto;
            auto err = adapter.GetFromContext<TTestProto>(ItemName, 2).MoveTo(proto);
            UNIT_ASSERT(err.Defined());
            UNIT_ASSERT_VALUES_EQUAL(err->Type, TError::EType::Input);
        }

    }

    Y_UNIT_TEST_F(StreamingItems, TAppHostFixture) {
        static constexpr TStringBuf firstItem{"first_item"};
        static constexpr TStringBuf secondItem{"second_item"};
        static constexpr TStringBuf nonStreamingItem{"non_streaming_item"};

        auto ahCtx = CreateAppHostContext();

        auto& ctx = ahCtx.TestCtx();

        {
            TTestProto proto;
            proto.SetTestString(TestString);
            ctx.AddProtobufItem(proto, nonStreamingItem, NAppHost::EContextItemKind::Input);
        }

        {
            auto streamCtx = MakeHolder<NAppHost::NService::TTestContext>();
            TTestProto proto;
            proto.SetTestString(TestString);
            streamCtx->AddProtobufItem(proto, "useless_item1", NAppHost::EContextItemKind::Input);
            ctx.AddStreamInputChunk(std::move(streamCtx));
        }

        {
            auto streamCtx = MakeHolder<NAppHost::NService::TTestContext>();
            TTestProto proto;
            proto.SetTestString(TestString);
            streamCtx->AddProtobufItem(proto, firstItem, NAppHost::EContextItemKind::Input);
            ctx.AddStreamInputChunk(std::move(streamCtx));
        }

        {
            auto streamCtx = MakeHolder<NAppHost::NService::TTestContext>();
            TTestProto proto;
            proto.SetTestString(TestString);
            streamCtx->AddProtobufItem(proto, "useless_item2", NAppHost::EContextItemKind::Input);
            ctx.AddStreamInputChunk(std::move(streamCtx));
        }

        auto& itemProxyAdapter = ahCtx.ItemProxyAdapter();

        // The item is put into the context in the second call of onNextInput.
        // Cycle is needed to cache check. (2 is enough, but I want 3 there ;).
        for (size_t i = 0; i < 3; ++i) {
            TTestProto proto;
            auto err = itemProxyAdapter.GetFromContext<TTestProto>(firstItem).MoveTo(proto);
            UNIT_ASSERT_C(!err.Defined(), TStringBuilder{} << i << " iteration");
            UNIT_ASSERT_VALUES_EQUAL_C(proto.GetTestString(), TestString, TStringBuilder{} << i << " iteration");
        }

        { // Item not found (this item not in context).
            auto rval = itemProxyAdapter.GetFromContext<TTestProto>(secondItem);
            UNIT_ASSERT(rval.Status().Defined());
            UNIT_ASSERT_VALUES_EQUAL(rval.Status()->Type, TError::EType::NotFound);
        }

        { // Cache item test.
            TTestProto proto;
            auto err = itemProxyAdapter.GetFromContext<TTestProto>(nonStreamingItem).MoveTo(proto);
            UNIT_ASSERT(!err.Defined());
            UNIT_ASSERT_VALUES_EQUAL(proto.GetTestString(), TestString);

            // Check for that prev items are still there.
            UNIT_ASSERT(!itemProxyAdapter.GetFromContext<TTestProto>(firstItem).Status().Defined());
            UNIT_ASSERT(!itemProxyAdapter.GetFromContext<TTestProto>("useless_item1").Status().Defined());
            UNIT_ASSERT(!itemProxyAdapter.GetFromContext<TTestProto>("useless_item2").Status().Defined());
        }
    }

    Y_UNIT_TEST_F(CheckAndPutIntoContext, TAppHostFixture) {
        auto ahCtx = CreateAppHostContext();

        auto& ctx = ahCtx.TestCtx();
        auto& itemProxyAdapter = ahCtx.ItemProxyAdapter();
        {
            TTestProto proto;
            proto.SetTestString(TestString);
            itemProxyAdapter.CheckAndPutIntoContext(proto, ItemName);
        }

        {
            TTestProto proto;
            proto.SetTestString(TestString + "123");
            itemProxyAdapter.CheckAndPutIntoContext(proto, ItemName);
        }

        auto items = ctx.GetProtobufItemRefs(ItemName);
        UNIT_ASSERT(items.size() == 1);
        TTestProto proto;
        UNIT_ASSERT(items.front().Fill(&proto));
        UNIT_ASSERT_VALUES_EQUAL(proto.GetTestString(), TestString);
    }

    Y_UNIT_TEST_F(ForEachCached, TAppHostFixture) {
        auto ahCtx = CreateAppHostContext();

        TDeque<TString> strings{{"test1", "test2", "test3"}};
        for (const auto& str : strings) {
            TTestProto proto;
            proto.SetTestString(str);
            ahCtx.TestCtx().AddProtobufItem(proto, "test_item", NAppHost::EContextItemKind::Input);
        }

        auto& itemProxyAdapter = ahCtx.ItemProxyAdapter();
        auto cb = [&strings](const TTestProto& proto) mutable {
            UNIT_ASSERT(!strings.empty());
            UNIT_ASSERT_VALUES_EQUAL(proto.GetTestString(), strings.front());
            strings.pop_front();
        };
        itemProxyAdapter.ForEachCached<TTestProto>("test_item", cb);
    }

    Y_UNIT_TEST_F(GetFromContextCached, TAppHostFixture) {
        auto ahCtx = CreateAppHostContext();

        TDeque<TString> strings{{"test1", "test2", "test3"}};
        for (const auto& str : strings) {
            TTestProto proto;
            proto.SetTestString(str);
            ahCtx.TestCtx().AddProtobufItem(proto, "test_item", NAppHost::EContextItemKind::Input);
        }

        auto& itemProxyAdapter = ahCtx.ItemProxyAdapter();
        for (size_t i = strings.size() - 1; i; --i) {
            auto res = itemProxyAdapter.GetFromContextCached<TTestProto>("test_item", i);
            UNIT_ASSERT(res.IsSuccess());
            TTestProto proto;
            res.MoveTo(proto);
            UNIT_ASSERT_VALUES_EQUAL(proto.GetTestString(), strings[i]);
        }
    }
}

} // namespace
