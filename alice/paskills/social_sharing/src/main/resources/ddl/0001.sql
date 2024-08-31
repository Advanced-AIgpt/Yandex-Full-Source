--!syntax_v1
create table documents(
    key_hash Uint64
    , key Utf8
    , opengraph_title Utf8
    , opengraph_type Utf8
    , opengraph_url Utf8
    , opengraph_description Utf8
    , opengraph_image_url Utf8
    , opengraph_image_type Utf8
    , opengraph_image_width Uint32
    , opengraph_image_height Uint32
    , opengraph_image_alt Utf8
    , required_features Utf8
    , template_type UInt32
    , template_data String
    , action_directive String
    , created_at Timestamp
    , PRIMARY KEY(key_hash, key)
) WITH (
    AUTO_PARTITIONING_BY_SIZE = ENABLED,
    UNIFORM_PARTITIONS = 50
);

create table document_candidates(
    key_hash Uint64
    , key Utf8
    , opengraph_title Utf8
    , opengraph_type Utf8
    , opengraph_url Utf8
    , opengraph_description Utf8
    , opengraph_image_url Utf8
    , opengraph_image_type Utf8
    , opengraph_image_width Uint32
    , opengraph_image_height Uint32
    , opengraph_image_alt Utf8
    , required_features Utf8
    , template_type UInt32
    , template_data String
    , action_directive String
    , created_at Timestamp
    , PRIMARY KEY(key_hash, key)
) WITH (
    AUTO_PARTITIONING_BY_SIZE = ENABLED,
    UNIFORM_PARTITIONS = 50,
    TTL = Interval("P7D") ON created_at
);

create table images(
    url_hash UInt64
    , url Utf8
    , group_id UInt64
    , image_name Utf8
    , avatars_response Utf8
    , created_at Timestamp
    , PRIMARY KEY (url_hash, url)
) WITH (
    AUTO_PARTITIONING_BY_SIZE = ENABLED,
    UNIFORM_PARTITIONS = 50
);
