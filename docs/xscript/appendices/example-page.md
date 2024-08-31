# Пример XML-файла на XScript

Ниже приведен пример XML-файла, написанного на XScript с использованием [CORBA-блоков](../concepts/block-corba-ov.md) и [Mist-блоков](../concepts/block-mist-ov.md).

```
<?xml version="1.0" encoding="windows-1251"?> <?xml-stylesheet href="task.xsl" type="text/xsl"?> <page name="index" xmlns:x="http://www.yandex.ru/xscript">
     <xscript>
         <xslt-param id="http_real_path" 
type="ProtocolArg">realpath</xslt-param>
         <add-headers>
             <header name="Cache-Control" value="max-age=0, proxy-revalidate"/>
         </add-headers>
     </xscript>
     <mist>
         <method>set_state_by_request</method>
         <param type="String">test</param>
     </mist>
     <block>
         <name>Yandex/Assessor/Assessor.id</name>
         <method>checkAuth</method>
         <param type="Request"/>
         <param type="Auth"/>
     </block>
     <block>
         <name>Yandex/Assessor/Assessor.id</name>
         <method>tasksList</method>
         <param type="Request"/>
         <param type="Auth"/>
     </block>
     <main-menu/>
     <mist>
         <method>set_state_by_protocol</method>
         <param type="String"/>
     </mist>
</page>
```

После работы блоков, но до наложения XSL-преобразования страница выглядит следующим образом:

```
<page name="index">
<state prefix="test" type="Request"/>

<xscript_invoke_failed error="CORBA exception" block="block" 
method="checkAuth" object="Yandex/Assessor/Assessor.id" 
exception="TRANSIENT" minor="TRANSIENT_ConnectFailed"/>

<xscript_invoke_failed error="CORBA exception" block="block" 
method="tasksList" object="Yandex/Assessor/Assessor.id" 
exception="TRANSIENT" minor="TRANSIENT_ConnectFailed"/>

<main-menu/>

<state prefix="" type="Protocol">

<param name="path">/asessor/index.xml</param>
<path>/asessor/index.xml</path>

<param name="uri">/asessor/index.xml</param>
<uri>/asessor/index.xml</uri>

<param name="originaluri">/asessor/index.xml</param>
<originaluri>/asessor/index.xml</originaluri>

<param name="originalurl">
https://devel.fireball.yandex.ru:8091/asessor/index.xml
</param>
<originalurl>
https://devel.fireball.yandex.ru:8091/asessor/index.xml
</originalurl>

<param name="host">devel.fireball.yandex.ru:8091</param>
<host>devel.fireball.yandex.ru:8091</host>

<param name="originalhost">
devel.fireball.yandex.ru:8091
</param>
<originalhost>
devel.fireball.yandex.ru:8091
</originalhost>

<param name="realpath">
/opt/lighttpd-xscript5/devel/asessor/index.xml
</param>
<realpath>
/opt/lighttpd-xscript5/devel/asessor/index.xml
</realpath>

<param name="secure">yes</param>
<secure>yes</secure>

<param name="bot">no</param>
<bot>no</bot>

<param name="method">GET</param>
<method>GET</method>

<param name="remote_ip">95.108.174.209</param>
<remote_ip>95.108.174.209</remote_ip>

</state>
</page>
```

Видно, что блоки вернули две ошибки.

После наложения основного XSL-преобразования получается HTML-страница:

```
<html xmlns:fo="http://www.w3.org/1999/XSL/Format">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=windows-1251">
<title>Яndex: Система оценки результатов поиска</title> <link rel="stylesheet" type="text/css" href="/main.css"> <meta name="Description" content=""> <meta name="Keywords" content=""> <meta http-equiv="Cache-Control" content="No-Cache"> </head> <conbody bgcolor="ffffff" leftmargin="0" marginwidth="0" topmargin="0" 
marginheight="0" rightmargin="0" text="000000" link="0000ff" vlink="0000ff"> <script language="JavaScript" src="/open_wnd.js"></script><table
border="0" cellpadding="8" cellspacing="0" width="100%"><tr valign="bottom"> <td><A HREF="/"><img src="http://img.yandex.ru/i/logo76x48.gif" 
alt="Яндекс" border="0" height="48" width="76"></A></td> <td align="right"><table cellpadding="2" cellspacing="0" border="0" 
class="login">
<tr><td align="right"><font class="ltitle">Логин: <b></b> <a href="http://passport.yandex.ru/passport?mode=logout">Выход</a><br><br></font></td></tr>

<tr><td align="right"><font class="ltitle"><a href="/help.xml">Справка</a></font></td></tr>
</table></td>
</tr></table>
<table width="100%" border="0" cellspacing="0" cellpadding="4" 
class="login"><tr bgcolor="ffcc00">
<td class="back"><font class="stitle"></font></td> <td align="right"><b><font class="stitle">Система оценки результатов поиска</font></b></td> </tr></table> <table border="0" cellpadding="8" cellspacing="0" width="100%"><tr valign="top"><td> <p><font color="red">Error:</font>
       <u>object:</u> Yandex/Assessor/Assessor.id
       <u>message:</u> </p>

<p><font color="red">Error:</font>
       <u>object:</u> Yandex/Assessor/Assessor.id
       <u>message:</u> </p>
<state prefix="" type="Protocol"></state> </td></tr></table> <hr size="1" width="100%" style="color: black;"> <table width="100%"><tr> <td width="33%"> </td> <td align="right" valign="top"><font size="-1">Copyright © 2003 «<a href="http://www.yandex.ru/">Яндекс</a>»<br><a
href="mailto:webadmin@yandex.ru">webadmin@yandex.ru</a></font></td>

</tr></table>
<!---->
</conbody>
</html>

```

### Узнайте больше {#learn-more}
* [Обработка запроса](../concepts/xscript-functionality.md)
* [XML-файл на XScript](../concepts/xscript-file-ov.md)
* [Диагностика ошибок в XScript](../concepts/error-diag-ov.md)