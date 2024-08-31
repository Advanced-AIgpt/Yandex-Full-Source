def test_no_auth(jupyterhub):
    response = jupyterhub.request('GET', '/test/whoami')
    response.raise_for_status()
    data = response.json()

    assert data == {
        'admin': False,
        'auth_type': 'unauthentificated',
        'client_name': 'unauthentificated',
        'client_type': 'unauthentificated'
    }
