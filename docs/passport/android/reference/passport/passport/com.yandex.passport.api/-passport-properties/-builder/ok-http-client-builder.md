//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportProperties](../index.md)/[Builder](index.md)/[okHttpClientBuilder](ok-http-client-builder.md)

# okHttpClientBuilder

[passport]\
abstract override var [okHttpClientBuilder](ok-http-client-builder.md): OkHttpClient.Builder

Определяет OkHttpClient.Builder. Удобно использовать для отладки и при необходимости переопределить компоненты сетевого уровня.

`<pre> OkHttpClient.Builder okHttpClientBuilder = new OkHttpClient.Builder() .dns(hostname -> { return Dns.SYSTEM.lookup(hostname); }) .addNetworkInterceptor(new HttpLoggingInterceptor(message -> Log.d(&quot;Network&quot;, message)) .setLevel(HttpLoggingInterceptor.Level.BODY)) .addNetworkInterceptor(new StethoInterceptor());

</pre>`
