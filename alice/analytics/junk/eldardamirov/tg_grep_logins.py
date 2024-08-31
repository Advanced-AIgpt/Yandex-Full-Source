
from telethon import TelegramClient
from telethon.tl.functions.messages import GetDialogsRequest
from telethon.tl.functions.channels import GetParticipantsRequest
from telethon.tl.types import ChannelParticipantsSearch, InputPeerEmpty
from telethon.tl.types import PeerChannel
import pandas as pd

import asyncio

#####################
# fill in, according to: https://core.telegram.org/api/obtaining_api_id
api_id = 0123 
api_hash = 'tg_api_hash' 
username = 'tg_username' # your username
#####################

username_list = []

async def main():
    async with TelegramClient('tmp3', api_id, api_hash) as client:

        chats = []
        last_date = None
        chunk_size = 200
        groups=[]

        result = await client(GetDialogsRequest(
                     offset_date=last_date,
                     offset_id=0,
                     offset_peer=InputPeerEmpty(),
                     limit=chunk_size,
                     hash = 0
                 ))
        chats.extend(result.chats)

        for chat in chats:
            try:
                if chat.megagroup == True:
                    groups.append(chat)
            except:
                continue

        print('Choose a group to scrape members from:')
        i = 0
        for g in groups:
            print(str(i) + '- ' + g.title)
            i += 1

        for chat in chats:
            try:
                if chat.megagroup == True:
                    groups.append(chat)
            except:
                continue

        g_index = input("Enter a Number: ")
        target_group = groups[int(g_index)]

        print('Fetching Members...')
        all_participants = []
        all_participants = await client.get_participants(target_group)

        for user in all_participants:
            print(user.username)
            username_list.append(user.username)

        
if __name__ == '__main__':
  loop = asyncio.get_event_loop()
  loop.run_until_complete(main())
  loop.close()


filename = input("Enter desired filename: ")

usernames =  pd.DataFrame(data={'tg_username': username_list})
usernames.to_csv(filename, encoding='utf-8', index=False)
