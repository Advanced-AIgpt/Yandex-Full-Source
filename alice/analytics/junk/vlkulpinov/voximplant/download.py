import json
import requests
import shutil
import os

SLEN = len('00:00:00 - 00:00:00 : ')

def get_channels(text):
    result_left = ''
    result_right = ''

    for line in text.split('\n'):
        if len(line) < 4:
            continue
        if line.startswith('Left'):
            result_left += line[len('Left') + 1 + SLEN:].strip() + ' '
        elif line.startswith('Right'):
            result_right += line[len('Right') + 1 + SLEN:].strip() + ' '
        else:
            raise ValueError("empty string")

    return result_left, result_right

def prep_wavs(mp3, base_name, channel):
    logfile = 'log.txt'
    os.system('ffmpeg -i {mp3} -af silencedetect=n=-30dB:d=0.6 -f null - 2> {log}'.format(mp3=mp3, log=logfile))

    periods = []
    prev_silence = None
    with open(logfile, 'r') as f:
        for line in f:
            line2 = line.strip()
            if not '[silencedetect' in line2:
                continue

            if 'silence_start' in line2:
                prev_silence = max(float(line2.split(' ')[-1]), 0) + 0.3
            else:
                periods.append((prev_silence, prev_silence - 0.6 + float(line2.split(' ')[-1])))

    print(periods)

    result = []

    prev_cut = 0.0
    iid = 1
    for period in periods:
        if period[0] - prev_cut < 10 and period[1] - prev_cut > 10:
            dur = 10.0
            wav = 'wav/' + base_name + '_{0:02d}_{1}'.format(iid, channel) + '.wav'
            iid += 1
            print(prev_cut, dur)
            result.append(wav)
            os.system('sox {mp3} -c 1 -r 16000 -b 16 {wav} trim {since:.1f} {dur:.1f}'.format(
                mp3=mp3,
                wav=wav,
                since=prev_cut,
                dur=dur
            ))
            prev_cut = prev_cut + dur
        elif period[0] - prev_cut > 10:
            dur = period[0] - prev_cut
            wav = 'wav/' + base_name + '_{0:02d}_{1}'.format(iid, channel) + '.wav'
            iid += 1
            print(prev_cut, dur)
            result.append(wav)
            os.system('sox {mp3} -c 1 -r 16000 -b 16 {wav} trim {since:.1f} {dur:.1f}'.format(
                mp3=mp3,
                wav=wav,
                since=prev_cut,
                dur=dur
            ))
            prev_cut = prev_cut + dur
        else:
            print('pass')

    wav = 'wav/' + base_name + '_{0:02d}_{1}'.format(iid, channel) + '.wav'
    print(prev_cut)
    result.append(wav)
    os.system('sox {mp3} -c 1 -r 16000 -b 16 {wav} trim {since:.1f}'.format(
        mp3=mp3,
        wav=wav,
        since=prev_cut
    ))

    return result




def main():

    with open('init_audio3.json', 'r') as f:
        init_audios = json.loads(f.read())

    final_audios = []
    file_id = 1

    # folder = './mp3/'
    for folder in ['./mp3/', './wav/']:
        for the_file in os.listdir(folder):
            file_path = os.path.join(folder, the_file)
            try:
                if os.path.isfile(file_path):
                    os.unlink(file_path)
                #elif os.path.isdir(file_path): shutil.rmtree(file_path)
            except Exception as e:
                print(e)

    for item in init_audios:

        r = requests.get(item['audio_url'], stream=True)
        base_name = '{:04d}'.format(file_id)
        mp3_name = 'mp3/' + base_name + '.mp3'
        mp3_name_left = 'mp3/' + base_name + '_left.mp3'
        mp3_name_right = 'mp3/' + base_name + '_right.mp3'
        left_name = 'wav/' + base_name + '_01_left' + '.wav'
        right_name = 'wav/' + base_name + '_01_right' + '.wav'

        if r.status_code == 200:
            with open(mp3_name, 'wb') as f:
                r.raw.decode_content = True
                shutil.copyfileobj(r.raw, f)
                file_id += 1
        else:
            raise ValueError('problem while downloading')

        os.system('lame -b 128 -m l {mp3} {mp3left}'.format(mp3=mp3_name, mp3left=mp3_name_left))
        os.system('lame -b 128 -m r {mp3} {mp3right}'.format(mp3=mp3_name, mp3right=mp3_name_right))
        names_left = prep_wavs(mp3_name_left, base_name, 'left')
        names_right = prep_wavs(mp3_name_right, base_name, 'right')

        print(item['audio_url'])

        for n in names_left:
            final_audios.append({
                'audio_url': item['audio_url'],
                # 'transcription_url': item['transcription_url'],
                'channel': 'left',
                'name': n,
                # 'vox_text': channel_left,
                'yandex_text': '',
                'toloka_text': '',
            })
        for n in names_right:
            final_audios.append({
                'audio_url': item['audio_url'],
                # 'transcription_url': item['transcription_url'],
                'channel': 'right',
                'name': n,
                # 'vox_text': channel_right,
                'yandex_text': '',
                'toloka_text': '',
            })
        # break

        # break

    with open('final.json', 'w') as f:
        f.write(json.dumps(final_audios, ensure_ascii=False, indent=4).encode('utf-8'))

if __name__ == '__main__':
    main()
