import alice.uniproxy.library.global_state as global_state


def test_initial_state():
    global_state.GlobalState.init()
    assert not global_state.GlobalState.is_listening()
    assert not global_state.GlobalState.is_ready()


def test_listening():
    global_state.GlobalState.init()
    global_state.GlobalState.set_listening()
    assert global_state.GlobalState.is_listening()
    assert not global_state.GlobalState.is_ready()
    assert global_state.GlobalState.is_offline()
    assert not global_state.GlobalState.is_online()


def test_ready():
    global_state.GlobalState.init()
    global_state.GlobalState.set_ready()
    assert not global_state.GlobalState.is_listening()
    assert global_state.GlobalState.is_ready()
    assert global_state.GlobalState.is_offline()
    assert not global_state.GlobalState.is_online()


def test_online():
    global_state.GlobalState.init()
    global_state.GlobalState.set_listening()
    global_state.GlobalState.set_ready()

    assert global_state.GlobalState.is_listening()
    assert global_state.GlobalState.is_ready()
    assert global_state.GlobalState.is_online()
    assert not global_state.GlobalState.is_offline()
