namespace NBASSSkill;

struct TServiceResponse {
    struct TItem {
        id: string (required);
        activation: string (required);
        description: string (required);
        logo_prefix: string (required);
        logo_avatar_id: string (required);
        logo_fg_image_id: string;
        logo_bg_image_id: string;
        logo_amelie_fg_url: string;
        logo_amelie_bg_url: string;
        logo_amelie_bg_wide_url: string;
        logo_bg_color: string (required);
        look: string (required);
        name: string;
    };
    struct TEditorsAnswer {
        struct TMatcher {
            struct TSkills {
                type: string (required);
                values: [string] (required);
            };
            skills: TSkills (required);
        };
        matcher: TMatcher (required);
        text: string (required);
    };

    items : [ TItem ] (required);
    recommendation_type: string (required);
    recommendation_source: string (required);
    editors_answer: TEditorsAnswer;
};

