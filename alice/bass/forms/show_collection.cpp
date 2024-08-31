#include "show_collection.h"

#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS {

namespace {

const TStringBuf SHOW_COLLECTION_FORM_NAME = "personal_assistant.scenarios.show_collection";

static const THashMap<TString, NSc::TValue> COLLECTION_DATA_BY_NAME = {
    {"MuradOsmann", NSc::TValue::FromJson(R"(
        [{
            "url": "https://yandex.ru/collections/user/muradosmann-official/samye-populiarnye-fotografii-followmeto/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1478051/caf967b4-d79c-4eb6-a6fa-a58a28e5eba6/orig",
            "text": "Самые популярные фотографии #FollowMeTo"
        }, {
            "url": "https://yandex.ru/collections/user/muradosmann-official/samye-neobychnye-mesta-planety-ot-followmeto/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1454573/7da0d563-83f5-43fa-8edf-8e67212c1060/orig",
            "text": "Самые необычные места планеты от #FollowMeTo"
        }, {
            "url": "https://yandex.ru/collections/user/muradosmann-official/rossiia-glazami-followmeto/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1454573/a02c38af-539a-43ba-9bf9-15e98a9a2f37/orig",
            "text": "Россия глазами #FollowMeTo"
        }]
    )")},
    {"PokrasLampas", NSc::TValue::FromJson(R"(
        [{
            "url": "https://yandex.ru/collections/user/pokraslampas-official/masshtabnye-proekty/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1478051/18917d11-0a8a-4a29-9624-57e3a8c906b1/orig",
            "text": "Масштабные проекты"
        }, {
            "url": "https://yandex.ru/collections/user/pokraslampas-official/fashion-kh-pokras-lampas/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1478051/1d4edf85-221b-4362-b46f-6c19c92a9b77/orig",
            "text": "Fashion х Покрас Лампас"
        }, {
            "url": "https://yandex.ru/collections/user/pokraslampas-official/pogruzhenie-v-malevicha/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/402806/58b40e74-42d3-4b5f-81c8-61574eec6dbd/orig",
            "text": "Погружение в Малевича"
        }]
    )")},
    {"SergeySuhov", NSc::TValue::FromJson(R"(
        [{
            "url": "https://yandex.ru/collections/user/sergeysuxov-official/ny/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1478051/a0d596f0-f852-4b03-b568-7990ef911c66/orig",
            "text": "NY"
        }, {
            "url": "https://yandex.ru/collections/user/sergeysuxov-official/uk/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1478051/a0a3ffa2-7e91-4baa-b098-eb8f08fd3367/orig",
            "text": "UK"
        }, {
            "url": "https://yandex.ru/collections/user/sergeysuxov-official/letiashchii-sharf/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1465560/7ee42ed0-8967-4667-b5cd-ec80566d3f2f/orig",
            "text": "Летящий шарф"
        }]
    )")},
    {"SergeyKalujny", NSc::TValue::FromJson(R"(
        [{
            "url": "https://yandex.ru/collections/user/kalyuzhniy-sergey-official/moskva/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1454573/6ee68a47-0ecd-4681-9672-1c89c7fc1029/orig",
            "text": "Москва"
        }, {
            "url": "https://yandex.ru/collections/user/kalyuzhniy-sergey-official/rain/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1100421/087724fc-95eb-423d-9ed6-9bc2506d3fea/orig",
            "text": "Rain"
        }, {
            "url": "https://yandex.ru/collections/user/kalyuzhniy-sergey-official/auto/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/402806/53d4a74e-19ed-4bc3-8e75-48272df4f0ae/orig",
            "text": "Auto"
        }]
    )")},
    {"SofiFil", NSc::TValue::FromJson(R"(
        [{
            "url": "https://yandex.ru/collections/user/sonchicc-official/liubimye-knigi-dlia-puteshestvii/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/936888/2c08f2fe-2922-4f15-9378-eb922587f5c6/orig",
            "text": "Любимые книги для путешествий"
        }, {
            "url": "https://yandex.ru/collections/user/sonchicc-official/niderlandy/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1465560/3fe94f4c-0b9b-4c62-a924-56f473ff6d3c/orig",
            "text": "Нидерланды"
        }, {
            "url": "https://yandex.ru/collections/user/sonchicc-official/sankt-peterburg/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1478051/931f72c2-4506-4044-aa9f-2910e0fd1a9e/orig",
            "text": "Санкт-Петербург"
        }]
    )")},
    {"NikolaiSobolev", NSc::TValue::FromJson(R"(
        [{
            "url": "https://yandex.ru/collections/user/nikolay-sobolev-official/top-5-zavedenii/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1537568/7ad89886-770e-4f34-a403-052176573147/orig",
            "text": "Топ 5 заведений"
        }, {
            "url": "https://yandex.ru/collections/user/nikolay-sobolev-official/igry/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1632262/17390195-d1fe-4891-a658-b578cbe38e75/orig",
            "text": "Игры"
        }, {
            "url": "https://yandex.ru/collections/user/nikolay-sobolev-official/knigi/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1426078/0adccb9d-d9f8-4267-902d-d0ef8ea8c5ca/orig",
            "text": "Книги"
        }, {
            "url": "https://yandex.ru/collections/user/nikolay-sobolev-official/top-5-filmov/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1426078/66581c42-4721-4d47-bac9-bbe89c95d58f/orig",
            "text": "Топ 5 фильмов"
        }]
    )")},
    {"DmitriyMaslennikov", NSc::TValue::FromJson(R"(
        [{
            "url": "https://yandex.ru/collections/user/dima-maslen-official/top-liubimykh-kompiuternykh-igr/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1419451/42f33a9b-7809-41f2-bf8f-95af2dfc45ab/orig",
            "text": "Топ любимых компьютерных игр"
        }, {
            "url": "https://yandex.ru/collections/user/dima-maslen-official/top-strashnykh-mest-mira/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1537568/27e1e9f7-144c-4539-94ff-71466da1d645/orig",
            "text": "Топ страшных мест мира"
        }, {
            "url": "https://yandex.ru/collections/user/dima-maslen-official/liubimye-kafe/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1072917/1a5d1139-5dfa-4097-b132-1b821437f60f/orig",
            "text": "Любимые кафе"
        }, {
            "url": "https://yandex.ru/collections/user/dima-maslen-official/knigi/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1605269/6c2935ab-eead-4bee-b387-083d80335382/orig",
            "text": "Книги"
        }, {
            "url": "https://yandex.ru/collections/user/dima-maslen-official/filmy/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1419451/e1e128fd-f5eb-4d80-8216-795d5eb64580/orig",
            "text": "Фильмы"
        }]
    )")},
    {"KonstantinPavlov", NSc::TValue::FromJson(R"(
        [{
            "url": "https://yandex.ru/collections/user/kostya-zzz-official/top-futbolnykh-komand/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1072917/83f28f61-82fe-4fb7-ab69-4d56c0137744/orig",
            "text": "Топ футбольных команд"
        }, {
            "url": "https://yandex.ru/collections/user/kostya-zzz-official/top-kafe-restoranov/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1510992/c876c1e5-a98f-4af4-93a2-5443ddb8cbfe/orig",
            "text": "Топ кафе/ресторанов"
        }, {
            "url": "https://yandex.ru/collections/user/kostya-zzz-official/eda/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1419451/1e1f9d78-3665-41cf-b213-42b422bb63c5/orig",
            "text": "Еда"
        }, {
            "url": "https://yandex.ru/collections/user/kostya-zzz-official/kompiuternye-igry/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1605269/4e3ff820-178e-4023-a74e-9be1e69ce929/orig",
            "text": "Компьютерные игры"
        }, {
            "url": "https://yandex.ru/collections/user/kostya-zzz-official/knigi/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1419451/60489d6d-4f06-4968-b350-019f3a9c8a3d/orig",
            "text": "Книги"
        }, {
            "url": "https://yandex.ru/collections/user/kostya-zzz-official/filmy/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1632262/d9a2421c-d1da-40a3-8fd5-3aa5a8dbf01a/orig",
            "text": "Фильмы"
        }]
    )")},
    {"PolinaTrubenkova", NSc::TValue::FromJson(R"(
        [{
            "url": "https://yandex.ru/collections/user/madam-kaka-official/kafe-restorany-zabegalovki/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/936888/e41ee4dd-dfca-4384-8867-aa0afc4b9a91/orig",
            "text": "Кафе/рестораны/забегаловки"
        }, {
            "url": "https://yandex.ru/collections/user/madam-kaka-official/filmy/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1350356/1d41babb-189c-45b9-9b2d-525aa1a70f0b/orig",
            "text": "Фильмы"
        }, {
            "url": "https://yandex.ru/collections/user/madam-kaka-official/serialy/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1570136/6069e0d2-c638-426e-b78c-7c2a6ecb2f5d/orig",
            "text": "Сериалы"
        }, {
            "url": "https://yandex.ru/collections/user/madam-kaka-official/knigi/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1537568/02dabbbb-d4a8-4bbc-8af3-d7e0440a2ef7/orig",
            "text": "Книги"
        }, {
            "url": "https://yandex.ru/collections/user/madam-kaka-official/top-liubimykh-igr-na-ps/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1072917/df2a8a7f-ac4b-4d83-b4d8-5bbabfa5a3c0/orig",
            "text": "Топ любимых игр на PS"
        }, {
            "url": "https://yandex.ru/collections/user/madam-kaka-official/top-liubimykh-vin/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1632262/d8433f21-b0ea-45e5-8032-7c3495920511/orig",
            "text": "Топ любимых вин"
        }, {
            "url": "https://yandex.ru/collections/user/madam-kaka-official/top-seksualnykh-muzhchin-v-kino/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/936888/aeff0fa2-608d-4bec-b597-7e26b1586a43/orig",
            "text": "Топ сексуальных мужчин в кино"
        }]
    )")},
    {"OlgaMarkes", NSc::TValue::FromJson(R"(
        [{
            "url": "https://yandex.ru/collections/user/olgamarkes-official/knigi-kotorye-ia-liubliu/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1632262/7a42948a-56df-4997-87b8-8ab48dbd527d/orig",
            "text": "Книги, которые я люблю"
        }, {
            "url": "https://yandex.ru/collections/user/olgamarkes-official/knigi-na-kotorykh-ia-vzroslela/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1072917/39a1b6d6-ed7a-4806-a4f6-adf462986cd1/orig",
            "text": "Книги, на которых я взрослела"
        }, {
            "url": "https://yandex.ru/collections/user/olgamarkes-official/knigi-na-kotorykh-ia-rosla/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1510992/a868e4a9-70e4-4806-b7dc-6cdfca7209f8/orig",
            "text": "Книги, на которых я росла"
        }, {
            "url": "https://yandex.ru/collections/user/olgamarkes-official/trenirovki-nedeli/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1510992/f90f5d79-6320-4508-9dec-e3a806e96166/orig",
            "text": "Тренировки недели"
        }, {
            "url": "https://yandex.ru/collections/user/olgamarkes-official/retsepty-osnovnykh-bliud-dlia-pravilnogo-pitaniia/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1633006/c44a8f41-1d0b-4b91-b707-c18d8471f80f/orig",
            "text": "Рецепты основных блюд для правильного питания"
        }, {
            "url": "https://yandex.ru/collections/user/olgamarkes-official/retsepty-salatov-dlia-pravilnogo-pitaniia/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1419451/704d24e7-2bd9-4ce9-b59c-2790ee8a5d8b/orig",
            "text": "Рецепты салатов для правильного питания"
        }, {
            "url": "https://yandex.ru/collections/user/olgamarkes-official/retsepty-desertov-dlia-pravilnogo-pitaniia/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/225246/a60f1cd1-b3e1-4e3d-9fbf-4ab24f8b510b/orig",
            "text": "Рецепты десертов для правильного питания"
        }, {
            "url": "https://yandex.ru/collections/user/olgamarkes-official/retsepty-zavtrakov-dlia-pravilnogo-pitaniia/",
            "image_url": "https://avatars.mds.yandex.net/get-pdb-teasers/1605269/f73b070f-025a-4bda-82c5-1bd895abc948/orig",
            "text": "Рецепты завтраков для правильного питания"
        }]
    )")}
};
}

TResultValue TShowCollectionHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::SHOW_COLLECTION);
    if (!ctx.ClientFeatures().SupportsDivCards()) {
        NSc::TValue errorData;
        errorData["code"] = TStringBuf("div_cards_not_supported");
        ctx.AddErrorBlock(TError(TError::EType::NOTSUPPORTED), std::move(errorData));
        return TResultValue();
    }
    TContext::TSlot* slotCollection = ctx.GetSlot("collection", "collection");
    if (IsSlotEmpty(slotCollection)) {
        NSc::TValue errorData;
        errorData["code"].SetString("collection_not_found");
        ctx.AddErrorBlock(TError(TError::EType::COLLECTIONERROR, "collection not found"), std::move(errorData));
        return TResultValue();
    }
    TStringBuf collectionName = slotCollection->Value.GetString();
    if (!COLLECTION_DATA_BY_NAME.contains(collectionName)) {
        NSc::TValue errorData;
        errorData["code"].SetString("collection_not_found");
        ctx.AddErrorBlock(TError(TError::EType::COLLECTIONERROR, "collection not found"), std::move(errorData));
        return TResultValue();
    }
    NSc::TValue data;
    data["collections"] = COLLECTION_DATA_BY_NAME.at(collectionName);
    ctx.AddTextCardBlock("show_collection");
    ctx.AddDivCardBlock("show_collection_gallery", data);
    return TResultValue();
}

void TShowCollectionHandler::Register(THandlersMap* handlers) {
    handlers->emplace(SHOW_COLLECTION_FORM_NAME, []() { return MakeHolder<TShowCollectionHandler>(); });
}

}

