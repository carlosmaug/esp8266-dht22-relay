<html lang="en">
	<head>
		<meta charset="UTF-8">
		<title>Esp8266 web server</title>
		<link rel="stylesheet" type="text/css" href="style.css">
	</head>
	<body>
		<div id="main">
			<h1>ESP8266<br> DHT22 & RELAY</h1>
			<button onclick="location.href = 'relay.tpl';" id="button" >RELAY<br>control</button>
			<button onclick="location.href = 'dht22.tpl';" id="button" style="margin: 0px 2em;">DHT22<br>readings</button>
			<button onclick="location.href = 'settings.tpl';" id="button" style="vertical-align: bottom; height: 3.3em;">Settings</button>
			<p>Page has been loaded <b>%counter%</b> times. </p>
		</div>
	</body>
</html>
