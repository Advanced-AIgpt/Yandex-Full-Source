#include <alice/nlg/library/runtime_api/globals.h>
#include <alice/nlg/library/runtime_api/exceptions.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NNlg;

Y_UNIT_TEST_SUITE(TestGlobals) {
    Y_UNIT_TEST(Simple) {
        TGlobals import;
        import.ResolveStore("foo") = TValue::Integer(1);

        TGlobals fromImport;
        fromImport.ResolveStore("bar") = TValue::Integer(2);

        TGlobals top;
        top.RegisterImport("import", import);
        top.RegisterFromImport("bar", "baz", fromImport);
        top.RegisterFromImport("awol1", "awol1", fromImport);
        top.ResolveStore("hello") = TValue::String("world");

        // self
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("world"), top.ResolveLoad("hello"));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), top.ResolveLoad("hello2"));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), top.ResolveLoad("hello", "import"));

        // import (positive, negative)
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(1), top.ResolveLoad("foo", "import"));
        UNIT_ASSERT_EXCEPTION(top.ResolveLoad("foo", "import2"), TImportError);
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), top.ResolveLoad("foo2", "import"));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), top.ResolveLoad("foo"));

        // from import (positive, negative)
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(2), top.ResolveLoad("baz"));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), top.ResolveLoad("bar"));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), top.ResolveLoad("awol1"));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), top.ResolveLoad("awol2"));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), top.ResolveLoad("bar", "import"));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), top.ResolveLoad("baz", "import"));
    }

    Y_UNIT_TEST(FromImportShadowing) {
        TGlobals fromImport;
        fromImport.ResolveStore("foo") = TValue::Integer(1);

        TGlobals top;
        top.RegisterFromImport("foo", "foo", fromImport);
        top.ResolveStore("foo") = TValue::Integer(2);

        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(2), top.ResolveLoad("foo"));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(1), fromImport.ResolveLoad("foo"));
    }

    Y_UNIT_TEST(IntransitiveImport) {
        TGlobals bottom;
        bottom.ResolveStore("foo") = TValue::Integer(1);

        TGlobals middle;
        middle.RegisterFromImport("foo", "foo", bottom);

        TGlobals top;
        top.RegisterImport("middle", middle);

        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(1), bottom.ResolveLoad("foo"));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(1), middle.ResolveLoad("foo"));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), top.ResolveLoad("foo", "middle"));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), top.ResolveLoad("foo"));
    }

    Y_UNIT_TEST(IntransitiveFromImport) {
        TGlobals bottom;
        bottom.ResolveStore("foo") = TValue::Integer(1);

        TGlobals middle;
        middle.RegisterFromImport("foo", "foo", bottom);

        TGlobals top;
        top.RegisterFromImport("foo", "foo", middle);

        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(1), bottom.ResolveLoad("foo"));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(1), middle.ResolveLoad("foo"));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), top.ResolveLoad("foo"));
    }

    Y_UNIT_TEST(Chain) {
        TGlobals bottomImport;
        bottomImport.ResolveStore("bar") = TValue::Integer(3);

        TGlobals bottom;
        bottom.RegisterImport("import", bottomImport);
        bottom.ResolveStore("foo") = TValue::Integer(1);

        TGlobals topImport;
        topImport.ResolveStore("bar") = TValue::Integer(4);

        TGlobals top;
        top.RegisterImport("import", topImport);
        top.ResolveStore("foo") = TValue::Integer(2);

        TGlobalsChain chain1{nullptr, &top};
        TGlobalsChain chain2{&chain1, &bottom};

        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(2), chain2.ResolveLoad("foo"));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(3), chain2.ResolveLoad("bar", "import"));
    }
}
