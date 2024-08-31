#pragma once

#include <dict/mt/libs/nn/voc.h>
#include <dict/mt/libs/libmt/token.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/string.h>
#include <util/generic/ptr.h>

#include <dict/dictutil/ios.h>
#include <dict/dictutil/str.h>
#include <dict/dictutil/version.h>

#include <util/string/join.h>


class TTokenizer {
public:
    TTokenizer(const TString& bpeVocPath, const ELanguage language = LanguageByName("ru")) {
        TString tokenizerParams = "class=SpaceTokenizer;filter=SegmenterFilter;mode=BPE;file=" + bpeVocPath + ";sentinels=1;langSplit=0;filter=UrlFilter";

        const int intVer = NDict::NMT::TTokenizer::GetVersionFromParams(tokenizerParams);
        TVersion version = TVersion(NDict::NMT::VER_MAJOR(intVer), NDict::NMT::VER_MINOR(intVer));

        NDict::NMT::TTokenizerParams tokenParams;
        tokenParams.Version = NDict::NMT::MK_VER(version.Major(), version.Minor());
        tokenParams.SetLang(language, language);
        tokenParams.SetParams(tokenizerParams);

        Tokenizer = THolder<NDict::NMT::TTokenizer>(NDict::NMT::TTokenizer::Create(tokenParams, nullptr));
    }

    TString Tokenize(const TString& string) {
        TStringReader in(UTF8ToWide(string));
        std::unique_ptr<NDict::NMT::ITokenInputStream> tokens(Tokenizer->Tokenize(&in));

        TVector<TString> tokenStrings;
        for (NDict::NMT::TToken token; tokens->Read(token);) {
            tokenStrings.push_back(WideToUTF8(token.Text));
        }

        return JoinSeq(" ", tokenStrings);
    };

private:
    THolder<NDict::NMT::TTokenizer> Tokenizer;
};

