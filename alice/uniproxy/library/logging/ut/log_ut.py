from alice.uniproxy.library.logging import Logger


def test_log():
    Logger.init("unitest", True)
    Logger.get().info(u"\u2122")
