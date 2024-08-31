#include "error.h"

#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/util/status.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;

TErrorMetaBuilder CreateBuilder() {
    TErrorMetaBuilder::TMetaProto nestedError;
    nestedError.SetErrorType("nested_error_type");
    nestedError.SetNetLocation("nested_error_location");
    nestedError.SetOrigin(TErrorMetaBuilder::TMetaProto::EOrigin::TSpeechKitResponseProto_TMeta_EOrigin_Exception);
    nestedError.SetMessage("nested_error_message");

    return TErrorMetaBuilder{TError{TError::EType::BadRequest} << "42"}
        .SetNetLocation("84")
        .AppendNested(TError{TError::EType::ScenarioError} << "nested_second_error_message", "nested_second_error_path")
        .AppendNested(std::move(nestedError));
}

Y_UNIT_TEST_SUITE(ResponseMeta) {
    Y_UNIT_TEST(ErrorBuilderFromTErrorAsJson) {
        const auto js = CreateBuilder().AsJson();
        UNIT_ASSERT_VALUES_EQUAL(js["http_code"].GetInteger(), 400);
        UNIT_ASSERT_VALUES_EQUAL(js["message"].GetString(), "42");
        UNIT_ASSERT_VALUES_EQUAL(js["origin"].GetString(), "Exception");
        UNIT_ASSERT_VALUES_EQUAL(js["error_type"].GetString(), "bad_request");
        UNIT_ASSERT_VALUES_EQUAL(js["type"].GetString(), "error");
        UNIT_ASSERT_VALUES_EQUAL(js["net_location"].GetString(), "84");
        UNIT_ASSERT_VALUES_EQUAL(js["nested"][0]["error_type"].GetString(), "scenario_error");
        UNIT_ASSERT_VALUES_EQUAL(js["nested"][0]["net_location"].GetString(), "nested_second_error_path");
        UNIT_ASSERT_VALUES_EQUAL(js["nested"][0]["message"].GetString(), "nested_second_error_message");
        UNIT_ASSERT_VALUES_EQUAL(js["nested"][0]["origin"].GetString(), "Status");
        UNIT_ASSERT_VALUES_EQUAL(js["nested"][1]["error_type"].GetString(), "nested_error_type");
        UNIT_ASSERT_VALUES_EQUAL(js["nested"][1]["net_location"].GetString(), "nested_error_location");
        UNIT_ASSERT_VALUES_EQUAL(js["nested"][1]["message"].GetString(), "nested_error_message");
        UNIT_ASSERT_VALUES_EQUAL(js["nested"][1]["origin"].GetString(), "Exception");
    }

    Y_UNIT_TEST(ErrorBuilderFromTErrorAsProtoItem) {
        const auto proto = CreateBuilder().AsProtoItem();
        UNIT_ASSERT_VALUES_EQUAL(proto.GetHttpCode(), 400);
        UNIT_ASSERT_VALUES_EQUAL(proto.GetMessage(), "42");
        UNIT_ASSERT_VALUES_EQUAL(proto.GetErrorType(), "bad_request");
        UNIT_ASSERT_VALUES_EQUAL(proto.GetType(), "error");
        UNIT_ASSERT_VALUES_EQUAL(proto.GetNetLocation(), "84");
        UNIT_ASSERT(proto.GetOrigin() == TErrorMetaBuilder::TMetaProto::EOrigin::TSpeechKitResponseProto_TMeta_EOrigin_Exception);
        const auto& nested = proto.GetNestedErrors();
        UNIT_ASSERT_VALUES_EQUAL(nested[0].GetType(), "scenario_error");
        UNIT_ASSERT_VALUES_EQUAL(nested[0].GetNetLocation(), "nested_second_error_path");
        UNIT_ASSERT_VALUES_EQUAL(nested[0].GetMessage(), "nested_second_error_message");
        UNIT_ASSERT(nested[0].GetOrigin() == TErrorMetaBuilder::TMetaProto::EOrigin::TSpeechKitResponseProto_TMeta_EOrigin_Status);
        UNIT_ASSERT_VALUES_EQUAL(nested[1].GetType(), "nested_error_type");
        UNIT_ASSERT_VALUES_EQUAL(nested[1].GetNetLocation(), "nested_error_location");
        UNIT_ASSERT_VALUES_EQUAL(nested[1].GetMessage(), "nested_error_message");
        UNIT_ASSERT(nested[1].GetOrigin() == TErrorMetaBuilder::TMetaProto::EOrigin::TSpeechKitResponseProto_TMeta_EOrigin_Exception);
    }

    Y_UNIT_TEST(ErrorBuilderFromTErrorToHttpResponse) {
        class TTestResponse : public IHttpResponse {
        public:
            IHttpResponse& AddHeader(const THttpInputHeader& ) override {
                return *this;
            }

            IHttpResponse& SetContent(const TString& content) override {
                Content_ = content;
                return *this;
            }

            IHttpResponse& SetContentType(TStringBuf contentType) override {
                ContentType_ = contentType;
                return *this;
            }

            IHttpResponse& SetHttpCode(HttpCodes httpCode) override {
                HttpCode_ = httpCode;
                return *this;
            }

            TString Content() const override {
                return Content_;
            }
            HttpCodes HttpCode() const override {
                return HttpCode_;
            }

            const THttpHeaders& Headers() const {
                return Default<THttpHeaders>();
            }

        protected:
            void DoOut() const override {
            }

        private:
            TString Content_;
            TString ContentType_;
            HttpCodes HttpCode_;
        };

        TTestResponse httpResponse;
        CreateBuilder().ToHttpResponse(httpResponse);
        UNIT_ASSERT_VALUES_EQUAL(httpResponse.Content(), R"({"meta":[{"http_code":400,"error_type":"bad_request","net_location":"84","type":"error","origin":"Exception","nested":[{"error_type":"scenario_error","net_location":"nested_second_error_path","origin":"Status","message":"nested_second_error_message"},{"error_type":"nested_error_type","net_location":"nested_error_location","origin":"Exception","message":"nested_error_message"}],"message":"42"}]})");
    }
}

} // namespace
