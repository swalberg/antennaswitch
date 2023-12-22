const char status_json[] PROGMEM = R"rawliteral(
  {
    "active": %ACTIVE_NUMBER%,
    "antennas": [
      "%ANTENNA1%", "%ANTENNA2%", "%ANTENNA3%", "%ANTENNA4%", "%ANTENNA5%"
    ]
  }

)rawliteral";

const char config_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Antenna rotator</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    input { display: block; }
  </style>
</head>
<body>
  <form action="/config" method="POST">
    <label for="ant1">1</label><input type="text" name="ant1" value="%ANTENNA1%" placeholder="Antenna1 name"><input type="text" name="stop1" value="%STOP1%" placeholder="Antenna1 stop">
    <label for="ant1">2</label><input type="text" name="ant2" value="%ANTENNA2%" placeholder="Antenna2 name"><input type="text" name="stop2" value="%STOP2%" placeholder="Antenna2 stop">
    <label for="ant1">3</label><input type="text" name="ant3" value="%ANTENNA3%" placeholder="Antenna3 name"><input type="text" name="stop3" value="%STOP3%" placeholder="Antenna3 stop">
    <label for="ant1">4</label><input type="text" name="ant4" value="%ANTENNA4%" placeholder="Antenna4 name"><input type="text" name="stop4" value="%STOP4%" placeholder="Antenna4 stop">
    <label for="ant1">5</label><input type="text" name="ant5" value="%ANTENNA5%" placeholder="Antenna5 name"><input type="text" name="stop5" value="%STOP5%" placeholder="Antenna5 stop">
    <input type="submit">
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
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    #status { background-color: #ccc; border-radius: 2px; }
    #status span { padding-left: 25px;}
    label.manualentry { display:block; padding-top: 15px; }
  </style>
</head>
<body>
  <h2>Antenna Switch</h2>

</body>
</html>
)rawliteral";
