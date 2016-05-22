<html lang="en">
	<head>
		<meta charset="UTF-8">
		<title>RELAY</title>
		<link rel="stylesheet" type="text/css" href="style.css">
		<script type="text/javascript">
			function changeState() {
				var el = document.getElementsByName("relay");
				var state = '%relayStatus%';
				switch(state) {
					case "on":
						el[0].value="off";
					break;
					case "off":
						el[0].value="on";
					break;
					default:
						el[0].value="Unkown";
				}
			}
		</script>
	</head>
	<body onload="changeState()">
		<div id="main">
			<h1>RELAY control</h1>
			<p>Relay status: <b>%relayStatus%</b>. </p>
			<form method="get" action="relay.cgi">
				<input type="submit" name="relay" value="on" id="button">
			</form>
			<button onclick="location.href = '/';" id="button">Home</button>
		</div>
	</body>
</html>
