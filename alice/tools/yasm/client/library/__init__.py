"""
Библиотека функций для генерации и применения алертов и дашбордов Голована
"""
from alice.tools.yasm.client.library.arguments import get_arguments
from alice.tools.yasm.client.library.alerts import alert_main, alert_wipe
from alice.tools.yasm.client.library.dashboard import dashboard_main
from alice.tools.yasm.client.library.verify import verify_main

__all__ = ["get_arguments", "alert_main", "alert_wipe", "dashboard_main", "verify_main"]
