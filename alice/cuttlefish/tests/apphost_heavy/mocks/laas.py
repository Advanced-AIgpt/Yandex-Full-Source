import json


class Laas:
    backend_name = "LAAS__VOICE"

    @classmethod
    def auto(cls, name, request):
        return {
            "code": 200,
            "reason": "OK",
            "headers": [["Content-Type", "application/json"]],
            "body": json.dumps(
                {
                    "region_id": 120542,
                    "precision": 2,
                    "latitude": 55.734840,
                    "longitude": 37.585976,
                    "should_update_cookie": False,
                    "is_user_choice": False,
                    "suspected_region_id": 120542,
                    "city_id": 213,
                    "region_by_ip": 213,
                    "suspected_region_city": 213,
                    "location_accuracy": 140,
                    "location_unixtime": 1613404161,
                    "suspected_latitude": 55.734840,
                    "suspected_longitude": 37.585976,
                    "suspected_location_accuracy": 140,
                    "suspected_location_unixtime": 1613404161,
                    "suspected_precision": 2,
                    "region_home": 213,
                    "probable_regions_reliability": 1.00,
                    "probable_regions": [{"region_id": 213, "weight": 1.000000}],
                    "regular_coordinates": [
                        {"lat": 55.734411, "lon": 37.586803, "type": 2},
                        {"lat": 55.610085, "lon": 37.702949, "type": 1},
                        {"lat": 55.732757, "lon": 37.588554, "type": 0},
                    ],
                    "country_id_by_ip": 225,
                    "is_anonymous_vpn": False,
                    "is_public_proxy": False,
                    "is_serp_trusted_net": False,
                    "is_tor": False,
                    "is_hosting": False,
                    "is_gdpr": False,
                    "is_mobile": False,
                    "is_yandex_net": True,
                    "is_yandex_staff": False,
                }
            ),
        }
