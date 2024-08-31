# coding: utf-8

import socket

import pytest
from alice.hollywood.fixture import Hollywood


@pytest.fixture
def hollywood_port():
    return 42


@pytest.fixture
def hollywood_grpc_port(hollywood_port):
    return hollywood_port + 1


@pytest.fixture
def hollywood(hollywood_port, shard):
    return Hollywood(port=hollywood_port, shard=shard)


@pytest.fixture
def expected_srcrwr(hollywood_grpc_port, shard):
    return f'HOLLYWOOD_{shard.upper()}:{socket.gethostname()}:{hollywood_grpc_port}'


@pytest.mark.parametrize('shard', [
    'all', 'common', 'general_conversation', 'video',
])
def test_srcrwr(hollywood, expected_srcrwr, shard):
    assert shard in hollywood.srcrwr.lower()
    assert hollywood.srcrwr == expected_srcrwr


@pytest.mark.parametrize('shard', ['all'])
def test_ports(hollywood, hollywood_port, hollywood_grpc_port):
    assert hollywood.port == hollywood_port
    assert hollywood.grpc_port == hollywood_grpc_port
