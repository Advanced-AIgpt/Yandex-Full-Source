from alice.uniproxy.library.backends_laas import _mac_for_laas, _wifinets_to_arg


def test_mac_for_laas():
    in_data = [
        '10:7b:ef:59:54:3c', '9c:6f:52:c0:0f:42', '24:7e:51:87:fa:32', '64:d1:54:a5:7f:07',
        '60:ce:86:92:b1:90', '42:23:43:b0:eb:1e', 'd0:17:c2:35:2a:28', 'f8:c0:91:12:36:34',
        '92:48:9a:17:3f:34', '04:bf:6d:42:66:c4', 'b0:48:7a:f9:22:45', 'e4:a7:c5:e2:ac:58',
        '64:6e:ea:df:64:ec', '38:d8:2f:f1:ee:88', '2c:fd:a1:0e:f2:f8', '1c:bd:b9:b6:1e:64',
        'e4:ca:12:9a:1c:fd', '52:ff:20:49:73:a4', 'b0:6e:bf:77:3d:f0', '10:bf:48:e6:11:ce',
    ]

    expected_out = [
        '107bef59543c', '9c6f52c00f42', '247e5187fa32', '64d154a57f07',
        '60ce8692b190', '422343b0eb1e', 'd017c2352a28', 'f8c091123634',
        '92489a173f34', '04bf6d4266c4', 'b0487af92245', 'e4a7c5e2ac58',
        '646eeadf64ec', '38d82ff1ee88', '2cfda10ef2f8', '1cbdb9b61e64',
        'e4ca129a1cfd', '52ff204973a4', 'b06ebf773df0', '10bf48e611ce',
    ]

    real_out = list([_mac_for_laas(mac) for mac in in_data])

    assert expected_out == real_out


def test_wifinets_to_arg():
    wifinets = [
        {"mac": "10:7b:ef:59:54:3c", "signal_strength": -56},
        {"mac": "9c:6f:52:c0:0f:42", "signal_strength": -57},
        {"mac": "24:7e:51:87:fa:32", "signal_strength": -63},
        {"mac": "64:d1:54:a5:7f:07", "signal_strength": -64},
    ]

    expected_out = '&wifinetworks=107bef59543c:-56,9c6f52c00f42:-57,247e5187fa32:-63,64d154a57f07:-64'

    ok, real_out = _wifinets_to_arg(wifinets)
    assert ok
    assert real_out == expected_out
