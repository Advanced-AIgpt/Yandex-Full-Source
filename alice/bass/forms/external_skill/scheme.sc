namespace NBASSExternalSkill;

struct TStyle {
    oknyx_logo               : string (allowed = ["alice", "microphone"]);
    oknyx_normal_colors      : [ string ] (required);
    oknyx_error_colors       : [ string ] (required);
    suggest_border_color     : string (required);
    suggest_text_color       : string (required);
    suggest_fill_color       : string (required);
    user_bubble_text_color   : string (required);
    user_bubble_fill_color   : string (required);
    skill_bubble_text_color  : string (required);
    skill_bubble_fill_color  : string (required);
    skill_actions_text_color : string (required);
};

struct TChatInfo {
    struct TMenuItem {
        title : string (required);
        url   : string (required);
    };

    dialog_id  : string;
    title      : string (required);
    image_url  : string (required);
    url        : string (required);
    voice      : string (required);
    menu_items : [ TMenuItem ] (required);
    style      : TStyle;
    dark_style : TStyle;
    listening_is_possible : bool;
};

struct TSocialApp {
    applicationName : string;
};

struct TAdminApiResponse {
    result : struct {
        accountLinking      : struct {
            applicationName : string;
        };
        allowedSocialApplications : [TSocialApp];
        backendSettings     : struct {
            uri         : string;
            functionId : string;
            (validate uri) {
                if (Uri()->Size()) {
                    helper->CheckForUrl("skill_url", Uri());
                }
            };
        };
        botGuid             : string;
        monitoringType      : string;
        developerName       : string;
        description         : string;
        category            : string (default = "");
        examples            : [ string ];
        exposeInternalFlags : bool (cppname = IsInternal, default = false);
        id                  : string (required);
        logo (required)     : struct {
            avatarId : string (required);
        };
        look                : string (default = "external");
        name                : string (cppname = Title, required);
        onAir               : bool (default = false);
        isRecommended       : bool (default = false);
        isVip               : bool (default = false);
        openInNewTab        : bool (default = true);
        salt                : string (required);
        storeUrl            : string (required);
        useZora             : bool (default = false);
        useNLU              : bool (default = false);
        voice               : string;
        platforms           : [ string ];
        featureFlags        : [ string ];
        score               : double (default = 0);
    };
    error (cppname = Error) : struct {
        code    : i64;
        message : string;
    };
    (validate) {
        if (HasError()) {
            helper->AddApiError(Error().Code(), Error().Message());
        }
    };
};

struct TSkillResponse1x {
    struct IBaseButton {
        payload : any;
        url     : string;

        (validate url) {
            helper->CheckForUrl("button/url", Url());
        };
        (validate payload) {
            helper->CheckForSize(subPath, Payload(),
                0, 4096
            );
        };
    };

    struct TButton : IBaseButton {
        title : string;
        hide  : bool (default = false);
        (validate title) {
            helper->CheckForUtf8Size(subPath, Title(), 1, 128);
            helper->AddForAbuseChecking(subPath, Title());
        };
    };

    struct TDivButton : IBaseButton {
        text : string;

        (validate text) {
            helper->CheckForUtf8Size(subPath, Text(), 0, 128);
            helper->AddForAbuseChecking(subPath, Text());
        };
        (validate) {
            if (TConst::Url().IsNull() && TConst::Payload().IsNull()) {
                helper->AddApiError(500, "url or payload must specified");
            }
        };
    };

    struct TCard {
        // XXX inherit from simple button????
        struct TItem {
            image_id        : string;
            mds_namespace   : string (default = "dialogs-skill-card");
            image_size      : string (default = "menu-list-x");
            title           : string;
            description     : string;
            button          : TDivButton;

            (validate title) {
                helper->CheckForUtf8Size(subPath, Title(), 1, 128);
                helper->AddForAbuseChecking(subPath, Title());
            };
            (validate description) {
                helper->CheckForUtf8Size(subPath, Description(), 0, 256);
                helper->AddForAbuseChecking(subPath, Description());
            };
            (validate image_id) {
                if (!ImageId()->empty()) {
                    helper->CheckForImageId(ImageId(), subPath);
                }
            };
        };

        type    : string;
        card_id : string; // TODO add some restrictions

        // BigImage
        image_id        : string (cppname = ImageId);
        mds_namespace   : string (default = "dialogs-skill-card");
        image_size      : string (default = "one-x");
        title           : string;
        description     : string;
        button          : TDivButton;

        (validate title) {
            helper->CheckForUtf8Size(subPath, Title(), 0, 128);
            helper->AddForAbuseChecking(subPath, Title());
        };
        (validate description) {
            helper->CheckForUtf8Size(subPath, Description(), 0, 256);
            helper->AddForAbuseChecking(subPath, Description());
        };
        (validate image_id) {
            helper->CheckForImageId(ImageId(), subPath);
        };

        // ItemsList
        header : struct {
            text : string;
            (validate text) {
                helper->CheckForUtf8Size(subPath, Text(), 1, 128);
                helper->AddForAbuseChecking(subPath, Text());
            };
        };

        footer : struct {
            text   : string;
            button : TDivButton;
            (validate text) {
                helper->CheckForUtf8Size(subPath, Text(), 1, 128);
                helper->AddForAbuseChecking(subPath, Text());
            };
        };

        items : [ TItem ];

        // main validator
        (validate type) {
            if (Type() == "BigImage"sv) {
                if (ImageId()->empty()) {
                    helper->AddProblem(NBASS::NExternalSkill::ERRTYPE_IMAGE_ID, "/response/card/image_id", "mandatory field");
                }
            }
            else if (Type() == "ItemsList"sv) {
                const size_t sz = Items().Size();
                if (sz < 1 || sz > 5) {
                    helper->AddProblem(NBASS::NExternalSkill::ERRTYPE_CARD_ITEMS, "/response/card/items", "number of items must be from 1 to 5");
                }
            }
            else {
                helper->AddProblem(NBASS::NExternalSkill::ERRTYPE_CARD_TYPE, subPath, "unsupported card type");
            }
        };
    };

    version : string;
    session : struct {
        session_id : string (cppname = SessionId);
        user_id    : string (cppname = UserId);
        message_id : ui64   (cppname = MessageId);

        (validate user_id) {
            helper->CheckForSize(subPath, UserId(), 64, 64);
        };
        (validate session_id) {
            helper->CheckForSize(subPath, SessionId(), 1, 64);
        };
        (validate message_id) {
            helper->CheckForNotNull(subPath, *MessageId().GetRawValue());
        };
    };
    response : struct {
        text : string (cppname = Text, required);
        tts  : string (cppname = TTS);
        card : TCard;
        buttons : [TButton];
        end_session: bool (cppname = EndSession, default = false);

        (validate text) {
            helper->CheckForUtf8Size(subPath, Text(), 1, 1024);
            helper->AddForAbuseChecking(subPath, Text());
        };
        (validate tts) {
            helper->CheckForUtf8Size(subPath, TTS(), 0, 1024);
            helper->AddForAbuseChecking(subPath, TTS(), true);
        };
    };
    start_account_linking : struct {
    };
    start_purchase : struct {
        struct TProduct {
            product_id : string;
            title : string;
            user_price : double;
            price : double;
            currency : string;
            purchase_type : string;
            purchase_payload : any;
        };

        purchase_request_id : string;
        merchant_key : string;
        title : string;
        image_url : string;
        description : string;
        products : [TProduct];
    };
    (validate) {
        helper->CheckForSize("/version", Version(), 3, 7);
        helper->CheckForNotNull("/session", *Session().GetRawValue());
        if (Response().GetRawValue()->IsNull() &&
            StartAccountLinking().GetRawValue()->IsNull() &&
            StartPurchase().GetRawValue()->IsNull())
        {
            helper->CheckForNotNull("/response", *Response().GetRawValue());
            helper->CheckForNotNull("/start_account_linking", *StartAccountLinking().GetRawValue());
            helper->CheckForNotNull("/start_purchase", *StartPurchase().GetRawValue());
        }
    };
};
