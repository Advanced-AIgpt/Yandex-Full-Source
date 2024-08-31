import React from 'react';
import Icon from 'antd/es/icon';
import Typography from 'antd/es/typography';

const { Text, Title, Paragraph } = Typography;

const infoText = (<div>
    <Title level={4}> Основные фичи </Title>
    <Paragraph>
        <ul>
            <li>Редактор грамматик</li>
            <li>Поиск по индексам <Text code>Verstehen</Text></li>
            <li>Сбор датасета</li>
            <li>Экспорт грамматики и датасета</li>
            <li>Поиск синонимов для нетерминалов</li>
        </ul>
    </Paragraph>

    <Title level={4}> Подробнее по вкладкам </Title>
    <Paragraph>
        <Text strong>Search</Text> <br/>
        Слева - логи сматченные грамматикой.
        Справа -  логи, похожие на сматченные, но еще не заматченные грамматикой. 
        Можно разнообразить похожие, меняя индекс поиска <Text code>Verstehen</Text> 
        (выпадающее меню в заголовке колонки).
        <br/>
        <br/>
        <Text strong>Validate</Text> <br/>
        В этой вкладке можно зафиксировать датасет для тестирования грамматики. 
        Примеры можно добавлять из вкладки <Text code>Search</Text> или вручную.
        Зафиксированные примеры так же участвуют в поиске по индексу ферштейна:
        positives - как положительные примеры для индекса, negatives - как отрицательные. <br/>
        Предполагается, что в идеальном датасете все примеры из колонки positives подсвечиваются зеленым,
        а negatives - красным (то есть в первой колнке все примеры которые матчатся грамматикой, а во второй - наоборот).
        Если это не так - неправильные примеры будут всплывать наверх, так их проще заметить и поправить. <br/>
        Если вы не очень понимаете, почему пример красный (то есть не заматчился, хотя должен был), можно кликнуть на него -
        по клику всплывет дебаг-лог разбора. <br/>
        После того как вы собраи идеальный датасет его можно экспортировать в файл 
        <Text delete>который возможно даже поддерживается Гранетом</Text> с помощью 
        <Text code>Export</Text>-><Text code>Export dataset...</Text>
        <br/>
        <br/>
        <Text strong>Synonyms</Text> <br/>
        На этой вкладке можно расширить грамматику синонимами, которые вам предложат по заданной фразе. Для этого:
        <ol>
            <li>
                Перенесите фразу из вкладки <Text code>Validate</Text> (<Icon type='more'/> -> 
                <Text code>Search for synonyms</Text>) или добавьте вручную в поле ввода. 
                <Text type='warning'>Важно: фраза должна матчиться грамматикой, т.к. синонимы побираются к
                конкретному нетерминалу</Text>
            </li>
            <li>
                Выбирайте нужную часть фразы сразу под полем ввода, по клику для нее будут выводиться синонимы
            </li>
            <li>
                На хорошую фразу с синонимом можно кликнуть - тогда она добавится в грамматику с комментарием 
                <Text code># auto-generated</Text>
            </li>
        </ol>
        Можно генерить синонимы, учитывая слова из данного нетерминала.
        (Переключатель <Text code>Fetch synonyms from grammar</Text>)<br/>
        Можно генерировать многословные (multi subword) выражения (выбирая желаемое число subword'ов)
        (указывая количество слов в <Text code>Number of words per token</Text>)<br/>
        Можно генерировать, добавляя шум (повышая разнообразие, но уменьшая синонимичность). 
        Регулировать его коэффицент можно по <Text code>Use noise</Text><br/>
        <Text code>Max number of variants per word</Text> регулирует количество отображаемых гипотез. 
    </Paragraph>

    <Title level={4}> Подробнее по настройкам </Title>
    <Paragraph>
        <ul>
            <li>
                <Text code>Show "Add to pos/neg" buttons</Text> Включить показ кнопок +/- для сэмплов из
                <Text code>Search</Text>, они переносят сэмпл на колонки из вкладки <Text code>Validate</Text>.
                То же самое можно делать чекбоксами и переносить батчами.
            </li>
            <li>
                <Text code>Show occurance in search</Text> Показать колонку с относительной частотой по колонке на вкладке <Text code>Search</Text>.
                По ней, например, можно сортировать чтобы видеть какую часть запросов покрывает сэмпл.
            </li>
            <li>
                <Text code>Show classifier score in search</Text> Показать колонку со скором на вкладке <Text code>Search</Text>
                <Text underline>DSSM + LogReg</Text> индекса.
                (Он быстро считается и достаточно показательный). По ней тоже можно сортировать, например, чтобы увидеть
                сэмплы с минимальным скором - они могут быть false-позитивами.
            </li>
            <li>
                <Text code>Show granet slots</Text> Включить отображение слотов на вкладке <Text code>Validate</Text>
            </li>
            <li>
                <Text code>Print values/types for granet slots</Text> Отображать значения и типы слотов на вкладке <Text code>Search</Text>
            </li>
            <li>
                <Text code>Lines to show</Text> Максимальное кол-во сэмплов в колонке.
            </li>
        </ul>
    </Paragraph>



    <Title level={4}> Ссылки </Title>
    <Paragraph>
        <a href="https://wiki.yandex-team.ru/alice/megamind/nlu/granet/">Вики по гранету</a>
        <br/>
        <a href="https://wiki.yandex-team.ru/users/artemkorenev/verstehen/">Вики по ферштейну</a>
    </Paragraph>

    <Title level={4}> Фичреквесты и багрепорты </Title>
    <Paragraph>
        Отправлять сюда: <a href="https://st.yandex-team.ru/DIALOG-6255">тикет</a>
    </Paragraph>
    </div>);

export default infoText;
