from alice.boltalka.emoji.lib.main import BoltalkaEmojifier

def main():
    print(BoltalkaEmojifier(data_dir='lib/').get_replies_with_emoji(['привет', 'как дела?', 'нормально']))