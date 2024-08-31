from abc import ABC, abstractmethod

from cached_property import cached_property


class Response(ABC):

    @property
    @abstractmethod
    def text(self):
        '''Текстовый ответ Алисы'''

    @abstractmethod
    def has_voice_response(self):
        '''Есть ли голосовой ответ Алисы'''

    @property
    @abstractmethod
    def voice_response(self):
        '''Объект голосового ответ Алисы'''

    @property
    @abstractmethod
    def output_speech_text(self):
        '''Голосовой ответ Алисы текстом'''

    @property
    @abstractmethod
    def scenario(self):
        '''Имя выигравшего сценария из аналитической информации'''

    @property
    @abstractmethod
    def product_scenario(self):
        '''Продуктовое имя сценария из аналитической информации'''

    @property
    @abstractmethod
    def intent(self):
        '''Интент сценария из аналитической информации'''

    @property
    @abstractmethod
    def directives(self):
        '''Клиентские директивы'''

    @cached_property
    def directive(self):
        return self.directives[0] if self.directives else None

    @property
    @abstractmethod
    def cards(self):
        '''Карточки Алисы: текстовы и дивные'''

    @cached_property
    def card(self):
        return self.cards[0] if self.cards else None

    @property
    @abstractmethod
    def raw(self):
        '''Доступ к сырому объекту ответа'''
