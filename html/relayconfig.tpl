<html lang="en">
	<head>
		<meta charset="UTF-8">
		<title>Relay configuration</title>
		<link rel="stylesheet" type="text/css" href="style.css">
		<script type="text/javascript">
			function changeState() {
				if ('%status%' == 'off') {
					document.getElementById('off').checked = true;
				} else {
					document.getElementById('on').checked = true;
				}
			}
		</script>
	</head>
	<body onload="changeState()">
		<div id="main">
			<h1>Relay settings</h1>
			<form name="config" action="relayconfig.cgi" method="get" style="display: inline;">
			<p>Relay: <label><input type="radio" name="relay" id="on" value="on" checked>On</label>
				<label><input type="radio" name="relay" id="off" value="off">Off</label> </p>
			<p>Maximun humidity to trigger realy <input type="number" name="humidity" value="%humidity%" min="0" max="99"> &#37;</p>
			<p>Maximun temparature to trigger realy <input type="number" name="temperature" value="%temperature%" min="-40" max="80"> &deg;C</p>
			<p>Time to turn off realy automatically <input type="number" name="time" value="%time%" min="1" max="120"> min</p>
			<input type="submit" name="connect" value="save" id="button" style="margin-right: 2em;">
			</form>
			<button onclick="location.href = '/settings.tpl';" id="button">Home</button>
		</div>
	</body>
</html>
