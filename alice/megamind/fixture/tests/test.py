import socket

import pytest
from alice.megamind.fixture import Megamind


@pytest.fixture
def megamind_port():
    return 42


@pytest.fixture
def megamind_grpc_port(megamind_port):
    return megamind_port + 3


@pytest.fixture
def megamind(megamind_port):
    return Megamind(port=megamind_port)


@pytest.fixture
def expected_srcrwr(megamind_grpc_port):
    return f'MEGAMIND_ALIAS:{socket.gethostname()}:{megamind_grpc_port}'


def test_srcrwr(megamind, expected_srcrwr):
    assert megamind.srcrwr == expected_srcrwr


def test_ports(megamind, megamind_port, megamind_grpc_port):
    assert megamind.port == megamind_port
    assert megamind.grpc_port == megamind_grpc_port
