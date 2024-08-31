#pragma once

#include "client_directive_model.h"

#include <google/protobuf/struct.pb.h>

#include <util/generic/vector.h>

#include <utility>

namespace NAlice::NMegamind {

class TFindContactsDirectiveModel final : public TClientDirectiveModel {
public:
    class TRequestPart final {
    public:
        explicit TRequestPart(const TString& tag);

        TRequestPart& AddValue(const TString& value);

        [[nodiscard]] const TString& GetTag() const;
        [[nodiscard]] const TVector<TString>& GetValues() const;

    private:
        TString Tag;
        TVector<TString> Values;
    };

public:
    template <typename TColumn, typename TListName, typename TStruct, typename TCallbackPayload, typename TValues>
    TFindContactsDirectiveModel(const TString& analyticsType, TColumn&& mimeTypeWhiteListColumn,
                                TListName&& mimeTypeWhiteListName, TStruct&& onPermissionDeniedCallbackPayload,
                                TCallbackPayload&& request, TValues&& values, const TString& callbackName)
        : TClientDirectiveModel("find_contacts", analyticsType)
        , MimeTypeWhiteListColumn(std::forward<TColumn>(mimeTypeWhiteListColumn))
        , MimeTypeWhiteListName(std::forward<TListName>(mimeTypeWhiteListName))
        , OnPermissionDeniedCallbackPayload(std::forward<TStruct>(onPermissionDeniedCallbackPayload))
        , Request(std::forward<TCallbackPayload>(request))
        , Values(std::forward<TValues>(values))
        , CallbackName(callbackName) {
    }

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] const TVector<TString>& GetMimeTypeWhiteListColumn() const;
    [[nodiscard]] const TVector<TString>& GetMimeTypeWhiteListName() const;
    [[nodiscard]] const google::protobuf::Struct& GetOnPermissionDeniedCallbackPayload() const;
    [[nodiscard]] const TVector<TFindContactsDirectiveModel::TRequestPart>& GetRequest() const;
    [[nodiscard]] const TVector<TString>& GetValues() const;
    [[nodiscard]] const TString& GetCallbackName() const {
        return CallbackName;
    }

private:
    TVector<TString> MimeTypeWhiteListColumn;
    TVector<TString> MimeTypeWhiteListName;
    google::protobuf::Struct OnPermissionDeniedCallbackPayload;
    TVector<TRequestPart> Request;
    TVector<TString> Values;
    TString CallbackName;
};

} // namespace NAlice::NMegamind
