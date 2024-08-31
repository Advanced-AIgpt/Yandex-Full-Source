import socket

import pytest
from alice.bass.fixture import Bass


@pytest.fixture
def bass_port():
    return 42


@pytest.fixture
def bass(bass_port):
    return Bass(port=bass_port)


@pytest.fixture
def expected_srcrwr(bass_port):
    return f'BASS:{socket.gethostname()}:{bass_port}'


def test_srcrwr(bass, expected_srcrwr):
    assert bass.srcrwr == expected_srcrwr


def test_ports(bass, bass_port):
    assert bass.port == bass_port
