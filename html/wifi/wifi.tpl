<html lang="en">
	<head>
		<meta charset="UTF-8">
		<title>WiFi settings</title>
		<link rel="stylesheet" type="text/css" href="../style.css">
		<script type="text/javascript" src="wifi.js"></script>
		<script type="text/javascript">
			window.onload = function() {
			        if ("%WiFiMode%" == "AP only") {
			                document.getElementById('notAP').style.visibility = 'hidden';
			                document.getElementById('notAP').style.display = 'none';
			        } else {
			                document.getElementById('softAP').style.visibility = 'hidden';
			                document.getElementById('softAP').style.display = 'none';
			                scanAPs('%currSsid%');
			        }
			};
		</script>
	</head>
	<body>
		<div id="main">
			<h1>WIFI Configuration</h1>
			<p>Current WiFi mode: <b>%WiFiMode%</b> </p>
			<div id="softAP" style="text-align: center;">
				<p>ESP8266 is acting as an Access Point so it can't connect to other WIFI networks</p>
				<p>Change to: </p>
				<p><button onclick="location.href = 'setmode.cgi?mode=03';" id="button" >AP + Station</button>
				<button onclick="location.href = 'setmode.cgi?mode=01';" id="button" >Station only</button> </p>
			</div>
			<div id="notAP" style="display: inline;">
				<p>Change to: </p>
				<p><button onclick="location.href = 'setmode.cgi?mode=02';" id="button" >SoftAP</button></p>
				<p>Select a network:</p>
				<form name="wifiform" action="connect.cgi" method="post" style="display: inline;">
					<div id="aps">Scanning...</div>
					<p>WiFi password, if applicable: </p>
					<p><input type="text" name="passwd" value="%WiFiPasswd%" id="button"> </p>
					<input type="submit" name="connect" value="Connect" id="button" style="margin-right: 2em;">
				</form>
			</div>
			<button onclick="location.href = '/settings.tpl';" id="button" >Home</button>
		</div>
	</body>
</html>
