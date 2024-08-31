# coding: utf-8


def set_bass_session_state(session, bass_state):
    session.set('bass_session_state', bass_state, persistent=True)


def get_bass_session_state(session):
    return session.get('bass_session_state')
