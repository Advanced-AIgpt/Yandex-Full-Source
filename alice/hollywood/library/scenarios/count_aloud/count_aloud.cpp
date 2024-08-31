#include <alice/hollywood/library/scenarios/count_aloud/nlg/register.h>

#include <alice/hollywood/library/base_scenario/scenario.h>

#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/frame/slot.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <util/string/join.h>

namespace NAlice::NHollywood {
    namespace {
        constexpr TStringBuf COUNT_ALOUD_FRAME = "alice.count_aloud";
        constexpr TStringBuf COUNT_FROM_SLOT = "count_from";
        constexpr TStringBuf COUNT_ORDER_DESCENDING = "descending";
        constexpr TStringBuf COUNT_ORDER_SLOT = "count_order";
        constexpr TStringBuf COUNT_TO_SLOT = "count_to";
        constexpr TStringBuf NLG_PHRASE_NAME = "result";
        constexpr TStringBuf NLG_TEMPLATE_NAME = "count_aloud";

        constexpr ui32 MAX_NUMBERS_ALLOWED = 100u;

        TMaybe<i32> GetNumSlot(const TFrame& frame, const TStringBuf slotName) {
            i32 value = 0;
            if (const TPtrWrapper<TSlot> slot = frame.FindSlot(slotName)) {
                TryFromString(slot->Value.AsString(), value);
                return value;
            }
            return Nothing();
        }

        TMaybe<TString> GetStrSlot(const TFrame& frame, const TStringBuf slotName) {
            if (const TPtrWrapper<TSlot> slot = frame.FindSlot(slotName)) {
                return slot->Value.AsString();
            }
            return Nothing();
        }

        std::pair<i32, i32> CreateBorders(
            const TMaybe<i32>& countFromValue,
            const TMaybe<i32>& countToValue,
            const TMaybe<TString>& countOrderValue
        ) {
            if (countOrderValue.Defined() && countOrderValue.GetRef() == COUNT_ORDER_DESCENDING) {
                if (countFromValue.Defined() && countToValue.Defined()) {
                    if (countFromValue.GetRef() < countToValue.GetRef()) {
                        // count from 2 to 5 backwards -> (5, 2)
                        return {countToValue.GetRef(), countFromValue.GetRef()};
                    } else {
                        // count from 5 to 2 backwards -> (5, 2)
                        return {countFromValue.GetRef(), countToValue.GetRef()};
                    }
                } else if (countToValue.Defined()) {
                    const i32 countTo = countToValue.GetRef();
                    if (countTo > 0) {
                        // count to 5 backwards -> (5, 1)
                        return {countTo, 1};
                    } else if (countTo == 0) {
                        // count to 0 backwards -> (10, 0)
                        return {10, countTo};
                    } else {
                        // count to -5 backwards -> (0, -5)
                        return {0, countTo};
                    }
                } else if (countFromValue.Defined()) {
                    const i32 countFrom = countFromValue.GetRef();
                    if (countFrom > 0) {
                        // count from 5 backwards -> (5, 1)
                        return {countFrom, 1};
                    } else if (countFrom == 0) {
                        // count from 0 backwards -> (0, -10)
                        return {countFrom, -10};
                    } else {
                        // count from -5 backwards -> (0, -5)
                        return {0, countFrom};
                    }
                } else {
                    // count backwards -> (10, 1)
                    return {10, 1};
                }
            } else {
                if (countFromValue.Defined() && countToValue.Defined()) {
                    // count from 2 to 5 -> (2, 5)
                    return {countFromValue.GetRef(), countToValue.GetRef()};
                } else if (countToValue.Defined()) {
                    const i32 countTo = countToValue.GetRef();
                    if (countTo > 0) {
                        // count to 5 -> (1, 5)
                        return {1, countTo};
                    } else if (countTo == 0) {
                        // count to 0 -> (10, 0)
                        return {10, countTo};
                    } else {
                        // count to -5 -> (0, -5)
                        return {0, countTo};
                    }
                } else if (countFromValue.Defined()) {
                    const i32 countFrom = countFromValue.GetRef();
                    // count from 5 -> (5, 5+10)
                    return {countFrom, countFrom + 10};
                } else {
                    // count -> (1, 10)
                    return {1, 10};
                }
            }

            Y_UNREACHABLE();
        }

        std::pair<TString, bool> CreateSequence(const i32 countFrom, i32 countTo) {
            ui32 cnt = std::abs(countFrom - countTo) + 1;
            bool cropped = false;
            if (cnt > MAX_NUMBERS_ALLOWED) { // See https://st.yandex-team.ru/ALICE-16948
                countTo = countFrom + MAX_NUMBERS_ALLOWED - 1;
                cnt = MAX_NUMBERS_ALLOWED;
                cropped = true;
            }

            const bool reversed = countFrom > countTo;
            TVector<TString> sequence(Reserve(cnt));
            for (i32 idx = countFrom; reversed ? idx >= countTo : idx <= countTo; reversed ? --idx : ++idx) {
                sequence.emplace_back(ToString(idx));
            }
            return {JoinSeq(", ", sequence), cropped};
        }

        class THandle: public TScenario::THandleBase {
        public:
            TString Name() const override {
                return "run";
            }

            void Do(TScenarioHandleContext& ctx) const override {
                const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
                const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
                const TFrame frame = request.Input().CreateRequestFrame(COUNT_ALOUD_FRAME);

                const auto [countFrom, countTo] = CreateBorders(
                    GetNumSlot(frame, COUNT_FROM_SLOT),
                    GetNumSlot(frame, COUNT_TO_SLOT),
                    GetStrSlot(frame, COUNT_ORDER_SLOT)
                );

                TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
                TRunResponseBuilder builder(&nlgWrapper);
                TResponseBodyBuilder& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);
                bodyBuilder.CreateAnalyticsInfoBuilder().SetProductScenarioName("count_aloud");

                TNlgData nlgData{ctx.Ctx.Logger(), request};
                LOG_INFO(ctx.Ctx.Logger()) << "Building sequence between " << countFrom << " and " << countTo;
                const auto [numbers, cropped] = CreateSequence(countFrom, countTo);
                nlgData.Context["numbers"] = numbers;
                nlgData.Context["cropped"] = cropped;
                if (cropped) {
                    nlgData.Context["count"] = MAX_NUMBERS_ALLOWED;
                }
                bodyBuilder.AddRenderedTextAndVoice(NLG_TEMPLATE_NAME, NLG_PHRASE_NAME, nlgData);

                ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
            }
        };
    }

    REGISTER_SCENARIO(
        "count_aloud",
        AddHandle<THandle>()
            .SetNlgRegistration(NLibrary::NScenarios::NCountAloud::NNlg::RegisterAll)
    );
}
