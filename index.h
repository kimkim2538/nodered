const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<body>

<div id="demo">
<h1>The ESP8266 NodeMCU AJAX</h1>
	<button type="button" onclick="sendData1(1)">manual ON</button>
	<button type="button" onclick="sendData1(0)">manual OFF</button><BR>
  <button type="button" onclick="sendData2(1)">pump manual ON</button>
  <button type="button" onclick="sendData2(0)">pump manual OFF</button><BR>
</div>

<div>
	  Flow Value is : <span id="FlowValue">0</span><br>
    Distan Value is : <span id="DisValue">0</span><br>
    manual State is : <span id="LEDState1">NA</span><br>
    pump manual State is : <span id="LEDState2">NA</span>
</div>

<script>
function sendData1(led1) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("LEDState1").innerHTML = this.responseText;
      
    }
  };
  xhttp.open("GET", "setLED1?LEDstate1="+led1, true);
  
  xhttp.send();
}

function sendData2(led2) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      
      document.getElementById("LEDState2").innerHTML = this.responseText;
    }
  };
  
  xhttp.open("GET", "setLED2?LEDstate2="+led2, true);
  xhttp.send();
}


setInterval(function() {
  // Call a function repetatively with 2 Second interval
  getData();
}, 2000); //2000mSeconds update rate

function getData() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("FlowValue").innerHTML = this.responseText;
      document.getElementById("DisValue").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "readValue", true);
  xhttp.send();
}
</script>

</body>
</html>
)=====";
