from datetime import datetime, timedelta


class BranchParser(object):

    _months = ['января', 'февраля', 'марта', 'апреля', 'мая', 'июня', 'июля', 'августа', 'сентября', 'октября', 'ноября', 'декабря']

    def parse_for_dt(self, dt):
        wd = dt.weekday()

        branches = []

        # MEGAMIND - воскресенье and четверг 02:00
        if wd in [3, 6]:
            branches.append(f'*02:00 {dt.day} {self._months[dt.month - 1]}*: ⚙️ [Megamind + VINS + BASS]')

        # HOLLYWOOD - воскресенье and среда 04:00
        if wd in [2, 6]:
            branches.append(f'*04:00 {dt.day} {self._months[dt.month - 1]}*: 🎭 [Hollywood]')

        # APPHOST -  среда and суббота 01:00
        if wd in [2, 5]:
            branches.append(f'*01:00 {dt.day} {self._months[dt.month - 1]}*: 🕸 [графы AppHost Алисы]')

        # UNIPROXY - пятница-воскресенье вечер
        if wd == 4:
            branches.append(f'*вечер {dt.day} {self._months[dt.month - 1]} - {(dt+timedelta(days=2)).day} {self._months[(dt+timedelta(days=2)).month - 1]}*: 🎤 [Uniproxy]')

        # BEGEMOT - понедельник-четверг вечер
        if wd in [0, 3]:
            branches.append(f'*вечер {dt.day} {self._months[dt.month - 1]}*: 🦛 [Begemot]')

        return branches

    def parse(self):
        msg = 'Список отвода веток в будущие дни:\n\n'

        branches = []

        now = datetime.now()
        for day_delta in range(8):
            dt = now + timedelta(days=day_delta)
            branches.extend(self.parse_for_dt(dt))

        msg += '\n'.join(branches)
        return msg
