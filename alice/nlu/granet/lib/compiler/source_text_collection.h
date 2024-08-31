#pragma once

#include "data_loader.h"

#include <alice/nlu/granet/lib/grammar/domain.h>
#include <alice/nlu/granet/lib/utils/utils.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/scheme/scheme_cast.h>
#include <library/cpp/scheme/scheme.h>

#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/generic/map.h>
#include <util/generic/maybe.h>
#include <util/generic/singleton.h>
#include <util/generic/string.h>
#include <util/stream/file.h>

namespace NGranet::NCompiler {

// ~~~~ TSourceTextCollection ~~~~

class TSourceTextCollection : public NJsonConverters::IJsonSerializable, public TMoveOnly {
public:
    TGranetDomain Domain;
    TString MainTextPath;
    THashMap<TString, TString> Texts;
    TMaybe<TString> ExternalSource;

public:
    bool IsEmpty() const;

    virtual NSc::TValue ToTValue() const override;
    virtual void FromTValue(const NSc::TValue& value, const bool validate) override;

    TString ToBase64() const;
    void FromBase64(TStringBuf base64);

    TString ToCompressedBase64() const;
    void FromCompressedBase64(TStringBuf str);
};

// ~~~~ TReaderFromSourceTextCollection ~~~~

class TReaderFromSourceTextCollection : public IDataLoader {
public:
    explicit TReaderFromSourceTextCollection(const TSourceTextCollection& collection);

    virtual bool IsFile(const TFsPath& path) override;
    virtual TString ReadTextFile(const TFsPath& path) override;

private:
    const TSourceTextCollection& Collection;
};

} // namespace NGranet::NCompiler
