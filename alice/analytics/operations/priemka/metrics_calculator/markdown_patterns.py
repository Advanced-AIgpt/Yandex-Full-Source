card_markdown_pattern = '''
<div style="background-color: #FFF; padding: 16px; color: #000;">
        <details style="text-align: right;">
            <summary><b>Состояние станции:</b></summary>
                {extra_info}
        </details>
        {query_or_screenshot}
        <div style="padding: 8px 12px; margin: 12px 0; width: fit-content; border-radius: 16px; background-color: #dedede; border-bottom-left-radius: 0px;">{answer}</div>{action}
        <div><b>Сценарий:</b> {scenario}</div>
</div>
'''


markdown_query_pattern = '''<div
    style="padding: 8px 12px; margin: 12px 0; width: fit-content; border-radius: 16px; background-color: #fce57f; margin-left: auto; border-bottom-right-radius: 0px;">{query}
</div>'''


markdown_screenshot_pattern = '''<img src = "{screenshot_url}" width=100%>'''


markdown_action_pattern = '''
<div style="white-space: pre-wrap"><b>Действие:</b> {action}</div>'''

markdown_extra_pattern = '''
<div style="color: #8f8f8c; text-align: left"><b>{state}:</b> {content}</div>'''

markdown_extra_music_pattern = '''
<div style="color: #8f8f8c; text-align: left"><b>Последняя прослушанная музыка:</b> {content}</div>
                <div style="color: #8f8f8c; text-align: left">{playback}</div>'''

markdown_extra_smart_home = '''
<details style="text-align: left; color: #8f8f8c">
                    <summary><b>Устройства умного дома:</b></summary>
                    <div style="color: #8f8f8c; white-space: pre-wrap">{content}</div>
                </details>'''
