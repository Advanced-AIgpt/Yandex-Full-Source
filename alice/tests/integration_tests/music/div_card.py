from alice.tests.library.vins_response import DivWrapper


class MusicDivCard(DivWrapper):
    @property
    def title(self):
        return self.data[1].title

    @property
    def action_url(self):
        return self.data[0].action.url

    @property
    def table_text(self):
        return self.data.table.first.text

    @property
    def author(self):
        return self.data.raw.blocks[1].text
