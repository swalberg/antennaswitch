const char status_json[] PROGMEM = R"rawliteral(
  {
    "active": %ACTIVE_NUMBER%,
    "antennas": [
      "%ANTENNA0%", "%ANTENNA1%", "%ANTENNA2%", "%ANTENNA3%", "%ANTENNA4%"
    ],
    "debug": "%DEBUG%"
  }

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
    <div><label for="ant0">1</label><input type="text" name="ant0" value="%ANTENNA0%" placeholder="Antenna1 name"></div>
    <div><label for="ant1">2</label><input type="text" name="ant1" value="%ANTENNA1%" placeholder="Antenna2 name"></div>
    <div><label for="ant2">3</label><input type="text" name="ant2" value="%ANTENNA2%" placeholder="Antenna3 name"></div>
    <div><label for="ant3">4</label><input type="text" name="ant3" value="%ANTENNA3%" placeholder="Antenna4 name"></div>
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
    <tr><td><p><a href="#"><button class="button" id="0" onClick="move(this)">USE</button></a></p></td>
    <td><p><a href="#"><button class="button" id="1" onClick="move(this)">USE</button></a></p></td>
    <td><p><a href="#"><button class="button" id="2" onClick="move(this)">USE</button></a></p></td>
    <td><p><a href="#"><button class="button" id="3" onClick="move(this)">USE</button></a></p></td>
    <td><p><a href="#"><button class="button" id="99" onClick="move(this)">USE</button></a></p></td></tr>
  
  </table>
  <h2 id="motorStatus"></h2>
  <h3 id="debug"></h3>
  <script>
  function move(el) {
    var xhr = new XMLHttpRequest();
    console.log(el);
    console.log(el.id);
    xhr.open("GET", "/move?ant=" + el.id);
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
            element.innerHTML = "ACTIVE";
            element.class = "button button2";
          } else {
            element.innerHTML = "USE";
            element.class = "button";
          }
        }
      );
      document.getElementById("debug").innerHTML = j.debug;
    };
    req.send();
  }, 2000);

  </script>
</body>
</html>
)rawliteral";
