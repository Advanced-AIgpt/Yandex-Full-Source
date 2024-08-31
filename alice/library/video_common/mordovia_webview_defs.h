#pragma once

#include <util/generic/strbuf.h>

namespace NAlice::NVideoCommon {

// fields
//  common
inline constexpr TStringBuf WEBVIEW_PARAM_EXP_FLAGS = "exp_flags";
inline constexpr TStringBuf WEBVIEW_PARAM_TEXT = "text";
//  entities
inline constexpr TStringBuf WEBVIEW_PARAM_ENTREF = "entref";
inline constexpr TStringBuf WEBVIEW_PARAM_UUIDS = "uuids";
//  videosearch gallery
inline constexpr TStringBuf WEBVIEW_PARAM_P = "p";
//  single entity
inline constexpr TStringBuf WEBVIEW_PARAM_SINGLE = "single";
inline constexpr TStringBuf WEBVIEW_PARAM_LANDING_URL = "landingUrl";
//  tvshow
inline constexpr TStringBuf WEBVIEW_PARAM_OFFSET = "offset";
inline constexpr TStringBuf WEBVIEW_PARAM_SEASON = "season";
inline constexpr TStringBuf WEBVIEW_PARAM_SEASON_ID = "season_id";


// flags
inline constexpr TStringBuf FLAG_WEBVIEW_VIDEO_HOST = "mordovia_video_base_url";

inline constexpr TStringBuf FLAG_WEBVIEW_VIDEO_GALLERY_PATH = "mordovia_video_gallery_path";
inline constexpr TStringBuf FLAG_WEBVIEW_VIDEO_GALLERY_SPLASH = "mordovia_video_gallery_splash";

inline constexpr TStringBuf FLAG_WEBVIEW_FILMS_GALLERY_PATH = "mordovia_films_gallery_path";
inline constexpr TStringBuf FLAG_WEBVIEW_FILMS_GALLERY_SPLASH = "mordovia_films_gallery_splash";

inline constexpr TStringBuf FLAG_WEBVIEW_VIDEO_ENTITY_PATH = "mordovia_video_single_card_path";
inline constexpr TStringBuf FLAG_WEBVIEW_VIDEO_ENTITY_SPLASH = "mordovia_video_single_card_splash";

inline constexpr TStringBuf FLAG_WEBVIEW_VIDEO_PROMO_SPLASH = "mordovia_video_webview_promo_splash";

inline constexpr TStringBuf FLAG_WEBVIEW_VIDEO_DESCRIPTION_CARD_PATH = "mordovia_video_description_card_path";
inline constexpr TStringBuf FLAG_WEBVIEW_VIDEO_DESCRIPTION_CARD_SPLASH = "mordovia_video_description_card_splash";

inline constexpr TStringBuf FLAG_WEBVIEW_VIDEO_ENTITY_SEASONS_PATH = "mordovia_video_seasons_gallery_path";
inline constexpr TStringBuf FLAG_WEBVIEW_VIDEO_ENTITY_SEASONS_SPLASH = "mordovia_video_seasons_gallery_splash";

inline constexpr TStringBuf FLAG_WEBVIEW_CHANNELS_PATH = "mordovia_channels_path";
inline constexpr TStringBuf FLAG_WEBVIEW_CHANNELS_SPLASH = "mordovia_channels_splash";

inline constexpr TStringBuf FLAG_MORDOVIA_MAIN_SCREEN_SPLASH = "mordovia_main_splash";

inline constexpr TStringBuf FLAG_MORDOVIA_CGI_STRING = "mordovia_cgi_string";

// defaults
inline constexpr TStringBuf DEFAULT_WEBVIEW_VIDEO_HOST = "https://yandex.ru";

inline constexpr TStringBuf DEFAULT_VIDEO_GALLERY_PATH = "/video/quasar/videoSearch/0/";

inline constexpr TStringBuf DEFAULT_VIDEO_GALLERY_SPLASH = TStringBuf(R"({"card":{"log_id":"station_video_search","states":[{"state_id":0,"div":{"type":"body"}}]},"templates":{"body":{"type":"container","orientation":"horizontal","background":[{"color":"#151517","type":"solid"}],"paddings":{"top":328,"left":80},"width":{"type":"match_parent","value":1920},"height":{"type":"fixed","value":1080},"items":[{"type":"cards"}]},"cards":{"type":"container","orientation":"horizontal","height":{"type":"fixed","value":383},"items":[{"type":"card"},{"type":"card"},{"type":"card"},{"type":"card"}]},"card":{"type":"container","margins":{"right":24},"width":{"type":"fixed","value":504},"height":{"type":"match_parent"},"items":[{"type":"placeholder-image","width":{"type":"fixed","value":504},"height":{"type":"fixed","value":284}},{"type":"placeholder-text","width":{"type":"fixed","value":450},"height":{"type":"fixed","value":28},"margins":{"top":28}},{"type":"placeholder-text","width":{"type":"fixed","value":300},"height":{"type":"fixed","value":28},"margins":{"top":8}}]},"placeholder-image":{"type":"placeholder","border":{"corner_radius":12},"height":{"type":"match_parent","value":24}},"placeholder-text":{"type":"placeholder","margins":{"left":24},"border":{"corner_radius":4},"height":{"type":"fixed","value":24}},"placeholder":{"type":"separator","delimiter_style":{"color":"#212123"},"background":[{"color":"#212123","type":"solid"}]}}})");

inline constexpr TStringBuf DEFAULT_FILMS_GALLERY_PATH = "/video/quasar/filmsSearch/0/";
inline constexpr TStringBuf DEFAULT_FILMS_GALLERY_SPLASH = TStringBuf(R"({"card":{"log_id":"station_home_carousel","states":[{"state_id":0,"div":{"type":"body"}}]},"templates":{"body":{"type":"container","orientation":"vertical","background":[{"color":"#151517","type":"solid"}],"paddings":{"top":140,"left":80},"width":{"type":"match_parent","value":1920},"height":{"type":"fixed","value":1080},"items":[{"type":"cards"}]},"cards":{"type":"container","orientation":"horizontal","height":{"type":"fixed","value":614},"margins":{"bottom":245},"items":[{"type":"card"},{"type":"card"},{"type":"card"},{"type":"card"},{"type":"card"}]},"card":{"type":"container","margins":{"right":40},"width":{"type":"fixed","value":410},"height":{"type":"match_parent"},"items":[{"type":"placeholder-image"}]},"placeholder-image":{"type":"placeholder","border":{"corner_radius":6},"height":{"type":"match_parent","value":24}},"placeholder":{"type":"separator","delimiter_style":{"color":"#212123"},"background":[{"color":"#212123","type":"solid"}]}}})");

inline constexpr TStringBuf DEFAULT_VIDEO_SINGLE_CARD_PATH = "/video/quasar/videoEntity/mainPage/";
inline constexpr TStringBuf DEFAULT_VIDEO_SINGLE_CARD_SPLASH = TStringBuf(R"({"card":{"log_id":"station_rich_single","states":[{"state_id":0,"div":{"type":"body"}}]},"templates":{"body":{"type":"container","orientation":"vertical","background":[{"color":"#151517","type":"solid"}],"paddings":{"top":164,"left":80},"width":{"type":"match_parent","value":1920},"height":{"type":"fixed","value":1080},"items":[{"type":"rich_content"}]},"rich_content":{"type":"container","margins":{"top":2},"height":{"type":"match_parent"},"items":[{"type":"placeholder-text","height":{"type":"fixed","value":200},"width":{"type":"fixed","value":860}},{"type":"placeholder-text","height":{"type":"fixed","value":33},"width":{"type":"fixed","value":830},"margins":{"top":22}},{"type":"placeholder-text","height":{"type":"fixed","value":32},"width":{"type":"fixed","value":860},"margins":{"top":20}},{"type":"placeholder-text","height":{"type":"fixed","value":32},"width":{"type":"fixed","value":800},"margins":{"top":5}}]},"placeholder-image":{"type":"placeholder","border":{"corner_radius":12},"height":{"type":"match_parent"}},"placeholder-text":{"type":"placeholder","margins":{"left":0},"border":{"corner_radius":6},"height":{"type":"fixed","value":24}},"placeholder":{"type":"separator","delimiter_style":{"color":"#212123"},"background":[{"color":"#212123","type":"solid"}]}}})");

inline constexpr TStringBuf DEFAULT_VIDEO_DESCRIPTION_CARD_PATH = "/video/quasar/videoEntity/descriptionPage/";
inline constexpr TStringBuf DEFAULT_VIDEO_DESCRIPTION_CARD_SPLASH = TStringBuf(R"({\"card\": {\"log_id\": \"station_video_description\", \"states\": [{\"state_id\": 0, \"div\": {\"type\": \"body\"}}]}, \"templates\": {\"body\": {\"type\": \"container\", \"orientation\": \"vertical\", \"background\": [{\"color\": \"#151517\", \"type\": \"solid\"}], \"paddings\": {\"top\": 48, \"left\": 80}, \"width\": {\"type\": \"match_parent\", \"value\": 1920}, \"height\": {\"type\": \"fixed\", \"value\": 1080}, \"items\": [{\"type\": \"placeholder-text\", \"width\": {\"type\": \"fixed\", \"value\": 340}, \"height\": {\"type\": \"fixed\", \"value\": 40}, \"margins\": {\"bottom\": 140}}, {\"type\": \"description_content\"}]}, \"description_content\": {\"type\": \"container\", \"paddings\": {\"top\": 8}, \"orientation\": \"horizontal\", \"height\": {\"type\": \"match_parent\"}, \"items\": [{\"type\": \"description_left\"}, {\"type\": \"description_right\"}]}, \"description_left\": {\"type\": \"container\", \"orientation\": \"vertical\", \"width\": {\"type\": \"fixed\", \"value\": 1060}, \"margins\": {\"right\": 140}, \"items\": [{\"type\": \"placeholder-text\", \"width\": {\"type\": \"fixed\", \"value\": 1050}}, {\"type\": \"placeholder-text\", \"width\": {\"type\": \"fixed\", \"value\": 1060}}, {\"type\": \"placeholder-text\", \"width\": {\"type\": \"fixed\", \"value\": 1020}}, {\"type\": \"placeholder-text\", \"width\": {\"type\": \"fixed\", \"value\": 1060}}, {\"type\": \"placeholder-text\", \"width\": {\"type\": \"fixed\", \"value\": 1000}}]}, \"description_right\": {\"type\": \"container\", \"orientation\": \"vertical\", \"items\": [{\"type\": \"directors\", \"margins\": {\"bottom\": 60}}, {\"type\": \"directors\"}]}, \"directors\": {\"type\": \"container\", \"items\": [{\"type\": \"placeholder-text\", \"width\": {\"type\": \"fixed\", \"value\": 155}, \"margins\": {\"bottom\": 15}}, {\"type\": \"placeholder-text\", \"width\": {\"type\": \"fixed\", \"value\": 490}}]}, \"placeholder-text\": {\"type\": \"placeholder\", \"border\": {\"corner_radius\": 2}, \"height\": {\"type\": \"fixed\", \"value\": 28}, \"margins\": {\"bottom\": 10}}, \"placeholder\": {\"type\": \"separator\", \"delimiter_style\": {\"color\": \"#212123\"}, \"background\": [{\"color\": \"#212123\", \"type\": \"solid\"}]}}})");

inline constexpr TStringBuf DEFAULT_VIDEO_SEASONS_GALLERY_PATH = "/video/quasar/videoEntity/seasons/";
inline constexpr TStringBuf DEFAULT_VIDEO_SEASONS_GALLERY_SPLASH = TStringBuf(R"({"card": {"log_id": "station_rich_series", "states": [{"state_id": 0, "div": {"type": "body"}}]}, "templates": {"body": {"type": "container", "orientation": "vertical", "background": [{"color": "#151517", "type": "solid"}], "paddings": {"top": 140, "left": 80}, "width": {"type": "match_parent", "value": 1920}, "height": {"type": "fixed", "value": 1080}, "items": [{"type": "tab"}, {"type": "cards"}, {"type": "cards"}]}, "tab": {"type": "placeholder-image", "height": {"type": "fixed", "value": 56}, "width": {"type": "fixed", "value": 150}, "margins": {"bottom": 47}}, "card": {"type": "container", "width": {"type": "fixed", "value": 450}, "items": [{"type": "placeholder-image", "height": {"type": "fixed", "value": 230}, "width": {"type": "fixed", "value": 410}}, {"type": "placeholder-text", "height": {"type": "fixed", "value": 32}, "width": {"type": "fixed", "value": 300}, "margins": {"top": 18}}]}, "cards": {"type": "container", "orientation": "horizontal", "height": {"type": "fixed", "value": 346}, "margins": {"bottom": 20}, "items": [{"type": "card"}, {"type": "card"}, {"type": "card"}, {"type": "card"}, {"type": "card"}]}, "placeholder-image": {"type": "placeholder", "border": {"corner_radius": 12}, "height": {"type": "match_parent"}}, "placeholder-text": {"type": "placeholder", "margins": {"left": 0}, "border": {"corner_radius": 6}, "height": {"type": "fixed", "value": 24}}, "placeholder": {"type": "separator", "delimiter_style": {"color": "#212123"}, "background": [{"color": "#212123", "type": "solid"}]}}})");

inline constexpr TStringBuf DEFAULT_CHANNELS_PATH = "/video/quasar/channels/0/";
inline constexpr TStringBuf DEFAULT_CHANNELS_SPLASH = TStringBuf(R"({"card": { "log_id": "channels", "states": [ { "state_id": 0, "div": { "type": "body" } } ] }, "templates": { "body": { "type": "container", "orientation": "vertical", "background": [ { "color": "#151517", "type": "solid" } ], "paddings": { "top": 160, "left": 80 }, "width": { "type": "match_parent", "value": 1920 }, "height": { "type": "fixed", "value": 1080 }, "items": [ { "type": "cards_container", "margins": { "bottom": 260 } } ] }, "cards_container": { "type": "container", "orientation": "vertical", "items": [ { "type": "cards", "margins": { "bottom": 186 } }, { "type": "cards" } ] }, "cards": { "type": "container", "orientation": "horizontal", "height": { "type": "match_parent", "value": 540 }, "items": [ { "type": "card" }, { "type": "card" }, { "type": "card" }, { "type": "card" }, { "type": "card" }, { "type": "card" } ] }, "card": { "type": "container", "margins": { "right": 40 }, "width": { "type": "fixed", "value": 410 }, "height": { "type": "fixed", "value": 234 }, "items": [ { "type": "placeholder-image" } ] }, "placeholder-image": { "type": "placeholder", "border": { "corner_radius": 16 }, "height": { "type": "match_parent", "value": 24 } }, "placeholder": { "type": "separator", "delimiter_style": { "color": "#212123" }, "background": [ { "color": "#212123", "type": "solid" } ] } } })");

inline constexpr TStringBuf DEFAULT_PROMO_NY_PATH = "/video/quasar/collection/0/";
inline constexpr TStringBuf DEFAULT_PROMO_NY_SPLASH = TStringBuf(R"({"card":{"log_id":"station_home_carousel","states":[{"state_id":0,"div":{"type":"body"}}]},"templates":{"body":{"type":"container","orientation":"vertical","background":[{"color":"#151517","type":"solid"}],"paddings":{"top":346,"left":80},"width":{"type":"match_parent","value":1920},"height":{"type":"fixed","value":1080},"items":[{"type":"cards"}]},"cards":{"type":"container","orientation":"horizontal","height":{"type":"fixed","value":474},"margins":{"bottom":245},"items":[{"type":"card"},{"type":"card"},{"type":"card"},{"type":"card"},{"type":"card"}]},"card":{"type":"container","margins":{"right":40},"width":{"type":"fixed","value":320},"height":{"type":"match_parent"},"items":[{"type":"placeholder-image"}]},"placeholder-image":{"type":"placeholder","border":{"corner_radius":6},"height":{"type":"match_parent","value":24}},"placeholder":{"type":"separator","delimiter_style":{"color":"#212123"},"background":[{"color":"#212123","type":"solid"}]}}})");


inline constexpr TStringBuf DEFAULT_MORDOVIA_MAIN_SCREEN_SPLASH = TStringBuf(R"({"card":{"log_id":"station_informers","states":[{"state_id":0,"div":{"type":"body"}}]},"templates":{"body":{"type":"container","orientation":"horizontal","background":[{"color":"#151517","type":"solid"}],"paddings":{"top":140,"left":80},"width":{"type":"match_parent","value":1920},"height":{"type":"fixed","value":1080},"items":[{"type":"container","width":{"type":"fixed","value":1860},"items":[{"type":"cards-big"},{"type":"cards-small","margins":{"top":40}}]}]},"cards-big":{"type":"container","orientation":"horizontal","items":[{"type":"card-big"},{"type":"card-big"},{"type":"card-big"}]},"card-big":{"type":"container","margins":{"right":40},"width":{"type":"fixed","value":560},"height":{"type":"fixed","value":410},"items":[{"type":"placeholder-image","height":{"type":"fixed","value":315}},{"type":"placeholder-text","width":{"type":"fixed","value":500},"height":{"type":"fixed","value":32},"margins":{"top":20}},{"type":"placeholder-text","width":{"type":"fixed","value":310},"height":{"type":"fixed","value":28},"margins":{"top":14}}]},"cards-small":{"type":"container","orientation":"horizontal","items":[{"type":"card"},{"type":"card"},{"type":"card"},{"type":"card"}]},"card":{"type":"container","margins":{"right":40},"width":{"type":"fixed","value":410},"height":{"type":"fixed","value":400},"items":[{"type":"placeholder-image","height":{"type":"fixed","value":234}},{"type":"placeholder-text","width":{"type":"fixed","value":286},"height":{"type":"fixed","value":32},"margins":{"top":22}},{"type":"placeholder-text","width":{"type":"fixed","value":190},"height":{"type":"fixed","value":28},"margins":{"top":10}}]},"placeholder-image":{"type":"placeholder","border":{"corner_radius":6},"height":{"type":"match_parent"}},"placeholder-text":{"type":"placeholder","margins":{"top":24},"border":{"corner_radius":4},"height":{"type":"fixed","value":24}},"placeholder":{"type":"separator","delimiter_style":{"color":"#212123"},"background":[{"color":"#212123","type":"solid"}]}}})");

// view_key id for all our video-related webview screens (i.e. main screen (Mordovia), video search screen, rich card screen etc.)
inline constexpr TStringBuf VIDEO_STATION_SPA_MAIN_VIEW_KEY = TStringBuf("VideoStationSPA:main");
inline constexpr TStringBuf VIDEO_STATION_SPA_VIDEO_VIEW_KEY = TStringBuf("VideoStationSPA:video");

} // namespace NAlice::NVideoCommon
