#include "serp_gallery.h"
#include "serp.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/urls_builder.h>

#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <util/string/builder.h>
#include <util/string/split.h>
#include <util/string/subst.h>

namespace NBASS {
namespace NSerpGallery {

namespace {

constexpr TStringBuf FORM_NAME_SERP_GALLERY = "personal_assistant.scenarios.serp_gallery";

TString JoinPassages(const NSc::TValue& passages, size_t startIndex = 0) {
    if (!passages.IsArray()) {
        return TString();
    }

    TStringBuilder builder;
    for (size_t i = startIndex; i < passages.ArraySize(); ++i) {
        const auto& passage = passages.Get(i);
        if (!passage.IsString()) {
            return TString();
        }
        if (i > 0) {
            builder << Endl;
        }
        builder << passage.GetString();
    }

    return builder;
}

bool TryFindPassage(const NSc::TValue& snippet, NSc::TValue& passage) {
    const NSc::TValue& passages = snippet.TrySelect("passages");
    passage = JoinPassages(passages);
    if (!passage.StringEmpty()) {
        return true;
    }

    const NSc::TValue& headline = snippet.TrySelect("headline");
    if (headline.IsString() && !headline.StringEmpty()) {
        passage = headline;
        return true;
    }

    return false;
}

bool TryFindPassage(const TVector<const NSc::TValue*> snippets, NSc::TValue& passage) {
    for (const auto snippet : snippets) {
        if (TryFindPassage(*snippet, passage)) {
            return true;
        }
    }
    return false;
}

TString RemoveHighlight(TString text) {
    SubstGlobal(text, "\u0007[", "");
    SubstGlobal(text, "\u0007]", "");
    return text;
}

NSc::TValue RemoveHighlight(const NSc::TValue& value) {
    if (!value.IsString()) {
        return value;
    }

    return NSc::TValue(RemoveHighlight(TString(value.GetString())));
}

bool TryFindVoicedSnippet(const NSc::TValue& document, NSc::TValue& voiceText, NSc::TValue& voiceTextContinuation) {
    for (const auto* voicedSnippet : NSerpSnippets::FindSnippets(document, "voiced_snippet", NSerpSnippets::ESS_CONSTRUCT)) {
        const auto& passages = voicedSnippet->TrySelect("passages");
        if (passages.IsArray() && passages.ArraySize() > 0) {
            voiceText = passages.Get(0);
            voiceTextContinuation = JoinPassages(passages, 1);
        }

        if (voiceText.IsString() && !voiceText.StringEmpty()) {
            return true;
        }
    }

    return false;
}

TString JoinWithComma(TStringBuf s1, TStringBuf s2) {
    return TStringBuilder() << s1 << ',' << s2;
}

void TryAddMapAndPhoneURIs(const NSc::TValue& document, TContext& ctx, NSc::TValue& output) {
    for (const auto* snippet : NSerpSnippets::FindSnippets(document, "adresa", NSerpSnippets::ESS_SNIPPETS_POST)) {
        const auto& regionLon = snippet->TrySelect("region_lon");
        const auto& regionLat = snippet->TrySelect("region_lat");
        const auto& regionSpnLon = snippet->TrySelect("region_spn_lon");
        const auto& regionSpnLat = snippet->TrySelect("region_spn_lat");
        const auto& what = snippet->TrySelect("what");

        const auto& item = snippet->TrySelect("item");
        const auto& items = snippet->TrySelect("items");
        if (item.IsDict() && items.IsArray()) {
            const auto& phone = item.TrySelect("phone");
            const auto& mapLongitude = item.TrySelect("map_longitude");
            const auto& mapLatitude = item.TrySelect("map_latitude");
            const auto& companyId = item.TrySelect("company_id");
            const auto& url = item.TrySelect("url");

            if (phone.IsString()) {
                output[FIELD_CALL_URI] = GeneratePhoneUri(ctx.MetaClientInfo(), phone.GetString());
            }

            TCgiParameters cgi;
            cgi.InsertUnescaped("source", "adrsnip");
            if (items.ArraySize() > 1 && !regionLon.IsNull() && !regionLat.IsNull() &&
                !regionSpnLon.IsNull() && !regionSpnLat.IsNull() && url.IsString())
            {
                cgi.InsertUnescaped("text", url.GetString());
                cgi.InsertUnescaped("sll", JoinWithComma(regionLon.ForceString(), regionLat.ForceString()));
                cgi.InsertUnescaped("sspn", JoinWithComma(regionSpnLon.ForceString(), regionSpnLat.ForceString()));
                output[FIELD_MAP_URI] = GenerateMapsUri(ctx, cgi);
            }
            else if (items.ArraySize() == 1 && !mapLongitude.IsNull() && !mapLatitude.IsNull() &&
                     what.IsString() && !companyId.IsNull())
            {
                const auto sll = JoinWithComma(mapLongitude.ForceString(), mapLatitude.ForceString());
                cgi.InsertUnescaped("text", what.GetString());
                cgi.InsertUnescaped("sll", sll);
                cgi.InsertUnescaped("ll", sll);
                cgi.InsertUnescaped("oid", companyId.ForceString());
                // TODO(@micyril): fix uri for this case
                // output[FIELD_MAP_URI] = GenerateMapsUri(ctx, cgi);
            }
        }

        break;
    }
}

TVector<TString> GetPronounceableDomains(TStringBuf subdomain) {
   auto names = StringSplitter(subdomain).Split('.').ToList<TStringBuf>();

   if (names.size() < 2) {
       return TVector<TString>();
   }

   TVector<TString> result{TStringBuilder() << names[names.size() - 2] << ' ' << names.back()};
   for (size_t i = 2; i < names.size(); ++i) {
       TStringBuilder subdomainBuilder;
       subdomainBuilder << names[names.size() - 1 - i];
       result.push_back(TStringBuilder() << names[names.size() - 1 - i] << ' ' << result.back());
   }

   return result;
}

void AddPronounceableDomains(NSc::TValue& items) {
    if (!items.IsArray()) {
        return;
    }

    THashMap<TString, size_t> domainToCount;
    TVector<TVector<TString>> pronounceableDomainsCollections;
    for (const auto& item : items.GetArray()) {
        const auto& subdomain = item.TrySelect("domain_name");
        assert(subdomain.IsString());
        auto domains = GetPronounceableDomains(subdomain.GetString());

        for (const auto& domain : domains) {
            ++domainToCount[domain];
        }
        pronounceableDomainsCollections.push_back(domains);
    }

    for (size_t i = 0; i < items.ArraySize(); ++i) {
        const auto& pronounceableDomains = pronounceableDomainsCollections[i];
        auto& item = items.GetOrAdd(i);
        if (pronounceableDomains.empty()) {
            item["tts_url"].SetNull();
        } else {
            size_t i;
            for (i = 1; i < pronounceableDomains.size(); ++i) {
                if (domainToCount[pronounceableDomains[i - 1]] == domainToCount[pronounceableDomains[i]]) {
                    break;
                }
            }
            item["tts_url"] = pronounceableDomains[i - 1];
        }
    }
}

} // namespace anonymous

TSerpGalleryBuilder::TSerpGalleryBuilder(TVoiceAnswerBuilder&& voiceAnswerBuilder, TContext& ctx)
    : Ctx(&ctx)
    , VoiceAnswerBuilder(std::move(voiceAnswerBuilder))
    , MarkAsVoice(ctx.HasExpFlag("enable_serp_gallery_voice_mark"))
    , AddPronounceButton(ctx.HasExpFlag("enable_serp_gallery_pronounce_button"))
    , AddExtraButtons(ctx.HasExpFlag("enable_serp_gallery_extra_buttons"))
    , NoVoiceAtStart(ctx.HasExpFlag("serp_gallery_no_voice_at_start"))
    , EnableLogIdForUrl(ctx.HasExpFlag("serp_gallery_log_id"))
{
    if (ctx.HasExpFlag(EXP_FLAG_ENABLE_SERP_GALLERY_DEBUG)) {
        MarkAsVoice = true;
        AddPronounceButton = true;
        AddExtraButtons = true;
    }
}

TMaybe<TSerpGalleryBuilder> TSerpGalleryBuilder::Create(TContext& ctx) {
    if (!ctx.HasExpFlag(EXPERIMENTAL_FLAG_ENABLE_SERP_GALLERY)) {
        return Nothing();
    }

    TMaybe<TVoiceAnswerBuilder> voiceAnswerBuilder = TVoiceAnswerBuilder::Create(ctx);
    if (!voiceAnswerBuilder) {
        return Nothing();
    }

    return TSerpGalleryBuilder(std::move(*voiceAnswerBuilder), ctx);
}

TMaybe<NSc::TValue> TSerpGalleryBuilder::BuildSerpGalleryItem(const NSc::TValue& document) {
    if (document.TrySelect("server_descr") != "WEB") {
        return Nothing();
    }

    NSc::TValue passage, voiceText, voiceTextContinuation, passageSource;
    if (TryFindVoicedSnippet(document, voiceText, voiceTextContinuation)) {
        passageSource = "voiced_snippet";
        passage = voiceTextContinuation.IsString() ?
            NSc::TValue(TStringBuilder() << voiceText.GetString() << " " << voiceTextContinuation.GetString()) :
            voiceText;
    } else if (TryFindPassage(NSerpSnippets::FindSnippets(document, NSerpSnippets::ESS_CONSTRUCT), passage)) {
        passageSource = "construct";
    } else if (TryFindPassage(NSerpSnippets::FindSnippets(document, NSerpSnippets::ESS_SNIPPETS_ALL), passage)) {
        passageSource = "snippets";
    }

    NSc::TValue result;
    result["text"] = RemoveHighlight(passage);
    result["voice_text"] = voiceText;
    result["voice_text_continuation"] = voiceTextContinuation;
    result["text_source"] = passageSource;
    result["title"] = RemoveHighlight(document.TrySelect("doctitle"));
    result["green_url"] = RemoveHighlight(document.TrySelect("green_url"));
    result["domain_name"] = document.TrySelect("favicon_domain");
    result[FIELD_URL] = document.TrySelect("url");

    if (AddExtraButtons) {
        TryAddMapAndPhoneURIs(document, *Ctx, result);
    }

    return result;
}

void TSerpGalleryBuilder::Build(const TStringBuf query, const NSc::TValue& searchResult) {
    NSc::TValue items;
    items.SetArray();

    const NSc::TValue& docs = searchResult.TrySelect("searchdata").TrySelect("docs");
    for (const auto& doc : docs.GetArray()) {
        TMaybe<NSc::TValue> item = BuildSerpGalleryItem(doc);
        if (item) {
            items.Push() = *item;
        }
    }

    AddPronounceableDomains(items);

    NSc::TValue data;
    data["serp_items"] = items;
    data["serp_url"] = GenerateSearchUri(Ctx, query);

    Ctx = Ctx->SetResponseForm(FORM_NAME_SERP_GALLERY, false /* setCurrentFormAsCallback */).Get();
    Y_ENSURE(Ctx);

    bool isPorno = NSerp::IsPornoSerp(searchResult);

    const auto* callIcon = Ctx->Avatar("serp_gallery", "call");
    const auto* mapIcon = Ctx->Avatar("serp_gallery", "map");
    const auto* pronounceIcon = Ctx->Avatar("serp_gallery", "pronounce");
    data["call_icon"] = callIcon ? NSc::TValue(callIcon->Https) : NSc::Null();
    data["map_icon"] = mapIcon ? NSc::TValue(mapIcon->Https) : NSc::Null();
    data["pronounce_icon"] = !isPorno && AddPronounceButton && pronounceIcon ? NSc::TValue(pronounceIcon->Https) : NSc::Null();

    Ctx->AddDivCardBlock("serp_gallery", data);

    if (NoVoiceAtStart || isPorno) {
        Ctx->AddStopListeningBlock();
    } else {
        VoiceAnswerBuilder.Build(*Ctx, items, 1, true, true);
        Ctx->CreateSlot(SLOT_NAME_ITEMS, SLOT_TYPE_ITEMS, true /* optional */, items);
    }

    if (MarkAsVoice) {
        Ctx->AddAttention("serp_gallery__mark_as_voice");
    }

    if (EnableLogIdForUrl) {
        Ctx->AddAttention("serp_gallery__log_id_for_url");
    }

    if (isPorno) {
        Ctx->AddTextCardBlock("serp_gallery_porno_warning");
    }
    else {
        Ctx->AddTextCardBlock("serp_invitation_message");
    }
}

} // NSerpGallery
} // NBASS
