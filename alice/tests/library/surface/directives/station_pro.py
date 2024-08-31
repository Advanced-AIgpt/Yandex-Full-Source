from . import mixin


class DirectivesMixin(mixin.BaseMixin):
    def setup_rcu(self, data):
        pass

    def setup_rcu_auto(self, data):
        pass

    def setup_rcu_check(self, data):
        pass

    def setup_rcu_advanced(self, data):
        pass

    def setup_rcu_manual(self, data):
        pass

    def setup_rcu_stop(self, data):
        pass

    def draw_led_screen(self, data):
        pass

    def force_display_cards(self, data):
        pass

    def screen_on(self, data):
        pass

    def screen_off(self, data):
        pass
