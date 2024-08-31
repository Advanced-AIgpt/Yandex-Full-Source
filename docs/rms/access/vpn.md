# Как настроить VPN

{% list tabs %}

- MacOS

  1. [Проверьте почту](https://mail.yandex-team.ru/#search?request=vpn%20от%3Afintech-infra%40yandex-team.ru) — возможно вам уже запросили доступ к банковскому VPN.

     Если не нашлось, [заведите тикет](https://st.yandex-team.ru/createTicket?queue=FINTECHADMIN&_form=102696) с тегом `дежурство`.

  2. В письме будет ссылка на секрет. Откройтё её и скачайте `YaBank_NPE.tblk.zip`.
  3. Распакуйте скачанный архив и откройте файл `*.tblk` в Tunnelblick.
  4. Откройте **Детали VPN** в Tunnelblick, выберите обычный яндексовский VPN (yandex) и откройте настройки.
  5. В строчке **Установите DNS/WINS** выберите **Не устанавливать DNS-сервер**.
  6. Откройте в текстовом редакторе файл:
     ```
     ~/Library/Application\ Support/Tunnelblick/Configurations/yandex.tblk/Contents/Resources/connected.sh
     ``` 
     Найдите в нём строчку: 
     ```bash
     TUNNEL_DEVICE="$( echo 'show State:/Network/OpenVPN' | scutil  | grep TunnelDevice | awk '{ print $3 }' )"
     ```
     Замените эту строчку на:
     ```bash
     YANDEX_IPV6_CHECK_ADDR="2a02:6b8::1"
     TUNNEL_DEVICE="$( route -n get -inet6 ${YANDEX_IPV6_CHECK_ADDR} | grep interface: | awk '{print $2}' )"
     ```
  7. Подключите VPN Yandex Bank NPE в Tunnelblick. Если появится окошко с просьбой "обезопасить конфигурацию", введите свой пароль и снова запустите подключение.

- Windows
  1. [Проверьте почту](https://mail.yandex-team.ru/#search?request=vpn%20от%3Afintech-infra%40yandex-team.ru) — возможно вам уже запросили доступ к банковскому VPN.

     Если не нашлось, [заведите тикет](https://st.yandex-team.ru/createTicket?queue=FINTECHADMIN&_form=102696) с тегом `дежурство`.
  2. В письме будет ссылка на секрет. Откройтё её и скачайте `YaBank_NPE.ovpn`.
  3. Скачайте и установите клиент по этой [ссылке](https://openvpn.net/community-downloads/) (Windows 64-bit MSI Installer).
  4. Импортируйте конфиг `YaBank_NPE.ovpn` (Import file) и попробуйте подключить VPN.

    Если в windows через OpenVpn не получается создать второе VPN-соединение, то надо выполнить **с правами администратора** файл
    `C:\Program Files\TAP-Windows\bin\addtap.bat` - будет создан еще один tap-адаптер.

- Ubuntu
  1. [Проверьте почту](https://mail.yandex-team.ru/#search?request=vpn%20от%3Afintech-infra%40yandex-team.ru) — возможно вам уже запросили доступ к банковскому VPN.

     Если не нашлось, [заведите тикет](https://st.yandex-team.ru/createTicket?queue=FINTECHADMIN&_form=102696) с тегом `дежурство`.
  2. В письме будет ссылка на секрет. Откройтё её и скачайте `YaBank_NPE.ovpn`.
  3. Загрузите профиль `YaBank_NPE.ovpn` в OpenVPN и попробуйте подключить VPN. [Примеры](https://www.digitalocean.com/community/tutorials/how-to-set-up-and-configure-an-openvpn-server-on-ubuntu-20-04-ru).

  {% cut "Пример настройки в Ubuntu 18.04, Network Manager" %}
    
    - Добавляем новый VPN:
      ![](https://jing.yandex-team.ru/files/jolfzverb/vpn1.png =400x)
    - Выбираем импортировать сохраненную конфигурацию:
      ![](https://jing.yandex-team.ru/files/jolfzverb/vpn2.png =400x)
    - выбираем файл профиля (ovpn файл, скачанный из yav), сохраняем конфигурацию:
      ![](https://jing.yandex-team.ru/files/jolfzverb/vpn3.png =400x)
    - для **Яндексового** vpn отключаем установку DNS для IPv4:
      ![](https://jing.yandex-team.ru/files/jolfzverb/vpn4.png =400x)
      Затем для IPv6:
      ![](https://jing.yandex-team.ru/files/jolfzverb/vpn5.png =400x)
    - подключаем Яндексовый VPN, подключаем Банковский, проверяем что все работает, зайдя на внутренний сервис, например https://gitlab.npe.yabank-team.net (не у всех групп есть доступ туда, правда)

  {% endcut %}


{% endlist %}

После этого у вас должен появиться доступ в админку

## Что дальше

* [Ознакомьтесь](../case-management/quickstart.md) с админкой Case Managemnt

## Решение проблем

Если после получения доступа у вас не видны события или выдаётся ошибка, вам поможет rusakovdv@.
