namespace NBASSSkill;

struct TCardBlock {
    struct TCase {
        idx: string (required);
        activation: string (required);
        description: string (required);
        logo: string (required);
        logo_fg_image: string;
        logo_bg_image: string;
        logo_amelie_fg_url: string;
        logo_amelie_bg_url: string;
        logo_amelie_bg_wide_url: string;
        logo_bg_color: string (required);
        recommendation_type: string (required);
        recommendation_source: string (required);
        name: string (required);
        look: string (required);
    };

    cases : [ TCase ] (required);
    store_url: string (required);
    recommendation_type: string (required);
    recommendation_source: string (required);
};
