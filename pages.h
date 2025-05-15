const char status_json[] PROGMEM = R"rawliteral(
  {
    "active": %ACTIVE_NUMBER%,
    "antennas": [
      "%ANTENNA0%", "%ANTENNA1%", "%ANTENNA2%", "%ANTENNA3%", "%ANTENNA4%"
    ],
    "manual": %MANUAL%,
    "debug": "%DEBUG%"
  }

)rawliteral";

const char config_wifi[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
  <html>Hello from ESP8266 at %IPADDR%!!!
  <p>%FOUND_NETWORKS%</p>
  <form method='post' action='/config_wifi'>
  <label>SSID: </label><input name='ssid' length=32>
  <label>Password: </label><input name='pass' length=64>
  <label>Hostname</label><input name='hostname'><input type='submit'></form>
  </html>
)rawliteral";

const char config_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Antenna switch</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial;}
  </style>
</head>
<body>
  <form action="/config" method="POST">
    <table>
    <tr><th>#</th><th>Name</th><th>6M</th><th>10M</th><th>12M</th><th>15M</th><th>17M</th><th>20M</th><th>30M</th><th>40M</th><th>60M</th><th>80M</th><th>160M</th></tr>
    <tr><td><label for="ant0">1</label></td><td><input type="text" name="ant0" value="%ANTENNA0%" placeholder="Antenna1 name">%ANTENNA0BANDS%</tr>
    <tr><td><label for="ant1">2</label></td><td><input type="text" name="ant1" value="%ANTENNA1%" placeholder="Antenna2 name">%ANTENNA1BANDS%</tr>
    <tr><td><label for="ant2">3</label></td><td><input type="text" name="ant2" value="%ANTENNA2%" placeholder="Antenna3 name">%ANTENNA2BANDS%</tr>
    <tr><td><label for="ant3">4</label></td><td><input type="text" name="ant3" value="%ANTENNA3%" placeholder="Antenna4 name">%ANTENNA3BANDS%</tr>
    </table>
    <div><input type="submit"></div>
  </form>
</body>
</html>
)rawliteral";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Antenna switch</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    xxp {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;}
    .button2 {background-color: #77878A;}
    .redbg { background-color: red;}
  </style>
</head>
<body>
  <h1>Remote Antenna Switch - %SWITCH_NAME%</h1>
  <h2>%FLASH_MESSAGE%</h2>
  <table>
  <tr><td>%ANTENNA0%</td><td>%ANTENNA1%</td><td>%ANTENNA2%</td><td>%ANTENNA3%</td><td>GROUND</td></tr>
    <tr><td><p><a href="#"><button class="button" id="0" onClick="change(this)">USE</button></a></p></td>
    <td><p><a href="#"><button class="button" id="1" onClick="change(this)">USE</button></a></p></td>
    <td><p><a href="#"><button class="button" id="2" onClick="change(this)">USE</button></a></p></td>
    <td><p><a href="#"><button class="button" id="3" onClick="change(this)">USE</button></a></p></td>
    <td><p><a href="#"><button class="button" id="99" onClick="change(this)">USE</button></a></p></td></tr>
  
  </table>
  <h2 id="motorStatus"></h2>
  <h3 id="debug"></h3>
  <p>[<a href="/config">Configure ports</a>] [<a href="/config_wifi">Configure wifi</a>]</p>
  <script>
  function change(el) {
    var xhr = new XMLHttpRequest();
    console.log(el);
    console.log(el.id);
    xhr.open("GET", "/change?ant=" + el.id);
    xhr.send();
  }

  setInterval(function () {
    var req = new XMLHttpRequest();
    req.responseType = 'json';
    req.open('GET', "/status.json", true);
    req.onload  = function() {
      var j = req.response;
    
      Array.from(document.getElementsByClassName("button")).forEach(
        function(element, index, array) {
          if (index == j.active) {
            if (j.manual) {
              element.innerHTML = "LOCKED";
            } else {
              element.innerHTML = "ACTIVE";
            }
            element.class = "button button2";
          } else {
            element.innerHTML = "USE";
            element.class = "button";
          }
        }
      );
    };
    req.send();
  }, 2000);

  </script>
</body>
</html>
)rawliteral";
