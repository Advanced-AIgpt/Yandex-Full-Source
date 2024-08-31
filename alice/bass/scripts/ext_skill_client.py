import argparse
import json
import requests
import uuid


def chat_loop(args):
    prefix = 'bass_ext_skill_client_'
    user_id = args.user or prefix + str(uuid.uuid4())
    session_id = args.session or prefix + str(uuid.uuid4())

    print('=' * 128)
    print('External skill dialog session with skill {}'.format(args.url))
    print('''

        ........
       .        .
      .          .
     .     /\\     .
    .     /  \\     .
    .    /    \\    .
    .   /      \\   .
    .   \\______/   .
     .            .
      .          .
       .        .
        ........

Started a dialog sesssion with a skill instance
To resume current session next time just run "python ext_skill_client.py --user {} --session {} {}"

To exit just press Ctrl+C
'''.format(user_id, session_id, args.url))

    message_id = 0
    while True:
        line = raw_input('You: ')
        url = args.url

        request = {
            'session': {
                'version' : 1,
                'new' : (message_id == 0),
                'user_id' : user_id,
                'session_id' : session_id,
                'skill_id' : '0',
            },
            'request':{
                'type' : 'SimpleUtterance',
                'command' : line,
                'original_utterance' : line,
            }
        }
        r = requests.post(args.url, data=json.dumps(request))

        if (r.status_code != 200):
            print('SKILL ERROR: ' + r.reason)
        else:
            response = json.loads(r.text)
            print('Alice: ' + response['response']['text'])

        message_id += 1


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('url', help='Skill URL')
    parser.add_argument('--user', help='Predefined user ID')
    parser.add_argument('--session', help='Predefined session ID')
    return parser.parse_args()


def main(args):
    chat_loop(args)


if __name__ == '__main__':
    main(parse_args())
