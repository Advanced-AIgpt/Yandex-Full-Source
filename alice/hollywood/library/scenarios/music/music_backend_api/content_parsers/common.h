#pragma once

namespace NAlice::NHollywood::NMusic {

constexpr TStringBuf ARTISTS = "artists";
constexpr TStringBuf CHILD_CONTENT = "childContent";
constexpr TStringBuf NAME = "name";
constexpr TStringBuf TITLE = "title";
constexpr TStringBuf TOTAL_PATH = "pager.total";
constexpr TStringBuf TRACK = "track";
constexpr TStringBuf TRACKS = "tracks";

TMaybe<NData::NMusic::TContentInfo> TryConstructContentInfo(const NJson::TJsonValue& resultJson);

} // namespace NAlice::NHollywood::NMusic
