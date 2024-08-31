from alice.tests.library.uniclient import Location


class Region(object):
    def __init__(self, lat=None, lon=None, timezone=None, region_id=None, client_ip=None, **kwargs):
        assert (lat and lon) or (not lat and not lon), f'You must define both lat, lon for Location: {lat}, {lon}'
        self.location = Location(lat=lat, lon=lon, **kwargs)
        self.timezone = timezone or 'Europe/Moscow'
        self.client_ip = client_ip
        self._region_id = region_id

    @property
    def region_id(self):
        return self._region_id


class _Region(Region):

    user_defined_region_id = False

    @property
    def region_id(self):
        if self.user_defined_region_id:
            return self._region_id
        return None


Amsterdam = _Region(lat=52.372518, lon=4.896977, timezone='Europe/Amsterdam', region_id=10466)
Baku = _Region(lat=40.3899498, lon=49.83001328, timezone='Asia/Baku', client_ip='5.191.19.110', region_id=10253)
Berlin = _Region(lat=52.513505, lon=13.38175, timezone='Europe/Berlin', region_id=177)
Istanbul = _Region(lat=41.011218, lon=28.978169, timezone='Europe/Istanbul', region_id=11508)
Madrid = _Region(lat=39.464109, lon=-0.375711, timezone='Europe/Madrid', client_ip='178.237.239.61', region_id=10430)  # Valencia
Minsk = _Region(lat=53.902496, lon=27.561481, timezone='Europe/Minsk', region_id=157)
Moscow = _Region(lat=55.733771, lon=37.587937, timezone='Europe/Moscow', client_ip='77.88.55.77', region_id=213)
StPetersburg = _Region(lat=59.918077, lon=30.304899, timezone='Europe/Moscow', region_id=2)
Tashkent = _Region(lat=41.31287766, lon=69.31058502, timezone='Asia/Tashkent', client_ip='213.230.74.251', region_id=10335)
Vilnius = _Region(lat=54.689388, lon=25.270894, timezone='Europe/Vilnius', region_id=11475)
Undefined = _Region(lat=None, lon=None)
ZeroLocation = _Region(lat=0.0, lon=0.0)
