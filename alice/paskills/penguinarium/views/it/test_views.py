import pytest
import json


@pytest.mark.parametrize(
    'uri,req_payload,resp_payload',
    [
        (
            '/add_node',
            {
                'node_id': '1',
                'intents': [{'intent_id': '1', 'utterances': ['.']}]
            },
            {'node_id': '1'}
        ),
        (
            '/rem_node',
            {'node_id': '1'},
            {'node_id': '1'}
        ),
    ],
    ids=['/add_node', '/rem_node']
)
@pytest.mark.asyncio
async def test_empty_db_ok(client, uri, req_payload, resp_payload):
    resp = await client.post(uri, data=json.dumps(req_payload))
    assert await resp.json() == resp_payload


@pytest.mark.asyncio
async def test_get_intents_several_times(client):
    await client.post('/add_node', data=json.dumps({
        'node_id': '1',
        'intents': [{'intent_id': '1', 'utterances': ['.']}]
    }))

    payload = json.dumps({'node_id': '1', 'utterance': 'а'})
    resps = [
        await client.post('/get_intents', data=payload)
        for _ in range(3)
    ]
    texts = [await r.text() for r in resps]

    assert all(texts[0] == t for t in texts)

    await client.post('/rem_node', data=json.dumps({
        'node_id': '1',
    }))


@pytest.mark.asyncio
async def test_get_multiple_intents(client):
    await client.post('/add_node', data=json.dumps({
        'node_id': '1',
        'intents': [
            {'intent_id': '1', 'utterances': ['йога для зрения']},
            {'intent_id': '2', 'utterances': ['разминка для глаз']},
            {'intent_id': '3', 'utterances': ['игра города']}
        ]
    }))

    resp = await client.post('/get_intents', data=json.dumps({
        'node_id': '1',
        'utterance': 'йога для глаз'
    }))
    resp_payload = await resp.json()

    assert resp_payload['intents'] == ['1', '2']

    await client.post('/rem_node', data=json.dumps({
        'node_id': '1',
    }))


@pytest.mark.parametrize(
    'utt_tokens,resp_text,is_new_session',
    [
        (['угу'], 'Слушайте мой первый вопрос! Чего вечно не хватает?', False),
        (['совершенно', 'отвлеченные', 'слова'], 'Не понимаю вас!', False),
        (['совершенно', 'отвлеченные', 'слова'], 'Ну что, поехали?', True),
    ],
    ids=['regular', 'fallback', 'new_session']
)
@pytest.mark.asyncio
async def test_add_get_graph_intent(client, app, add_grph_pld, get_grph_intnt_pld,
                                    resp_text):
    response = await client.post(
        '/add_graph',
        data=json.dumps(add_grph_pld)
    )
    resp_json = await response.json()
    assert resp_json['skill_id'] == add_grph_pld['id']

    response = await client.post(
        '/get_graph_intent',
        data=json.dumps(get_grph_intnt_pld)
    )
    assert response.status == 200
    resp_json = await response.json()
    assert resp_json['response']['text'] == resp_text

    response = await client.post(
        '/rem_graph',
        data=json.dumps({'graph_id': add_grph_pld['id']})
    )
    assert response.status == 200


@pytest.mark.parametrize('utt_tokens,is_new_session', [(['угу'], False)])
@pytest.mark.asyncio
async def test_get_graph_intent_unknown(client, get_grph_intnt_pld):
    response = await client.post(
        '/get_graph_intent',
        data=json.dumps(get_grph_intnt_pld)
    )

    assert response.status == 400
    assert await response.text() == '400: Unknown graph id some-skill-id'


@pytest.mark.asyncio
async def test_graph_repeating_add_rem(client, add_grph_pld, app):
    response = await client.post(
        '/add_graph',
        data=json.dumps(add_grph_pld)
    )
    resp_json = await response.json()
    assert resp_json['skill_id'] == add_grph_pld['id']
    assert await app['nodes_storage'].get('lack_of') is not None

    add_grph_pld['nodes'] = add_grph_pld['nodes'][:1]
    add_grph_pld['nodes'][0]['edges'][0]['node_id'] = 'for_one'
    response = await client.post(
        '/add_graph',
        data=json.dumps(add_grph_pld)
    )
    assert response.status == 200
    assert await app['nodes_storage'].get('lack_of') is None

    for _ in range(2):
        response = await client.post(
            '/rem_graph',
            data=json.dumps({'graph_id': add_grph_pld['id']})
        )
        assert response.status == 200


@pytest.mark.asyncio
async def test_get_solomon(client):
    response = await client.get('/solomon')
    resp_json = await response.json()

    assert len(resp_json) > 0

    for _ in range(3):
        await client.post(
            '/get_intents',
            data=json.dumps({'node_id': 'unexisting_node', 'utterance': 'а'})
        )

    response = await client.get('/solomon')
    resp_json = await response.json()

    bad_req_sensor = {
        'kind': 'RATE',
        'labels': {
            'path': '/get_intents',
            'sensor': 'http_responses',
            'status_code': '400'
        },
        'value': 3
    }
    assert bad_req_sensor in resp_json['sensors']


@pytest.mark.asyncio
async def test_unexisting_node(client):
    resp = await client.post(
        '/get_intents',
        data=json.dumps({'node_id': '1', 'utterance': 'а'})
    )
    assert resp.status == 400


@pytest.mark.asyncio
async def test_ping(client):
    resp = await client.get('/ping')
    assert resp.status == 200
