<html lang="en">
   <head>
      <meta charset="UTF-8">
      <title>DHT</title>
      <link rel="stylesheet" type="text/css" href="style.css">
   </head>

   <body>
      <div id="main">
         <h1>DHT 22 temperature/humidity</h1>
         <p>Sensor %sensor_present% operating correctly. </p>
         <p>Temperature: %temperature% &deg;C, humidity: %humidity% &#37; </p>
         <button onclick="location.href = '/';" id="button" style="margin-right: 2em">Home</button>
         <button onclick="location.href = '/dht22.tpl';" id="button" >Reload</button>
      </div>
   </body>
</html>
