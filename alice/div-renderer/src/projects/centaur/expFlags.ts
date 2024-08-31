import { createHasExperiment } from '../../common/helpers/expFlags';

export enum ExpFlags {
    musicPerformanceOpen = 'music_performance_open',
    newFact = 'divrender_new_fact',
    scenarioDataPrettyJSON = 'scenario_data_pretty_json',
    errorOnAssert = 'error_on_assert',
    showError = 'show_error',
    teasersDesignWithDoubleScreenNews = 'teasers_design_with_double_screen_news',
    teasersDesignWithDoubleScreenWeather = 'teasers_design_with_double_screen_weather',
    extendedNewsDesignWithDoubleScreen = 'extended_news_design_with_double_screen',
    extendedNewsDesignWithDoubleScreen2 = 'extended_news_design_with_double_screen2',
    searchRichCardSkeleton = 'search_rich_card_skeleton',
    templateMayNoExist = 'template_may_no_exist',
    scenarioWidgetMechanics = 'scenario_widget_mechanics',
    photoFrameDebugUpdateData = 'photo_frame_debug_update_data',
    div2CardAsString = 'div2_card_as_string',
    renderPlayerLikes = 'render_player_likes',
    telegramCallControlButtons = 'telegram_call_control_buttons',
    mainScreenTypedAction = 'centaur_typed_action',
    videoSearchShowAllResults = 'video_search_show_all_results',
}

export const hasExperiment = createHasExperiment<ExpFlags>();
