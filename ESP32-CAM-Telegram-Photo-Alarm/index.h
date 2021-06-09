// объявляем Web страницу для настройки параметров работы этой Telegram сигнализации
const char WebPage [] PROGMEM = R"=====(
<HTML>
  <HEAD>
      <TITLE>ESP32-CAM Alarm</TITLE>
      <meta charset="utf-8">   
  </HEAD>
  <BODY>
    <CENTER>
    <h1 style="font-family:Helvetica;font-size:42;margin-bottom:-10;color:#0095ff">ESP32-CAM Alarm</h1>
  <form class="form-horizontal">
    <div class="form-group">
      <label class="col-md-4 control-label" for="network"></label>  
      <div class="col-md-6">
        <p><input id="network" name="network" required type="text" spellcheck="false" style="margin-bottom:-3;border-style:solid;border-color:#ffaa00;color:#ffaa00;font-family:Helvetica;font-size:32;font-weight:bold;" placeholder="название WiFi сети"></p>
      </div>
    </div>

    <div class="form-group">
      <label class="col-md-4 control-label" for="password"></label>  
      <div class="col-md-6">
        <p><input id="password" name="password" required type="password" style="margin:-3;border-style:solid;border-color:#0095ff;color:#0095ff;font-family:Helvetica;font-size:32;font-weight:bold;" minlength="8" placeholder="пароль WiFi сети"></p>
      </div>
    </div>

    <div class="form-group">
      <label class="col-md-4 control-label" for="bot_token"></label>  
      <div class="col-md-6">
        <p><input id="bot_token" name="bot_token" required type="text" spellcheck="false" style="margin:-3;border-style:solid;border-color:#9500ff;color:#9500ff;font-family:Helvetica;font-size:32;font-weight:bold;" placeholder="токен Telegram бота"></p>
      </div>
    </div>

     <div class="form-group">
      <label class="col-md-4 control-label" for="chat_id"></label>  
      <div class="col-md-6">
        <p><input id="chat_id" name="chat_id" required type="password" spellcheck="false" style="margin:-3;border-style:solid;border-color:#ff004c;color:#ff004c;font-family:Helvetica;font-size:32;font-weight:bold;" placeholder="ваш chat id">
      </div>
    </div>

    <div class="form-group">
      <label class="col-md-4 control-label" for="pir_delay"></label>  
      <div class="col-md-6">
        <p><input id="pir_delay" name="pir_delay" required type="number" step="100" style="margin:-3;border-style:solid;border-color:#0d00ff;color:#0d00ff;font-family:Helvetica;font-size:32;font-weight:bold;" min="3000" placeholder="задержка PIR датчика"></p>
      </div>
    </div>

  <div class="form-group">
      <label class="col-md-4 control-label" for="flashon"></label>
      <div class="col-md-8">
        <button id="flashon" name="flashon" style="font-weight:bold;font-size:40.5;margin-top:-3;margin-right:5;background-color:#18c746;border-width:4;border-color:#000000" class="btn btn-success">flash on</button>
        <button id="flashoff" name="flashoff" style="font-weight:bold;font-size:40.5;margin-top:-3;background-color:#ff0000;border-width:4;border-color:#000000" class="btn btn-danger">flash off</button>
      </div>
    </div>
    </CENTER>
  </BODY>
</HTML>
)=====";

// объявляем текст, который будет отображается при успешной записи ваших параметров работы в SPIFFS память
const char SPIFFSWrite [] PROGMEM = R"=====(
  <HTML>
    <HEAD>
      <meta charset="utf-8"> 
    </HEAD>
    <BODY>
      <CENTER>
        <h1 style="font-family:Helvetica;margin-top:6;font-size:24;margin-top:3;color:#0095ff">Параметры работы записаны!</h1>
      </CENTER> 
    </BODY>
  </HTML>
)=====";

// объявляем текст, который будет отображаться при ошибке записи ваших параметров работы в SPIFFS память
const char SPIFFSError [] PROGMEM = R"=====(
  <HTML>
    <HEAD>
      <meta charset="utf-8">
    </HEAD>
    <BODY>
      <CENTER>
        <h1 style="font-family:Helvetica;margin-top:6;font-size:23.6;margin-top:3;color:#ff0000">Не удалось записать в SPIFFS!</h1>
      </CENTER> 
    </BODY>
  </HTML>
)=====";
