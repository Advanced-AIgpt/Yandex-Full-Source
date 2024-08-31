def get_location_by_coordinates():
    return """
        IF (
            $p0 IS NOT NULL AND {lat} IS NOT NULL AND {lon} IS NOT NULL,
            Geo::RoundRegionByLocation({lat}, {lon}, "town").name
            || ", " ||
            Geo::RoundRegionByLocation({lat}, {lon}, "region").name,
            NULL
        )
    """.format(
        lat='Yson::LookupDouble($p0, "lat", Yson::Options(false as Strict))',
        lon='Yson::LookupDouble($p0, "lon", Yson::Options(false as Strict))',
    )


def get_location_by_region_id():
    return """
        IF(
            $p0 IS NOT NULL,
            Geo::RoundRegionById($p0, "town").name
            || ", " ||
            Geo::RoundRegionById($p0, "region").name,
            NULL
        )
    """


def get_location_by_client_ip():
    return """
        IF(
            $p0 IS NOT NULL,
            Geo::RoundRegionByIp($p0, "town").name
            || ", " ||
            Geo::RoundRegionByIp($p0, "region").name,
            NULL
        )
    """
