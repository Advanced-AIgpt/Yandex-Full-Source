# Требования для разработчика CORBA-компонентов

Разработчик CORBA-компонентов для использования XScript должен соблюдать следующие правила и соглашения:

- **ОС**: хотя выполняться CORBA-компоненты будут под управлением _Debian GNU/Linux_ на разработческих серверах, разработчики вольны использовать десктопные ОС на своих рабочих станциях - Linux _Slackware_, _Ubuntu_, _FreeBSD_, _Windows_.
- Методы CORBA-объектов, вызываемые из XScript-блоков, должны возвращать значение типа XMLSrting; для C++ это значит:
    ```
    #include <yandex/string_helpers.h>
    // Создаём XML
    std::string str;
    ...
    // Возвращаем его в XScript
    return String2XMLString(str);
    ```
    для Java это значит:
    ```
    (hl cpp)
    byte[] out = "<test/>".getBytes("UTF-8");
    return out;
    ```
    
- В Яндексе CORBA-компоненты разрабатываются на языках Java, C++ и Python.
    Для разработки CORBA-компонентов на _Java_ используются библиотеки [OpenORB](http://openorb.sourceforge.net/) и [JacORB](http://www.ociweb.com/products/jacorb).
    
    Для разработки на _C++_ используется библиотека [OmniORB](http://omniorb.sourceforge.net/) с TAO.
    
    Для разработки на _Python_ также используется [OmniORB](http://omniorb.sourceforge.net/) и в качестве "обвязки" разработанная в Яндексе библиотека _ycorba_.
    
- Настоятельно рекомендуется, чтобы время отклика серванта на метод не превышало 1 секунду;
- Если сервант выступает клиентом других сервантов, то он обязан устанавливать тайм аут на вызов методов, и при этом корректно отслеживать ситуацию не-ответа. В библиотеке omniORB это делается директивой `clientCallTimeOutPeriod = <время_в_миллисекундах>`.
- Для сигнализации о внутренней ошибке серванта используется исключение  описанное в [xscript.idl](https://svn.yandex.ru/websvn/wsvn/xscript/xscript-corba/trunk/idl/xscript.idl). Возвращение в XScript кода вида `<error message="blah-blah">` допускается только в случае ошибки, вызванной внешними, по отношению к системе в целом, условиям - например, если пользователь ввёл некорректные данные. Если же ошибка вызвана тем, что произошла внутренняя ошибка XScript, то сервант обязан вернуть исключение в XScript. Это вводится для того, что бы такая ситуация отслеживалась максимально быстро и применялись меры по её устранению.
- Выдаваемый сервантом XML обязан в первом теге нести информацию о собственной версии. Версия должна совпадать с номером версии Debian-пакета. Скрипт `src/mkversion/mkver` создаёт два файла: `debian_version.h` и `debian_version.cpp`. Добавлен Patch-уровень и Build-уровень.
- Сервант обязан писать лог работы в `/var/log/corba/<servant>.log` и не должен удалять свои логи после перезапуска; инициализация поддержки лога на Java:
    ```
    import org.apache.log4j.Logger;
    import ru.yandex.util.LogSignalHandler;
    ...
    public static void main (String args[])
    {
    String logFile = settings.getProperty("testproject.log.file",
    "/var/log/corba/testproject.log");
    String log4jConfig = settings.getProperty("testproject.home.dir", ".")
    +"/"+settings.getProperty("testproject.log4j.config", "log4j-config.xml");
    LogSignalHandler.startLog(logFile, log4jConfig);
    Reloadable servant = ...
    LogSignalHandler.install(servant, logFile, log4jConfig);
    ...
    
    ```
    
    а для записи в лог:
    ```
    class Test { private static Logger logger = Logger.getLogger(Test.class); Test ()
    {
    logger.debug("test");
    }
    }
    ```
    
- Сервант должен поддерживать корректную полную реинициализацию и восстановление после любого внешнего обращения, вызвавшего сбой. На Java:
    ```
    private Yandex.AuthFactory authFactory;
    private void initAuthFactory () throws
    org.omg.CosNaming.NamingContextPackage.InvalidName,
    org.omg.CosNaming.NamingContextPackage.CannotProceed,
    org.omg.CosNaming.NamingContextPackage.NotFound
    {
    String authFactoryPath = settings.getProperty("testproject.ns.authFactory.path",
    "Yandex/Auth/Factory.id");
    org.omg.CORBA.Object obj = orb.string_to_object(
    settings.getProperty("testproject.nameService"));
    NamingContextExt rootContext = NamingContextExtHelper.narrow(obj);
    synchronized (this)
    {
    authFactory = Yandex.AuthFactoryHelper.narrow(
    rootContext.resolve_str(authFactoryPath));
    }
    }
    ```
    
- нужно использовать только там, где это действительно необходимо. Не допускается использование synchronized в объявлении метода, экспортируемого в IDL.
- **ODBC/JDBC**: для обращения к базе данных на Java рекомендуется использовать _JDBC pool_. В качестве метода, предотвращающего утечку соединений из пула, можно использовать следующий код (_Java_):
    ```
    import java.sql.*;
    class Test
    {
    private Test () { }
    static String mySelect (Connection conn, String str)
    {
    PreparedStatement pstmt = null;
    try {
    pstmt = conn.prepareStatement("SELECT ...");
    pstmt.setString(1, str);
    pstmt.execute();
    ResultSet rs = pstmt.executeQuery();
    if (rs.next()) {
    return ...
    }
    } catch (SQLException ex) {
    ...
    } finally {
    try {
    if (pstmt != null) pstmt.close();
    } catch (SQLException e) { }
    }
    return null;
    }
    >}
    ```
    
- **Ведение лог-файла**: сервант обязан отражать свою работу в логе, размещение и регламент работы которого чётко регламентированы: каждый вызов метода и каждая ошибка должны заносится в лог, логи помещаются в `/var/log/corba/$SERVANT.log`, раз в сутки происходит ротация лога (лог за предыдущие сутки запаковывается в gzip, файл лога пересоздаётся). Для этого скрипт, который сопровождает компонент, должен раз в сутки генерировать сигнал SIGHUP, а компонент должен его обрабатывать, переоткрывая лог.
- Сервант должен обрабатывать сигнал SIGTERM — завершение работы.
- Скомпилированный исполняемый файл помещается в `/usr/lib/yandex/$SERVANT-bin`.
- Все серванты упаковываются в Debian-пакеты и выполняются в среде Debian package management system, или `dpkg`, операционной системы _Debian GNU/Linux_, при этом разработчик вовсе не обязан устанавливать эту ОС на своём рабочем компьютере.
- Для написания приёмочных тестов (acceptance tests) параметр функции InitHandlers должен быть не зашит в коде, а браться из конфигурационного файла, например:
    ```
    InitHandlers(theConfig::Instance().getValue("SERVANT_NAME"), orb);
    ```
    
    Это делается для того, чтобы можно было запустить вторую копию серванта для проведения теста.
    
- Все сообщения и прочие ресурсы, особенно на русском языке, должны содержаться в XSL, а не в коде.
- Название серванта должно браться из конфигурационного файла под именем SERVANT_NAME. При инициализации серванта оно используется в трех местах: `InitHandlers`, `create_POA`, `string_to_ObjectId`.
- Путь к серванту должен быть унесен в конфигурационный файл под именем `SERVANT_PATH`.
- Путь к другим сервантам должны быть также перенесены в конфигурационный файл.
- Так как для работы консольных программ теперь может понадобиться конфигурационный файл, доступный по переменной среды YANDEX_CONFIG, следует добавить в  команду `ln -sf` на конфигурационный файл, текущий для данного сервера по именем `servant.cfg`, а в `rm -f` этого файла.
- **Запускающий скрипт**: как правило – симлинк на `/usr/bin/start_servant.sh`, помещается в `/usr/bin/$SERVANT.sh`.
- Скрипт, выставляющий переменные окружения (PYTHONPATH=..., YANDEX_XML_CONFIG=... NLS_LANG=...), помещается в `/etc/yandex/$SERVANT/$SERVANT.cfg`.
- Pid-файл (если файл есть, сервант запускаться не должен. когда сервант завершается – pid-файл должен удаляться) помещается в `/var/run/corba/$SERVANT.pid`.
- Предусмотрен механизм пингера, которому каждый сервант обязан уметь отвечать на его периодические пинги, что он работает. Для нового серванта необходимо настроить его поддержку:`/etc/yandex/pinger.d/$SERVANT` = $ping{c_balance_bars} = 'Yandex/Balance/Bars.id'.

### Узнайте больше {#learn-more}
* [Общие сведения](../concepts/xscript-ov.md)
* [Обработка запроса](../concepts/xscript-functionality.md)
* [http://www.debian.org/doc/debian-policy](http://www.debian.org/doc/debian-policy)
* [Введение](https://doc.yandex-team.ru/Debian/deb-pckg-guide/concepts/About.html)