from alice.analytics.utils.datetime_utils import calc_business_time_duration


def test_calc_working_hours_duration():
    # dt: 2020-12-01 00:44:09+03:00 2020-12-01 02:34:58+03:00 => 0.00h
    assert calc_business_time_duration(1606772649, 1606779298) == 0
    # dt: 2020-11-30 23:41:28+03:00 2020-12-01 02:34:58+03:00 => 0.00h
    assert calc_business_time_duration(1606768888, 1606779298) == 0
    # dt: 2020-10-30 13:34:11+03:00 2020-10-31 21:28:21+03:00 => 20.90h
    assert calc_business_time_duration(1604054051, 1604168901) == 75250
    # dt: 2020-09-15 10:32:46+03:00 2020-11-06 13:50:42+03:00 => 679.30h
    assert calc_business_time_duration(1600155166, 1604659842) == 2445476
    # dt: 2020-10-30 13:36:00+03:00 2020-11-02 09:43:58+03:00 => 35.40h
    assert calc_business_time_duration(1604054160, 1604299438) == 127440
    # dt: 2020-10-30 17:07:15+03:00 2020-10-31 19:43:06+03:00 => 15.60h
    assert calc_business_time_duration(1604066835, 1604162586) == 56151
    # dt: 2020-10-30 17:31:16+03:00 2020-11-02 08:21:57+03:00 => 31.48h
    assert calc_business_time_duration(1604068276, 1604294517) == 113324
    # dt: 2020-10-30 18:42:50+03:00 2020-10-31 11:41:55+03:00 => 5.98h
    assert calc_business_time_duration(1604072570, 1604133715) == 21545
    # dt: 2020-10-30 19:09:37+03:00 2020-10-31 23:39:58+03:00 => 16.84h
    assert calc_business_time_duration(1604074177, 1604176798) == 60623
    # dt: 2020-10-30 19:18:31+03:00 2020-11-02 10:39:00+03:00 => 30.34h
    assert calc_business_time_duration(1604074711, 1604302740) == 109229
    # dt: 2020-10-30 19:15:43+03:00 2020-11-02 12:07:04+03:00 => 31.86h
    assert calc_business_time_duration(1604074543, 1604308024) == 114681
    # dt: 2020-10-30 19:55:06+03:00 2020-10-31 17:36:55+03:00 => 10.70h
    assert calc_business_time_duration(1604076906, 1604155015) == 38509
