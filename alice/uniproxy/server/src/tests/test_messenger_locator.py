import logging
import uuid

import alice.uniproxy.library.backends_memcached as memcached
import messenger.locator2
import messenger.locator

import alice.uniproxy.library.testing
from alice.uniproxy.library.utils.timestamp import PerformanceCounter

from messenger.protos.uniproxy_pb2 import TLocationEntry



logging.basicConfig(
    filename='test_messenger_locator.log',
    level=logging.DEBUG,
    format='%(asctime)s %(levelname)12s: %(name)-16s %(message)s'
)


def test_encode_hostname():
    test_cases = [
        ('uniproxy-1.uniproxy.dev.uniproxy.voice-ext.stable.qloud-d.yandex.net', 1, 'uniproxy', 'dev', None),
        ('be-uniproxy-11.be-uniproxy.dev.uniproxy.voice-ext.stable.qloud-d.yandex.net', 11, 'be-uniproxy', 'dev', None),
        ('be-uniproxy-3.be-uniproxy.test.uniproxy.voice-ext.stable.qloud-d.yandex.net', 3, 'be-uniproxy', 'test', None),
        ('man1-6921-man-uniproxy-20233.gencfg-c.yandex.net', None, None, None, 'man1-6921-man-uniproxy-20233.gencfg-c.yandex.net'),
    ]

    entry = messenger.locator2.ClientEntry()

    for test_case in test_cases:
        host_no, component, environment, custom_hostname = entry.encoded_hostname(test_case[0])
        assert host_no == test_case[1]
        assert component == test_case[2]
        assert environment == test_case[3]
        assert custom_hostname == test_case[4]


def test_decode_hostname_normal():
    entry = messenger.locator2.ClientEntry()
    location = TLocationEntry()
    location.HostNo = 42
    assert entry.decoded_hostname(location) == 'be-uniproxy-42.be-uniproxy.dev.uniproxy.voice-ext.stable.qloud-d.yandex.net'


def test_decode_hostname_custom_environment():
    entry = messenger.locator2.ClientEntry()
    location = TLocationEntry()
    location.HostNo = 42
    location.Environment = 'custom'
    assert entry.decoded_hostname(location) == 'be-uniproxy-42.be-uniproxy.custom.uniproxy.voice-ext.stable.qloud-d.yandex.net'


def test_decode_hostname_custom_component():
    entry = messenger.locator2.ClientEntry()
    location = TLocationEntry()
    location.HostNo = 42
    location.Component = 'uniqproxy'
    assert entry.decoded_hostname(location) == 'uniqproxy-42.uniqproxy.dev.uniproxy.voice-ext.stable.qloud-d.yandex.net'


def test_decode_hostname_custom_hostname():
    entry = messenger.locator2.ClientEntry()
    location = TLocationEntry()
    location.CustomHost = 'man1-6921-man-uniproxy-20233.gencfg-c.yandex.net'
    assert entry.decoded_hostname(location) == 'man1-6921-man-uniproxy-20233.gencfg-c.yandex.net'


@alice.uniproxy.library.testing.ioloop_run
def _test_update_resolve():
    yield memcached.memcached_init_async()
    memcached = memcached.memcached_client(memcached.MEMCACHED_MSSNGR_SERVICE)
    locator = messenger.locator2.ClientLocator()

    guid = '70f60162-daf3-40a7-a2b1-2e9edb95bb65'
    client_id = 'd43d1b77-a593-45c0-a8de-ce291608260f'
    client_id2 = 'b80aa9b4-b891-4250-affc-8fb25bea5637'

    yield memcached.xdelete(guid)

    p, s = yield locator.update_location(guid, client_id)
    assert p and s

    p, s = yield locator.update_location(guid, client_id2)
    assert p and s

    entries = yield locator.resolve_locations([guid])
    assert len(entries) == 1
    assert guid in entries

    for _, entry in entries.items():
        locations = list(entry.enumerate_locations())
        assert len(locations) == 2

        for uuid, cid, location in locations:
            assert cid in [client_id, client_id2]


@alice.uniproxy.library.testing.ioloop_run
def _test_update_resolve_old():
    guid = '70f60162-daf3-40a7-a2b1-2e9edb95bb65'
    client_id = 'd43d1b77-a593-45c0-a8de-ce291608260f'
    client_id2 = 'b80aa9b4-b891-4250-affc-8fb25bea5637'

    yield memcached.memcached_init_async()
    memcached = memcached.memcached_client(memcached.MEMCACHED_MSSNGR_SERVICE)
    locator = messenger.locator2.ClientLocator()

    guid = '70f60162-daf3-40a7-a2b1-2e9edb95bb65'
    client_id = 'd43d1b77-a593-45c0-a8de-ce291608260f'
    client_id2 = 'b80aa9b4-b891-4250-affc-8fb25bea5637'

    yield [
        memcached.xdelete(guid),
        memcached.xdelete(client_id),
        memcached.xdelete(client_id2)
    ]

    yield [
        memcached.xset(guid, '{};{}'.format(client_id, client_id2)),
        memcached.xset(client_id, 'lyalchenko-dell.dev.yandex.net'),
        memcached.xset(client_id2, 'lyalchenko-dell.dev.yandex.net')
    ]

    entries = yield locator.resolve_locations([guid])
    assert len(entries) == 1
    assert guid in entries

    for _, entry in entries.items():
        locations = list(entry.enumerate_locations())
        assert len(locations) == 2

        for uuid, cid, location in locations:
            assert uuid in [client_id, client_id2]


@alice.uniproxy.library.testing.ioloop_run
def _test_update_and_resolve_many_client_ids():
    yield memcached.memcached_init_async()
    memcached = memcached.memcached_client(MEMCACHED_MSSNGR_SERVICE)
    locator = messenger.locator2.ClientLocator()

    guid = '70f60162-daf3-40a7-a2b1-2e9edb95bb65'
    client_ids = [
        str(uuid.uuid4()) for i in range(0, 50)
    ]

    yield memcached.xdelete(guid)

    futures = [
        locator.update_location(guid, client_id) for client_id in client_ids
    ]

    results = yield futures
    results = map(lambda x: x[0] or x[1], results)
    assert all(results)

    entries = yield locator.resolve_locations([guid])
    assert len(entries) == 1
    assert guid in entries

    for _, entry in entries.items():
        locations = list(entry.enumerate_locations())
        assert len(locations) == len(client_ids)

        for _, cid, location in locations:
            assert cid in client_ids

        cids = [x[1] for x in locations]
        for cid in client_ids:
            assert cid in cids


@alice.uniproxy.library.testing.ioloop_run
def _test_update_and_resolve_many_guids():
    yield memcached.memcached_init_async()
    memcached = memcached.memcached_client(memcached.MEMCACHED_MSSNGR_SERVICE)
    locator = messenger.locator2.ClientLocator()

    guids = [
        '2d543c08-cfb3-4b70-9227-cfb4e8d472d0', 'b99be7ac-ee2d-46be-8a88-9e526765c552',
        '20234b23-270c-49d5-9fd5-14fe35800a9b', '51bf01b0-e6b4-4952-904a-445b77a35203',
        'aee62548-3767-42dc-ac70-2c9c32cbb637', 'ecf21b6b-6188-4420-b278-c74e3064dcae',
        '1cdca7d3-1866-46a7-9dea-39eadcae067e', '7528190c-fd55-409b-b21d-0437c5061812',
        '9901624a-db03-4049-8280-76964ff915a1', '32ec9dc8-05eb-4719-abdd-54449ed8465a',
        '72052f2b-13e0-4f1a-bbe3-fa720edd6223', '2c94c94b-d283-4089-83ec-71cd61cdca22',
        'd8a09404-33eb-475b-873f-39deaa80dd90', '93ca2dc7-dae1-47e5-92b3-1844c5077eaf',
        'f81ee34d-963c-4cef-a531-dc59d1b7d130', '86fc541a-48f0-4947-833c-55e7c8df47e0',
        '3bb9d906-4d46-45b3-b933-353e4acf7934', 'b67cf707-d8e2-4684-911e-c94a3d96ecb4',
        '0102d3db-089e-4741-b50c-126c701acba8', '1ff4e57a-dec8-4db8-afa9-9794f23e7f80',
        '67573126-a4fe-4f09-89c5-a1b491162620', '62468aef-eece-4c5c-8ddf-bade03241ff3',
        '0cfde929-38e9-4839-8720-ff065b967866', '5c3fadb7-62c7-4f78-b09f-e8a62cf027a1',
        '88f1ffc5-0af4-4e23-a81a-163e085ef158', 'eb601792-b199-4be0-ac55-5a271ebe4c93',
        '916a7003-1ee7-4a40-bff6-0ee7797f8f9d', 'de6a3a68-34f4-4b6c-8dbf-8bfcaed63cba',
        '031bdd0a-d3c7-4f46-a889-52393bed8487', 'bf2bb49c-050b-4dac-a808-4adfdf5a4f58',
        '3f011d42-9ef3-4417-be85-28a4d4c7c932', '2be10a85-10ed-45dc-86c4-28326683685e',
        '724b6b8c-bf59-4bfb-9b46-14d892a6da99', '906bbbf8-f54e-43f6-a2fd-46d6e5848ffd',
        '610fec5b-132f-4a5b-be5a-6b157ea41a82', 'c36c81f6-0132-4ef4-a68a-35d2113b59ea',
        '2a409023-5c25-4245-9422-401dc79c8fc7', '8975bd84-db02-40a9-8023-27d349846192',
        'b8f0a50d-aca0-4870-bf92-3b9f85c9025a', '734d41e4-514a-415f-b295-713c0c7a166b',
        '72d3d482-c4d4-43ae-87a7-f89522a0dcd3', '01afb009-61c0-482d-b785-462dd069981e',
        '4ed8d0a1-d656-417f-a6e1-3d8b726f0e3d', 'e09ac498-78a7-44b0-8dff-e0842649dd78',
        '8c73b772-926a-4c5c-9927-7d53643140a5', '0dde4966-d06c-491a-a031-33b877e02116',
        '7cc496c7-b1fb-492d-8ee0-7f1e346dd04e', 'd916d8c4-a18a-4092-863a-2efb32597bc3',
        '02531467-c5f9-4f6d-9cbf-68c05fa0dd15', 'aebfd952-5f4b-4f19-bc15-4004c00ce72b',
    ]

    client_ids = [
        str(uuid.uuid4()) for x in range(0, len(guids))
    ]

    # ................................................................................................................
    #   delete all data
    yield [
        memcached.xdelete(guid) for guid in guids
    ]

    # ................................................................................................................
    #   update locations data
    futures = [
        locator.update_location(guid, client_id) for guid, client_id in zip(guids, client_ids)
    ]

    results = yield futures
    results = map(lambda x: x[0] or x[1], results)
    assert all(results)

    entries = yield locator.resolve_locations(guids)
    assert len(entries) == len(guids)

    for guid, client_id in zip(guids, client_ids):
        assert guid in entries
        for _, cid, location in entries[guid].enumerate_locations():
            assert cid == client_id
            break


@alice.uniproxy.library.testing.ioloop_run
def test_update_and_resolve_many_guids_many_clients():
    yield memcached.memcached_init_async()
    memcached = memcached.memcached_client(memcached.MEMCACHED_DELIVERY_SERVICE)
    locator = messenger.locator2.ClientLocator(memcached.MEMCACHED_DELIVERY_SERVICE)

    guids = [
        '16b73702-0f0d-405d-a5b8-3d47ac1ead79', '226e7e46-8281-4176-ad53-98bbd6bffa89',
        '37a4b60c-5485-437a-b1db-d413d44e37ec', 'f654e291-b3f4-4478-a878-b9f370fac2a6',
        '5d9bf837-63f4-4098-8627-f7a7a5cd112f', 'fcc9861b-3163-4977-b5b4-9ae19adf3448',
        'f19c9155-39a6-4bb7-ab15-61dc366bebf0', '813cbb18-9044-4fa2-ac8b-23369df6ce8f',
        '67baa54a-2c56-43fc-bf17-e1c8a0516f2c', '94f34ca5-db5a-4b7b-b115-56bfeb80f860',
        '47737382-29f5-4dd9-9401-7798490bd499', '10e843fe-e2b1-4d21-9c42-ded03c22e3fe',
        'c8b155a5-3e19-45cc-95d8-f0b823422fd3', '57a40d16-2bbf-4e3d-aaf0-36f16b84d68a',
        '7cd3ed3f-04f6-48e5-b895-abf45305356c', 'd5b715aa-dbb5-4ce5-848b-e7ff222e11dc',
        'bb8fc28a-d5ba-4226-aa79-6b15e03719bb', '5cb94be7-c904-413f-a590-54b7b974af19',
        'cc57a26d-07a0-469a-aa86-43dc72c336bd', '2bd596f4-ddc7-4b5d-a48a-b78fd45d8546',
        'cf597b4e-2ea9-4ff3-b5e9-15cbf2b70937', '9e8ee664-b38e-424d-a692-f6d7613565ce',
        '14418d73-63c0-4300-ae62-e62cabed1935', 'be6580c2-165c-463c-827e-ab964ffa15bb',
        '9cfba49b-f4ff-4774-86e5-b1cc12cca3b8', '1ae60a85-7c33-4b30-ae59-47a437c00165',
        'ba50db99-2b3a-4ff6-adaa-5ec45c9e3bd6', '8d68bb6b-ef15-429a-a0cb-297bb06d9359',
        'cb038e5b-1102-4eba-be56-194cfb98cc88', 'fdb26644-2ec1-42c8-9da3-e905bd126eef',
        '84acf2fa-5137-4b8c-a727-3037ffd1defd', 'f70a53fb-2590-40b2-b2f2-2013105a1f5c',
        'd7797b36-394e-48b5-a860-f0352f5261fa', '324d2199-d40b-457d-b40d-cdf111b800db',
        '826fbcf5-011e-4a53-bf7b-ca1c158e3a99', '8c55d73f-1bc8-407f-b024-274c59ef5721',
        '21505222-95a9-4ed5-83cc-290caba529e9', 'ade2f842-4520-4f12-b87d-b9acb3de7983',
        'f3b6021a-d034-4698-9bf8-c08cbabad18b', '33813a95-24a6-498a-bfb9-63950a87f2cb',
        '8e75f368-82a0-45dc-82af-7060d8ce8dbe', '0ec67a26-ebee-44bd-85b1-e81da0d5107e',
        '69b07bd6-f370-44fc-8433-73785212fdb6', '8dfc2287-6eb2-4c3e-8c18-0e5aa379c547',
        '138d6409-ee41-4135-9a5b-7b4d9066c607', '4555f11f-22b2-4acc-8e9b-90d71fd983a5',
        'e9f3bdd3-9d69-4dd7-a8eb-72aa82577fe5', '6a9ce50e-7fab-4b3f-9e96-da932e4a39f6',
        'a2da7f01-c0f0-42b5-aba0-37091e06a4e6', '978b57e1-1615-4f11-b3aa-64de5ab66b0a',
        'fbb1f94c-1e38-4317-a8cb-5146764d3966', '52d283b1-ac9b-4dc0-8d97-7b84604077cd',
        '78ecdd85-a216-45c6-a6c6-41f59587f337', '2b49bcae-1db2-4bda-ac4b-680f34dd48f5',
        '6f9b1e45-79e0-4324-b753-a47596359bba', '0bb6f05f-d84e-4fcd-8551-007f78c32114',
        'a648b733-3d98-48da-aca9-bc5f250886fb', 'd8ded7ea-ad29-4907-a8c6-97f862a7d8bc',
        'af6511f2-049e-4b84-9a91-4df97c287333', '90107b99-60a8-405b-8d53-5bb8f6f19cc2',
        'b2bac782-e9de-4f58-aed3-4d16c1d17c6e', '9997dbbd-d394-48a8-ae92-1de3bffd400e',
        '573e66c6-426f-4a7f-9b35-b45cb5112b1c', '5460623a-1184-4ada-add3-09015abd6333',
        '9a7fb8c0-32a4-44a6-93a2-785ff9be0166', '3dfaf8f1-7b8e-49de-b35f-cdeacfdefa7b',
        '6545e54f-3b75-4edb-a0ca-87af0f725dab', '48816d1d-3093-41a5-b951-2918d611ee45',
        '0c212474-cdc5-42a5-8900-2b940ceb6f54', 'bdc4d636-4fb2-4682-afb9-9a6dc412eb51',
        'd035e00b-371b-4b26-806e-ea3d8285c507', 'e83c94be-6c34-4121-a2b7-03cdfc13132b',
        '0b234ada-6dab-4a5a-aafe-d03e005dad31', '093bb91e-0cc9-42d4-b000-be40819cecfd',
        'afe3f12c-5b63-487c-b577-3995fce8d7c6', 'a6cdfdf8-bc94-4a5c-8914-f35c15e98f1c',
        'b1f06e67-6852-425b-a64f-4ac02e494e0e', '458b3726-5ca5-4554-b244-443e9d4d200e',
        '7383ff44-0dc5-4478-b04b-d9d9a0b815fc', 'bfe0e0dd-cdef-4f37-a75b-3672ed4a61eb',
        '4a09c921-1241-4cc9-b097-927723a60048', 'ba593fc1-9412-47d5-bd9e-3c48cf209f2b',
        'c005434b-fd45-4c1f-bc5a-952d718dba78', 'f2f0e8fc-c085-44a3-82a9-0e11dfd08b6c',
        '500beb56-25e8-4ea9-bb54-e4d6b303bd58', 'f3142616-7d92-4cf2-b889-bcabf3863d05',
        '60724464-37a3-40b1-85f8-0e1fd21afc9e', 'd85ef1c5-7f48-4af4-87c9-95ff2e81a1f6',
        '61dd270b-fdad-49ca-a434-58bc46a18c2f', '0d2c9d92-2005-49ec-bee5-5fe14fbd60a6',
        'fd2fcf78-11ce-454c-b7e6-606818b04df9', 'a17077a1-e183-4d2f-8570-9fd759d49a72',
        '7269ba8b-def3-46b5-a094-50e900d573fd', 'e1d0e385-b0b3-4a66-afa1-d018a94d36d9',
        'd25cc63e-e5e6-458d-8289-1c5eb2275f85', '74e9291d-d7d6-4add-82f1-e1311909c4e0',
        'e620e487-5a2c-4801-ac70-d8ae172bc75e', '7d330068-271f-49fc-aa64-7e83091673d3',
        '5113cd16-8236-4c3a-af65-19e89af4bf7c', '9290aa7f-d6cd-49a9-b4d2-f5d879459f5f',
        '641763ad-a86f-4c5b-a135-4522a9796a0f', '33a0ba6b-cb89-4835-843a-5b3a90338e5b',
        '0aeaa199-68b3-4398-aec8-9362b91acef1', '748438d7-15d5-4f5e-9cf2-6945feb76541',
        'adbdd682-4238-430e-a1ed-f0fafe6c0881', 'cd9698cc-0a5c-4b9e-90b3-507039200c16',
        'b44a1205-e227-48fe-9e3b-5e65d92bf11d', '77a51900-3530-4338-931a-32e9ba4c4ad8',
        'e2c570a4-e488-45da-acfb-202cf9dbb8c6', 'def942a8-a251-44e3-b6b0-c6242f48a7bf',
        'fa2a75f6-cd6e-4a66-82e4-f08350153cab', 'fa40f49b-4ea4-4005-9843-fbca3b108bb9',
        '52a46401-6928-4d8b-ab91-f5cbb605b3cf', '3e23a2ba-98bb-4268-9abb-12d435874023',
        '3912df58-e68a-453d-8f5b-c35d02ba1495', 'c76bcabf-2d2e-48e6-8268-b89b291ed3c4',
        '86418cd2-603e-4427-a5f7-11be28a1035b', '98a05e64-5a1a-4495-b25a-67ca44564363',
        '24fb1327-012f-406b-8c8d-856a106f09b6', 'bde7a446-141d-49ea-98e6-1bd8a7a71556',
        '1e8729ef-4c6d-49a1-99dc-d4b98da43b30', '298a6ee6-e930-4024-a096-c3dbc9299071',
        'c73fcffb-72e0-49af-b255-182c91d4e10a', '6d973836-8679-4b56-b29c-87db38c4bfe3',
        '228eb721-4b48-429a-bde3-06c94fe819b8', '04b66f17-81e2-4fd2-9065-bc236b5a9a78',
        '274fc687-46c1-4473-8ff9-93b140a0cb22', '531280c7-fa10-4684-ba4f-0e0315d9e17a',
        '4306ea0c-7ddf-457b-8da6-482aa51166d5', '7769701d-5304-41f7-9642-78c1a3d7db41',
        'bc4b741e-935a-48b8-9b0d-b8851729611c', '142f6897-aba0-4fe4-985b-6431c13f837a',
        'fa02cd1e-430c-482a-8400-99d539a50e04', '6f93ce51-7b4c-4102-be8b-4a45cd0e30d5',
        '7d688a51-8d09-405a-8394-e495e3894a2e', '146cc16d-c158-4478-be20-3e50de0ff4da',
        '2ddfaddf-3d52-44be-b609-116ce85f2393', 'b35ab5b1-bbfe-4ad5-a725-20e4574bc8ef',
        'bc688c31-503b-4ffc-9d01-0f008eaa6d96', 'e2dcf3f4-43b6-41a2-ab11-3d86783b20de',
        '249c126d-ea89-4331-a933-8daaa8817b32', '3f797b6c-883e-4ce5-87f5-83ad140ea460',
        '540a5169-0202-4b12-bb6e-809c15b4264d', '03a5ee96-5d0f-4988-9556-2f12fe658d8d',
        '7cbe7e85-ff06-4970-824d-9e3a8eecc0cd', '71217e9b-337e-4b3a-a10a-932919bfd040',
        '49b27c8b-13ea-4e1b-9b62-51a51b17880b', 'fb932fc6-1aee-4ea6-a6e9-b4746bbd5131',
        '57b44b90-41f5-408e-a3cb-29fac939273a', 'c4ac1e58-d0c9-491d-88eb-0fcf0b3b16c3',
        '123def2b-bf98-4482-ae64-ff927285b706', 'cb16e2c9-268a-4ade-b8ae-786db85368af',
        '0162fe0b-2b93-44f9-bce1-c61e94d08fc6', '8edd397e-ce43-4004-93c5-62bf8895e609',
        '738722ec-352f-459a-a7bf-200256492ac4', '9919ce44-2a43-444a-8fc1-6522b1eaf5ea',
        '4a8f3440-3271-4964-9b1b-de543d328b18', '4e17240a-5e8b-4cbe-9af1-fd84def98658',
        '34e9dd09-ac4b-4e23-9af1-9b4ce0f1f087', 'c8dde0e8-61f3-46e1-9c44-b7a8fba1f73e',
        '43353099-cc22-4a49-ac5c-0d912cdf6156', '321c162c-084e-4d8e-a6d6-3c993daf4f97',
        'cd61300f-e799-4e88-83f9-89a5fba79939', 'c92986a5-6d2a-4533-9ecb-15474956ee58',
        '5aaa6999-c12d-46fd-965c-f342cf085cfe', 'c20cdb69-ebf7-492d-85d6-ca40623e362f',
        'cad9d1dc-0784-48f4-8a01-f04d510c1685', 'f3b31d45-b681-41e5-9a72-666799511e8e',
        '7b42c02d-073c-4d85-b56c-7b9bfce54275', '046244f9-dee7-4c7c-955b-f4a34be40736',
        '5040bc08-3acc-4c53-8710-37425057e91f', '55712258-1259-4b2f-88c5-2fa933638268',
        '7bd5921f-3ccb-480a-83a8-b1e4cabd95e1', 'c4e0d421-51c0-457e-bb21-2065ba782f53',
        'a0487dba-84b4-4cd5-8679-ae3e3323f08b', 'cd889d97-dfcd-422b-8d23-c683f39808eb',
        '53286d23-27f2-43e6-b189-526864ab1c97', 'aee4d57e-6597-4052-9fa2-032601e9c76c',
        '7b1f3b51-56e4-4f37-ac4e-7145d00cdbb6', '5d14f092-e6b5-49ad-ac60-3f636e8d5578',
        'bce65344-9477-4271-a530-533c9cd33b14', 'ee56c4b1-7be3-438e-b698-8abb4a9a9020',
        '5b9d6905-ef63-471f-b5d7-454d06098cc2', 'd6b6702b-275b-4ce2-82b5-ba5a47b64a24',
        '569d0e16-2921-4361-b96a-009d1c17de4f', 'ab6e157e-c997-4eae-a170-9659e90e8289',
        'cafdf240-a76b-47e1-9156-51e566f21e13', '2f2ee237-6be8-4e70-953a-df810d061493',
        '4075c276-10be-46d4-9173-4598785acda8', '0c0aaeb3-7bfd-4201-ae22-a9a953bc1adf',
        '990451ea-3bfd-466b-8e4a-8f881c084c56', '3d7449c3-3257-417a-b0b3-11d7ca8ff5bf',
        '56060b0a-d227-4445-99b0-851d8a7b0197', '99d90927-acd5-45be-aeeb-9f025814027d',
        '8672f219-ad73-4408-95e5-83bd0752df03', '97ada405-d676-4a6c-ae3d-396a840436ea',
        '67354103-1f4b-4793-9e6c-f60605e90851', '3b15a8f1-2081-4f51-8842-63d85e7c1ebb',
        'b2525d2d-b967-49a6-82f0-cadc6ad90ceb', 'c9e13ce3-dc34-47bb-84f4-c1ae751ce1f7',
        'b3420398-3120-4c21-8aec-a1a0fced28a9', 'c002e4af-a889-496b-ab52-b1015f84a561',
        'ebc4d2ed-1988-451c-bd1b-18b40b452f92', '37d8e9ba-0a4e-44f9-a77e-a73326fbce02',
        'e14071ab-f053-4c44-aed5-241342244c6e', 'cbacd4b7-0601-4bcd-ab01-91be4dd0507c',
        '56ac8a57-c620-43b4-900a-c3d83b97d4c6', 'e15ae4e7-4c75-432a-a4d0-765b9df91e32',
        '15ae90b9-5d05-4599-8c87-04eb9b0843ce', '8df7943a-ccb1-4ea9-b007-98409058e80d',
        '2381f171-3f5c-4521-b12e-51caf345bf03', '9fa08b1c-b1c7-4065-846a-cabf77781187',
        'cb44a991-34ac-4898-ab8f-1b57322250fb', '1f6bd8fd-f8b4-442e-b384-1c581b0ac6dd',
        '88a650b1-4529-4065-9d3e-7c24a0a17726', 'bcf4c9b8-896d-4d05-99bc-26fc6998a4c7',
        'aba3ef6e-b88a-4265-9d6f-9db4b88fc89f', 'dfaaa78f-e0a2-4a1b-9343-11435160eadf',
        '1a33be96-64ed-41b7-b10e-4d2848642d93', 'b09b96ed-93ca-4044-a3e8-48e35e5951ee',
        '3dd2d7ae-9cbf-4758-bd7f-da9759ac6268', '537b7f2c-f801-476e-ac5b-0b67a9fc9e00',
        'fe79aa4f-d3b9-41ce-ac20-c03054db7428', '5a585759-7840-4fae-8c9a-42b262e911ce',
        '728b6019-2a13-4681-8adc-5cc3df1b4286', 'ee8e1d58-8c17-47a9-917e-ee591e230749',
        '94a51434-dad7-4d61-b89e-9ddff47cb89b', '32e7dd3d-59c0-46be-b802-54d4a4a96254',
        '432afa46-d24d-4c93-9e0a-19582ec4493b', '04748267-6ac6-44d7-8cc5-f2f6a35cae62',
        'a6dcfaf6-4836-439e-9265-588e84390e43', '138fc96e-6504-4391-a014-71a55b4d7d6c',
        '6e37c5bb-c08a-4cc5-ac4f-0506c81d8cf2', '1fa24c22-88ab-4726-bb45-622f6c50481d',
        '64524c0a-f477-4712-9261-b9fbcd1027fc', '2c8974be-04a0-4853-a789-67e02a280dc8',
        'ee3c0662-9b74-4057-b134-6a0eb9ba0738', 'a0efa37c-d29d-4709-8528-5f747d2553cc',
        '8461e5bc-07bd-4994-a5f2-a084b36ee64b', '64261668-02ea-481c-9c43-f1841fab48f0',
        '809f8b72-6a65-4cce-94f9-a63697a17de4', '0a66a196-ffae-450f-96ec-1d815b92bc18',
        'bb1ec6cd-fcaf-4363-be38-43fe5d08db85', 'a9e49601-059f-43b0-8bb0-4fe366903250',
        '85bdb314-f7fb-4a02-8bfa-781922198f28', 'e0b60c10-c9e2-44c3-b0c3-0ba7aa669195',
        '6184d7ed-9427-45d2-82b4-b68df6e43f4f', '071cf6af-28c9-4e6f-aac7-db36580847a1',
        'e843ff1d-c266-4852-89d5-b078b448f877', 'b97ba400-0402-447c-9cf1-f8489e7006b1',
        'ba14dd22-4d32-4ed9-a659-ad5945f66c89', '876ae655-4490-45a8-a185-e5d846602a72',
        '7c9949bd-05af-47ef-89cd-801c40697fe0', '17990e91-f281-490b-874f-332f745b6ea2',
        '6099b19a-ba54-431b-bdf7-740e7143bbda', 'ec40c6e9-21b9-4f6b-aac9-9190ffbfbb1b',
        'b637ccf1-8af8-4090-9380-43e9ba2d617d', '8cf17563-2809-4307-be6b-746b569fe433',
        '6aef079b-1360-4ac8-9229-e292eb6420cc', '407a0e97-a310-4108-80f7-39ceb925620d',
        '5fbde1aa-2500-4742-aa8b-02445126d6a4', '5fe1fd9c-7f5d-4abd-896b-30238a565029',
        'fb184052-9f6b-462f-ab81-8df52b12ffae', '52d99d10-50a2-48f0-b393-c05a3a91c952',
        '8f0f7869-94f4-4cb3-875a-236865374170', '81e6f14c-73af-4874-8874-f1ff3fb8fedf',
        'cdd581fd-de15-44ba-80ae-66c4bfe958f0', '3659a168-8195-4f17-bf71-106fdf5a2ba1',
        '5ff92f66-8f5d-441a-8009-a1a76810ae97', 'e19c0b3e-8957-4926-9f05-ae9a7db94a4a',
        '6ca4133c-ad42-471c-a74c-77ee3fa1820d', 'b6d7b070-3b17-4ea9-b097-122e1861d246',
        '0a8f66f0-6257-4e0d-9792-8fb8c3ce03a4', 'd878c1fb-88db-4b04-ab53-b0f80dc41fc3',
        '14c53ba0-6d90-45c9-a066-efe6353a6170', '25902a5c-c2f6-40b6-8d7c-e25dca1e0352',
        'ebd0b95d-c9ed-4a43-9b3f-f7ade6aee62c', 'e47b83a7-abbf-4f36-90da-627d35700902',
        '7cd23341-76cc-4919-b4eb-43a9ee562eec', 'd622df22-80f9-4b81-a91e-24bf8d2000c0',
        '156fc269-2d6a-4e48-921d-f16f6ae2403b', '5dad6a0a-a50a-478b-a849-f1836ca50f6f',
        'a9ad3b64-889c-4565-8290-16ca85b325d1', 'fde2e686-1d35-4bcf-958a-8b028969a817',
        '7ccecb50-a3cf-4cf1-8109-07af0dfa7336', '50e0708c-1a09-477f-ac55-fadd51bacc01',
        'fffa4cd1-578f-435b-95a0-3c7f3b049e1f', 'b9c1c205-96f8-43d9-a5b4-bd24fb65ade0',
        '75ab2722-3ebd-4951-967c-c3e06c55b355', '11aa538b-4b32-44d9-993e-1998ed053513',
        'cbdf4d42-31f6-46ba-b6b2-3078b75f7dc0', '1c28ebcd-2039-49f6-9ecc-970150e89d8a',
        'f44478e7-4f59-4936-be9e-bcbfdc659502', '87523cb8-42dc-4aa3-87ef-182218e8745f',
        '11c93eac-b9e9-450c-a9a8-1ed24eaf24b6', 'f012be39-5221-42fe-adca-f3a0a587168d',
        '56932efb-c898-4dc2-a63c-c0faa52baf93', 'd450e744-ea03-4741-9eec-0e3db8f93c22',
        '52f110b0-b08a-4647-b3f3-86325c86369f', '6e45a526-b992-44b7-b3a8-b8e74c3cef33',
        'e786f2db-0982-478b-8895-a0f9384312de', '9866915f-b1e7-4200-bae8-fecd8492b9d8',
    ]

    client_ids = [
        str(uuid.uuid4()) for x in range(0, 30)
    ]

    # ................................................................................................................
    #   delete all data
    yield [
        memcached.xdelete(guid) for guid in guids
    ]

    # ................................................................................................................
    #   update locations data
    for cid in client_ids:
        futures = [
            locator.update_location(guid, cid) for guid in guids
        ]

        results = yield futures
        results = map(lambda x: x[0] or x[1], results)
        assert all(results)

    # ................................................................................................................
    #   update locations data
    with open('bigchat.resolve.txt', 'w') as ofs:
        for i in range(0, 1000):
            counter = PerformanceCounter().start()

            entries = yield locator.resolve_locations(guids)
            assert len(entries) == len(guids)

            duration = counter.stop()

            counter2 = PerformanceCounter().start()
            cloc = None
            for guid in guids:
                #assert guid in entries
                for _, cid, location in entries[guid].enumerate_locations():
                    if cloc is None:
                        cloc = location
            duration2 = counter2.stop()

            ofs.write('RESOLVE %.6f   ENUMERATE/ASSERT %.6f    LOC %s\n' % (duration * 1000, duration2 * 1000, cloc))


@alice.uniproxy.library.testing.ioloop_run
def test_update_and_resolve_many_guids_many_clients_old():
    yield memcached_init_async()
    memcached = memcached.memcached_client(memcached.MEMCACHED_DELIVERY_SERVICE)
    locator = messenger.locator2.ClientLocator(memcached.MEMCACHED_DELIVERY_SERVICE)

    guids = [
        '16b73702-0f0d-405d-a5b8-3d47ac1ead79', '226e7e46-8281-4176-ad53-98bbd6bffa89',
        '37a4b60c-5485-437a-b1db-d413d44e37ec', 'f654e291-b3f4-4478-a878-b9f370fac2a6',
        '5d9bf837-63f4-4098-8627-f7a7a5cd112f', 'fcc9861b-3163-4977-b5b4-9ae19adf3448',
        'f19c9155-39a6-4bb7-ab15-61dc366bebf0', '813cbb18-9044-4fa2-ac8b-23369df6ce8f',
        '67baa54a-2c56-43fc-bf17-e1c8a0516f2c', '94f34ca5-db5a-4b7b-b115-56bfeb80f860',
        '47737382-29f5-4dd9-9401-7798490bd499', '10e843fe-e2b1-4d21-9c42-ded03c22e3fe',
        'c8b155a5-3e19-45cc-95d8-f0b823422fd3', '57a40d16-2bbf-4e3d-aaf0-36f16b84d68a',
        '7cd3ed3f-04f6-48e5-b895-abf45305356c', 'd5b715aa-dbb5-4ce5-848b-e7ff222e11dc',
        'bb8fc28a-d5ba-4226-aa79-6b15e03719bb', '5cb94be7-c904-413f-a590-54b7b974af19',
        'cc57a26d-07a0-469a-aa86-43dc72c336bd', '2bd596f4-ddc7-4b5d-a48a-b78fd45d8546',
        'cf597b4e-2ea9-4ff3-b5e9-15cbf2b70937', '9e8ee664-b38e-424d-a692-f6d7613565ce',
        '14418d73-63c0-4300-ae62-e62cabed1935', 'be6580c2-165c-463c-827e-ab964ffa15bb',
        '9cfba49b-f4ff-4774-86e5-b1cc12cca3b8', '1ae60a85-7c33-4b30-ae59-47a437c00165',
        'ba50db99-2b3a-4ff6-adaa-5ec45c9e3bd6', '8d68bb6b-ef15-429a-a0cb-297bb06d9359',
        'cb038e5b-1102-4eba-be56-194cfb98cc88', 'fdb26644-2ec1-42c8-9da3-e905bd126eef',
        '84acf2fa-5137-4b8c-a727-3037ffd1defd', 'f70a53fb-2590-40b2-b2f2-2013105a1f5c',
        'd7797b36-394e-48b5-a860-f0352f5261fa', '324d2199-d40b-457d-b40d-cdf111b800db',
        '826fbcf5-011e-4a53-bf7b-ca1c158e3a99', '8c55d73f-1bc8-407f-b024-274c59ef5721',
        '21505222-95a9-4ed5-83cc-290caba529e9', 'ade2f842-4520-4f12-b87d-b9acb3de7983',
        'f3b6021a-d034-4698-9bf8-c08cbabad18b', '33813a95-24a6-498a-bfb9-63950a87f2cb',
        '8e75f368-82a0-45dc-82af-7060d8ce8dbe', '0ec67a26-ebee-44bd-85b1-e81da0d5107e',
        '69b07bd6-f370-44fc-8433-73785212fdb6', '8dfc2287-6eb2-4c3e-8c18-0e5aa379c547',
        '138d6409-ee41-4135-9a5b-7b4d9066c607', '4555f11f-22b2-4acc-8e9b-90d71fd983a5',
        'e9f3bdd3-9d69-4dd7-a8eb-72aa82577fe5', '6a9ce50e-7fab-4b3f-9e96-da932e4a39f6',
        'a2da7f01-c0f0-42b5-aba0-37091e06a4e6', '978b57e1-1615-4f11-b3aa-64de5ab66b0a',
        'fbb1f94c-1e38-4317-a8cb-5146764d3966', '52d283b1-ac9b-4dc0-8d97-7b84604077cd',
        '78ecdd85-a216-45c6-a6c6-41f59587f337', '2b49bcae-1db2-4bda-ac4b-680f34dd48f5',
        '6f9b1e45-79e0-4324-b753-a47596359bba', '0bb6f05f-d84e-4fcd-8551-007f78c32114',
        'a648b733-3d98-48da-aca9-bc5f250886fb', 'd8ded7ea-ad29-4907-a8c6-97f862a7d8bc',
        'af6511f2-049e-4b84-9a91-4df97c287333', '90107b99-60a8-405b-8d53-5bb8f6f19cc2',
        'b2bac782-e9de-4f58-aed3-4d16c1d17c6e', '9997dbbd-d394-48a8-ae92-1de3bffd400e',
        '573e66c6-426f-4a7f-9b35-b45cb5112b1c', '5460623a-1184-4ada-add3-09015abd6333',
        '9a7fb8c0-32a4-44a6-93a2-785ff9be0166', '3dfaf8f1-7b8e-49de-b35f-cdeacfdefa7b',
        '6545e54f-3b75-4edb-a0ca-87af0f725dab', '48816d1d-3093-41a5-b951-2918d611ee45',
        '0c212474-cdc5-42a5-8900-2b940ceb6f54', 'bdc4d636-4fb2-4682-afb9-9a6dc412eb51',
        'd035e00b-371b-4b26-806e-ea3d8285c507', 'e83c94be-6c34-4121-a2b7-03cdfc13132b',
        '0b234ada-6dab-4a5a-aafe-d03e005dad31', '093bb91e-0cc9-42d4-b000-be40819cecfd',
        'afe3f12c-5b63-487c-b577-3995fce8d7c6', 'a6cdfdf8-bc94-4a5c-8914-f35c15e98f1c',
        'b1f06e67-6852-425b-a64f-4ac02e494e0e', '458b3726-5ca5-4554-b244-443e9d4d200e',
        '7383ff44-0dc5-4478-b04b-d9d9a0b815fc', 'bfe0e0dd-cdef-4f37-a75b-3672ed4a61eb',
        '4a09c921-1241-4cc9-b097-927723a60048', 'ba593fc1-9412-47d5-bd9e-3c48cf209f2b',
        'c005434b-fd45-4c1f-bc5a-952d718dba78', 'f2f0e8fc-c085-44a3-82a9-0e11dfd08b6c',
        '500beb56-25e8-4ea9-bb54-e4d6b303bd58', 'f3142616-7d92-4cf2-b889-bcabf3863d05',
        '60724464-37a3-40b1-85f8-0e1fd21afc9e', 'd85ef1c5-7f48-4af4-87c9-95ff2e81a1f6',
        '61dd270b-fdad-49ca-a434-58bc46a18c2f', '0d2c9d92-2005-49ec-bee5-5fe14fbd60a6',
        'fd2fcf78-11ce-454c-b7e6-606818b04df9', 'a17077a1-e183-4d2f-8570-9fd759d49a72',
        '7269ba8b-def3-46b5-a094-50e900d573fd', 'e1d0e385-b0b3-4a66-afa1-d018a94d36d9',
        'd25cc63e-e5e6-458d-8289-1c5eb2275f85', '74e9291d-d7d6-4add-82f1-e1311909c4e0',
        'e620e487-5a2c-4801-ac70-d8ae172bc75e', '7d330068-271f-49fc-aa64-7e83091673d3',
        '5113cd16-8236-4c3a-af65-19e89af4bf7c', '9290aa7f-d6cd-49a9-b4d2-f5d879459f5f',
        '641763ad-a86f-4c5b-a135-4522a9796a0f', '33a0ba6b-cb89-4835-843a-5b3a90338e5b',
        '0aeaa199-68b3-4398-aec8-9362b91acef1', '748438d7-15d5-4f5e-9cf2-6945feb76541',
        'adbdd682-4238-430e-a1ed-f0fafe6c0881', 'cd9698cc-0a5c-4b9e-90b3-507039200c16',
        'b44a1205-e227-48fe-9e3b-5e65d92bf11d', '77a51900-3530-4338-931a-32e9ba4c4ad8',
        'e2c570a4-e488-45da-acfb-202cf9dbb8c6', 'def942a8-a251-44e3-b6b0-c6242f48a7bf',
        'fa2a75f6-cd6e-4a66-82e4-f08350153cab', 'fa40f49b-4ea4-4005-9843-fbca3b108bb9',
        '52a46401-6928-4d8b-ab91-f5cbb605b3cf', '3e23a2ba-98bb-4268-9abb-12d435874023',
        '3912df58-e68a-453d-8f5b-c35d02ba1495', 'c76bcabf-2d2e-48e6-8268-b89b291ed3c4',
        '86418cd2-603e-4427-a5f7-11be28a1035b', '98a05e64-5a1a-4495-b25a-67ca44564363',
        '24fb1327-012f-406b-8c8d-856a106f09b6', 'bde7a446-141d-49ea-98e6-1bd8a7a71556',
        '1e8729ef-4c6d-49a1-99dc-d4b98da43b30', '298a6ee6-e930-4024-a096-c3dbc9299071',
        'c73fcffb-72e0-49af-b255-182c91d4e10a', '6d973836-8679-4b56-b29c-87db38c4bfe3',
        '228eb721-4b48-429a-bde3-06c94fe819b8', '04b66f17-81e2-4fd2-9065-bc236b5a9a78',
        '274fc687-46c1-4473-8ff9-93b140a0cb22', '531280c7-fa10-4684-ba4f-0e0315d9e17a',
        '4306ea0c-7ddf-457b-8da6-482aa51166d5', '7769701d-5304-41f7-9642-78c1a3d7db41',
        'bc4b741e-935a-48b8-9b0d-b8851729611c', '142f6897-aba0-4fe4-985b-6431c13f837a',
        'fa02cd1e-430c-482a-8400-99d539a50e04', '6f93ce51-7b4c-4102-be8b-4a45cd0e30d5',
        '7d688a51-8d09-405a-8394-e495e3894a2e', '146cc16d-c158-4478-be20-3e50de0ff4da',
        '2ddfaddf-3d52-44be-b609-116ce85f2393', 'b35ab5b1-bbfe-4ad5-a725-20e4574bc8ef',
        'bc688c31-503b-4ffc-9d01-0f008eaa6d96', 'e2dcf3f4-43b6-41a2-ab11-3d86783b20de',
        '249c126d-ea89-4331-a933-8daaa8817b32', '3f797b6c-883e-4ce5-87f5-83ad140ea460',
        '540a5169-0202-4b12-bb6e-809c15b4264d', '03a5ee96-5d0f-4988-9556-2f12fe658d8d',
        '7cbe7e85-ff06-4970-824d-9e3a8eecc0cd', '71217e9b-337e-4b3a-a10a-932919bfd040',
        '49b27c8b-13ea-4e1b-9b62-51a51b17880b', 'fb932fc6-1aee-4ea6-a6e9-b4746bbd5131',
        '57b44b90-41f5-408e-a3cb-29fac939273a', 'c4ac1e58-d0c9-491d-88eb-0fcf0b3b16c3',
        '123def2b-bf98-4482-ae64-ff927285b706', 'cb16e2c9-268a-4ade-b8ae-786db85368af',
        '0162fe0b-2b93-44f9-bce1-c61e94d08fc6', '8edd397e-ce43-4004-93c5-62bf8895e609',
        '738722ec-352f-459a-a7bf-200256492ac4', '9919ce44-2a43-444a-8fc1-6522b1eaf5ea',
        '4a8f3440-3271-4964-9b1b-de543d328b18', '4e17240a-5e8b-4cbe-9af1-fd84def98658',
        '34e9dd09-ac4b-4e23-9af1-9b4ce0f1f087', 'c8dde0e8-61f3-46e1-9c44-b7a8fba1f73e',
        '43353099-cc22-4a49-ac5c-0d912cdf6156', '321c162c-084e-4d8e-a6d6-3c993daf4f97',
        'cd61300f-e799-4e88-83f9-89a5fba79939', 'c92986a5-6d2a-4533-9ecb-15474956ee58',
        '5aaa6999-c12d-46fd-965c-f342cf085cfe', 'c20cdb69-ebf7-492d-85d6-ca40623e362f',
        'cad9d1dc-0784-48f4-8a01-f04d510c1685', 'f3b31d45-b681-41e5-9a72-666799511e8e',
        '7b42c02d-073c-4d85-b56c-7b9bfce54275', '046244f9-dee7-4c7c-955b-f4a34be40736',
        '5040bc08-3acc-4c53-8710-37425057e91f', '55712258-1259-4b2f-88c5-2fa933638268',
        '7bd5921f-3ccb-480a-83a8-b1e4cabd95e1', 'c4e0d421-51c0-457e-bb21-2065ba782f53',
        'a0487dba-84b4-4cd5-8679-ae3e3323f08b', 'cd889d97-dfcd-422b-8d23-c683f39808eb',
        '53286d23-27f2-43e6-b189-526864ab1c97', 'aee4d57e-6597-4052-9fa2-032601e9c76c',
        '7b1f3b51-56e4-4f37-ac4e-7145d00cdbb6', '5d14f092-e6b5-49ad-ac60-3f636e8d5578',
        'bce65344-9477-4271-a530-533c9cd33b14', 'ee56c4b1-7be3-438e-b698-8abb4a9a9020',
        '5b9d6905-ef63-471f-b5d7-454d06098cc2', 'd6b6702b-275b-4ce2-82b5-ba5a47b64a24',
        '569d0e16-2921-4361-b96a-009d1c17de4f', 'ab6e157e-c997-4eae-a170-9659e90e8289',
        'cafdf240-a76b-47e1-9156-51e566f21e13', '2f2ee237-6be8-4e70-953a-df810d061493',
        '4075c276-10be-46d4-9173-4598785acda8', '0c0aaeb3-7bfd-4201-ae22-a9a953bc1adf',
        '990451ea-3bfd-466b-8e4a-8f881c084c56', '3d7449c3-3257-417a-b0b3-11d7ca8ff5bf',
        '56060b0a-d227-4445-99b0-851d8a7b0197', '99d90927-acd5-45be-aeeb-9f025814027d',
        '8672f219-ad73-4408-95e5-83bd0752df03', '97ada405-d676-4a6c-ae3d-396a840436ea',
        '67354103-1f4b-4793-9e6c-f60605e90851', '3b15a8f1-2081-4f51-8842-63d85e7c1ebb',
        'b2525d2d-b967-49a6-82f0-cadc6ad90ceb', 'c9e13ce3-dc34-47bb-84f4-c1ae751ce1f7',
        'b3420398-3120-4c21-8aec-a1a0fced28a9', 'c002e4af-a889-496b-ab52-b1015f84a561',
        'ebc4d2ed-1988-451c-bd1b-18b40b452f92', '37d8e9ba-0a4e-44f9-a77e-a73326fbce02',
        'e14071ab-f053-4c44-aed5-241342244c6e', 'cbacd4b7-0601-4bcd-ab01-91be4dd0507c',
        '56ac8a57-c620-43b4-900a-c3d83b97d4c6', 'e15ae4e7-4c75-432a-a4d0-765b9df91e32',
        '15ae90b9-5d05-4599-8c87-04eb9b0843ce', '8df7943a-ccb1-4ea9-b007-98409058e80d',
        '2381f171-3f5c-4521-b12e-51caf345bf03', '9fa08b1c-b1c7-4065-846a-cabf77781187',
        'cb44a991-34ac-4898-ab8f-1b57322250fb', '1f6bd8fd-f8b4-442e-b384-1c581b0ac6dd',
        '88a650b1-4529-4065-9d3e-7c24a0a17726', 'bcf4c9b8-896d-4d05-99bc-26fc6998a4c7',
        'aba3ef6e-b88a-4265-9d6f-9db4b88fc89f', 'dfaaa78f-e0a2-4a1b-9343-11435160eadf',
        '1a33be96-64ed-41b7-b10e-4d2848642d93', 'b09b96ed-93ca-4044-a3e8-48e35e5951ee',
        '3dd2d7ae-9cbf-4758-bd7f-da9759ac6268', '537b7f2c-f801-476e-ac5b-0b67a9fc9e00',
        'fe79aa4f-d3b9-41ce-ac20-c03054db7428', '5a585759-7840-4fae-8c9a-42b262e911ce',
        '728b6019-2a13-4681-8adc-5cc3df1b4286', 'ee8e1d58-8c17-47a9-917e-ee591e230749',
        '94a51434-dad7-4d61-b89e-9ddff47cb89b', '32e7dd3d-59c0-46be-b802-54d4a4a96254',
        '432afa46-d24d-4c93-9e0a-19582ec4493b', '04748267-6ac6-44d7-8cc5-f2f6a35cae62',
        'a6dcfaf6-4836-439e-9265-588e84390e43', '138fc96e-6504-4391-a014-71a55b4d7d6c',
        '6e37c5bb-c08a-4cc5-ac4f-0506c81d8cf2', '1fa24c22-88ab-4726-bb45-622f6c50481d',
        '64524c0a-f477-4712-9261-b9fbcd1027fc', '2c8974be-04a0-4853-a789-67e02a280dc8',
        'ee3c0662-9b74-4057-b134-6a0eb9ba0738', 'a0efa37c-d29d-4709-8528-5f747d2553cc',
        '8461e5bc-07bd-4994-a5f2-a084b36ee64b', '64261668-02ea-481c-9c43-f1841fab48f0',
        '809f8b72-6a65-4cce-94f9-a63697a17de4', '0a66a196-ffae-450f-96ec-1d815b92bc18',
        'bb1ec6cd-fcaf-4363-be38-43fe5d08db85', 'a9e49601-059f-43b0-8bb0-4fe366903250',
        '85bdb314-f7fb-4a02-8bfa-781922198f28', 'e0b60c10-c9e2-44c3-b0c3-0ba7aa669195',
        '6184d7ed-9427-45d2-82b4-b68df6e43f4f', '071cf6af-28c9-4e6f-aac7-db36580847a1',
        'e843ff1d-c266-4852-89d5-b078b448f877', 'b97ba400-0402-447c-9cf1-f8489e7006b1',
        'ba14dd22-4d32-4ed9-a659-ad5945f66c89', '876ae655-4490-45a8-a185-e5d846602a72',
        '7c9949bd-05af-47ef-89cd-801c40697fe0', '17990e91-f281-490b-874f-332f745b6ea2',
        '6099b19a-ba54-431b-bdf7-740e7143bbda', 'ec40c6e9-21b9-4f6b-aac9-9190ffbfbb1b',
        'b637ccf1-8af8-4090-9380-43e9ba2d617d', '8cf17563-2809-4307-be6b-746b569fe433',
        '6aef079b-1360-4ac8-9229-e292eb6420cc', '407a0e97-a310-4108-80f7-39ceb925620d',
        '5fbde1aa-2500-4742-aa8b-02445126d6a4', '5fe1fd9c-7f5d-4abd-896b-30238a565029',
        'fb184052-9f6b-462f-ab81-8df52b12ffae', '52d99d10-50a2-48f0-b393-c05a3a91c952',
        '8f0f7869-94f4-4cb3-875a-236865374170', '81e6f14c-73af-4874-8874-f1ff3fb8fedf',
        'cdd581fd-de15-44ba-80ae-66c4bfe958f0', '3659a168-8195-4f17-bf71-106fdf5a2ba1',
        '5ff92f66-8f5d-441a-8009-a1a76810ae97', 'e19c0b3e-8957-4926-9f05-ae9a7db94a4a',
        '6ca4133c-ad42-471c-a74c-77ee3fa1820d', 'b6d7b070-3b17-4ea9-b097-122e1861d246',
        '0a8f66f0-6257-4e0d-9792-8fb8c3ce03a4', 'd878c1fb-88db-4b04-ab53-b0f80dc41fc3',
        '14c53ba0-6d90-45c9-a066-efe6353a6170', '25902a5c-c2f6-40b6-8d7c-e25dca1e0352',
        'ebd0b95d-c9ed-4a43-9b3f-f7ade6aee62c', 'e47b83a7-abbf-4f36-90da-627d35700902',
        '7cd23341-76cc-4919-b4eb-43a9ee562eec', 'd622df22-80f9-4b81-a91e-24bf8d2000c0',
        '156fc269-2d6a-4e48-921d-f16f6ae2403b', '5dad6a0a-a50a-478b-a849-f1836ca50f6f',
        'a9ad3b64-889c-4565-8290-16ca85b325d1', 'fde2e686-1d35-4bcf-958a-8b028969a817',
        '7ccecb50-a3cf-4cf1-8109-07af0dfa7336', '50e0708c-1a09-477f-ac55-fadd51bacc01',
        'fffa4cd1-578f-435b-95a0-3c7f3b049e1f', 'b9c1c205-96f8-43d9-a5b4-bd24fb65ade0',
        '75ab2722-3ebd-4951-967c-c3e06c55b355', '11aa538b-4b32-44d9-993e-1998ed053513',
        'cbdf4d42-31f6-46ba-b6b2-3078b75f7dc0', '1c28ebcd-2039-49f6-9ecc-970150e89d8a',
        'f44478e7-4f59-4936-be9e-bcbfdc659502', '87523cb8-42dc-4aa3-87ef-182218e8745f',
        '11c93eac-b9e9-450c-a9a8-1ed24eaf24b6', 'f012be39-5221-42fe-adca-f3a0a587168d',
        '56932efb-c898-4dc2-a63c-c0faa52baf93', 'd450e744-ea03-4741-9eec-0e3db8f93c22',
        '52f110b0-b08a-4647-b3f3-86325c86369f', '6e45a526-b992-44b7-b3a8-b8e74c3cef33',
        'e786f2db-0982-478b-8895-a0f9384312de', '9866915f-b1e7-4200-bae8-fecd8492b9d8',
    ]

    lg = open('test_update_and_resolve_many_guids_many_clients_old.log', 'w')

    # ................................................................................................................
    #   delete all data
    lg.write('GEN UUIDS...\n')
    lg.flush()
    client_ids = [
        [str(uuid.uuid4()) for x in range(0, 30)]
        for i in range(0, len(guids))
    ]

    lg.write('DEL OLD DATA...\n')
    lg.flush()
    # ................................................................................................................
    #   delete all data
    yield [
        memcached.xdelete(guid) for guid in guids
    ]

    lg.write('UPDATE MAPPINGS...\n')
    lg.flush()
    # ................................................................................................................
    #   update locations data
    futures = [
        memcached.xset(guid, ';'.join(uuids), exptime=1800)
        for guid, uuids in zip(guids, client_ids)
    ]

    yield futures

    lg.write('UPDATE LOCATIONS...\n')
    lg.flush()
    location = utils.hostname.current_hostname()
    for uuids in client_ids:
        futures = [
            memcached.xset(uuid_, location) for uuid_ in uuids
        ]
        yield futures

    lg.write('RESOLVE LOCATIONS...\n')
    lg.flush()
    # ................................................................................................................
    #   update locations data
    with open('bigchat.resolve.old.txt', 'w') as ofs:
        for i in range(0, 1000):
            counter = PerformanceCounter().start()

            entries = yield locator.resolve_locations(guids)
            assert len(entries) == len(guids)

            duration = counter.stop()

            counter2 = PerformanceCounter().start()
            cloc = None
            for guid in guids:
                #assert guid in entries
                for _, cid, location in entries[guid].enumerate_locations():
                    if cloc is None:
                        cloc = location
            duration2 = counter2.stop()

            ofs.write('RESOLVE %.6f   ENUMERATE/ASSERT %.6f    LOC %s\n' % (duration * 1000, duration2 * 1000, cloc))

    lg.close()


@alice.uniproxy.library.testing.ioloop_run
def _test_remove_resolve():
    yield memcached_init_async()
    memcached = memcached.memcached_client(memcached.MEMCACHED_MSSNGR_SERVICE)
    locator = messenger.locator2.ClientLocator()

    guid = '70f60162-daf3-40a7-a2b1-2e9edb95bb65'
    client_id = 'd43d1b77-a593-45c0-a8de-ce291608260f'
    client_id2 = 'b80aa9b4-b891-4250-affc-8fb25bea5637'

    yield memcached.xdelete(guid)

    p, s = yield locator.update_location(guid, client_id)
    assert p and s

    p, s = yield locator.update_location(guid, client_id2)
    assert p and s

    entries = yield locator.resolve_locations([guid])
    assert len(entries) == 1
    assert guid in entries

    for _, entry in entries.items():
        locations = list(entry.enumerate_locations())
        assert len(locations) == 2

        for _, cid, location in locations:
            assert cid in [client_id, client_id2]

    p, s = yield locator.remove_location(guid, client_id)
    assert p and s

    entries = yield locator.resolve_locations([guid])
    assert len(entries) == 1
    assert guid in entries

    for _, entry in entries.items():
        locations = list(entry.enumerate_locations())
        assert len(locations) == 1

        for _, cid, location in locations:
            assert cid == client_id2

if __name__ == '__main__':
    test_update_resolve()
