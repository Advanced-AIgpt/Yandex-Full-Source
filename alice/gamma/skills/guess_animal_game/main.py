# coding: utf-8
import gamma_sdk.client as client

from .game.game import GuessAnimalSkill


def main():
    client.start_skill(GuessAnimalSkill())


if __name__ == '__main__':
    main()
