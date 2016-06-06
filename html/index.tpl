<html lang="en">
   <head>
      <meta charset="UTF-8">
      <title>DHT</title>
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
         <h1>ESP8266</h1>
         <p>DHT22 sensor %sensor_present% operating correctly. </p>
         <p>Temperature: <b>%temperature% &deg;C</b>, humidity: <b>%humidity% &#37;</b> </p>
         <form method="get" action="relay.cgi">
	 <p>Relay status: <b>%relayStatus%</b>. 
            <input type="submit" name="relay" value="on" id="button"> </p>
         </form>
         <button onclick="location.href = 'index.tpl';" id="button" style="vertical-align: bottom; height: 3.3em;">Reload</button>
         <button onclick="location.href = 'settings.tpl';" id="button" style="vertical-align: bottom; height: 3.3em;">Settings</button>
      </div>
   </body>
</html>
