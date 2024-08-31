from nirvana_api import *
from nirvana_api.blocks import *
from nirvana_api.highlevel_api import *
import time
from datetime import timedelta, date

# nirvana_token = 'robot-voice-qa_nirvana_token'
# nirvana_token = 'irinfox_nirvana_token'
# ToDo: добавить читалку токена из yav
nirvana_token = "YOUR_TOKEN_HERE"
workflow_id='f67eb54f-b0fe-4946-8df3-93aaece7d428'
original_instance = '29785aa8-66ce-4baf-be62-acf0d4a2c9f1'
api = NirvanaApi(oauth_token=nirvana_token)


def daterange(start_date, end_date):
    for n in range(int((end_date - start_date).days)):
        yield start_date + timedelta(n)


def main():
    start_date = date(2020, 4, 20)
    end_date = date(2020, 7, 27)

    for single_date in daterange(start_date, end_date):
        str_date = single_date.strftime("%Y-%m-%d")
        print(str_date)

        new_instance_id = api.clone_workflow_instance(
            workflow_id=workflow_id,
            workflow_instance_id=original_instance
        )

        api.set_global_parameters(
            workflow_id=workflow_id,
            workflow_instance_id=new_instance_id,
            param_values=dict(parameter='date', value=str_date)
        )

        api.start_workflow(
            workflow_id=workflow_id,
            workflow_instance_id=new_instance_id
        )

        time.sleep(180)


if __name__ == '__main__':
    main()
