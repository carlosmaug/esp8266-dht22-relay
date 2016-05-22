var xhr = new XMLHttpRequest();

function createInputForAp(ap, selectedAp) {
	if (ap.essid == "" && ap.rssi == 0) return;

	var div = document.createElement("div");
	div.id = "apdiv";

	var input = document.createElement("input");
	input.type = "radio";
	input.name = "essid";
	input.value = ap.essid;
	if (selectedAp == ap.essid) input.checked = "1";
	input.id = "opt-"+ap.essid;

	var label = document.createElement("label");
	label.htmlFor = "opt-"+ap.essid;
	label.textContent = ap.essid+" (rssi "+ap.rssi+")";
	div.appendChild(input);
	div.appendChild(label);

	return div;
}
			
function getSelectedEssid(apName) {
	var e = document.forms.wifiform.elements;
	for (var i = 0; i < e.length; i++) {
		if (e[i].type == "radio" && e[i].checked) return e[i].value;
	}
	return apName;
}

function scanAPs(currAp) {
	xhr.open("GET", "wifiscan.cgi");
	xhr.onreadystatechange = function() {
		if (xhr.readyState == 4 && xhr.status >= 200 && xhr.status < 300) {
			var data = JSON.parse(xhr.responseText);
			selectedAp = getSelectedEssid(currAp);
			if (data.result.inProgress == "0" && data.result.APs.length > 1) {
				document.getElementById("aps").innerHTML = "";
				for (var i = 0; i < data.result.APs.length; i++) {
					if (data.result.APs[i].essid == "" && data.result.APs[i].rssi == 0) continue;

					document.getElementById("aps").appendChild(createInputForAp(data.result.APs[i], selectedAp));
				}
				window.setTimeout(scanAPs, 20000);
			} else {
				window.setTimeout(scanAPs, 1000);
			}
		}
	}
	xhr.send();
}

