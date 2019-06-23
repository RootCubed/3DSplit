function init() {
	if (!(window.File && window.FileReader && window.FileList && window.Blob)) {
		document.querySelector("#fileInput #text").textContent = "Reading files not supported by this browser";
	} else {
		let fileDrop = document.querySelector("#fileInput");
		document.querySelector("#fileInput input").addEventListener("change", function(e){
			for (let i = 0; i < 4; i++) {
				//reset progress labels
				$("#s" + i).classList.remove("complete");
				$("#s" + i).classList.remove("abort");
			}
			//get the files
			let files = e.target.files;
			if (files.length > 0){
				//select most recent file
				let file = files[0];
				$("#fileInput #text").textContent = file.name;
				$("#s0").classList.add("complete");
				//start main function
				parseFile(file);
			}
		});
	}
}

var downloadable = undefined;
var yeet;

function parseFile(file) {
	let reader = new FileReader();
	reader.onload = function(e){
		let finalFile = [];
		$("#s1").classList.add("complete");
		// convert file
		let parsedLSS;
		let fileWithoutBOM;
		parser = new DOMParser();
		try {
			if (e.target.result.charCodeAt(0) == 0xEF) { // remove BOM
				fileWithoutBOM = e.target.result.substr(3);
			} else {
				fileWithoutBOM = e.target.result;
			}
			parsedLSS = parser.parseFromString(fileWithoutBOM, "text/xml");
			yeet = parsedLSS;
		} catch(err) {
			$("#s1").classList.add("abort");
			$("#s1").textContent = "Error in reading LiveSplit file: " + err;
			return;
		}
		$("#s2").classList.add("complete");
		// metadata
		yeet = parsedLSS;
		finalFile.push(parsedLSS.getElementsByTagName("GameName")[0].textContent);
		finalFile.push(parsedLSS.getElementsByTagName("CategoryName")[0].textContent);
		finalFile.push(parsedLSS.getElementsByTagName("AttemptCount")[0].textContent);
		$("#s3").classList.add("complete");
		// split names
		finalFile.push(parsedLSS.getElementsByTagName("Segment").length);
		let names = [];
		let pbtimes = [];
		let golds = [];
		for (let segment of parsedLSS.getElementsByTagName("Segment")) {
			names.push(segment.getElementsByTagName("Name")[0].textContent);
			let pbst = segment.getElementsByTagName("SplitTime")[0];
			if (pbst.childNodes.length == 3) {
				pbtimes.push(tsMs(pbst.childNodes[1].textContent));
			} else {
				pbtimes.push(0);
			}
			let goldst = segment.getElementsByTagName("BestSegmentTime")[0];
			if (goldst.childNodes.length == 3) {
				golds.push(tsMs(goldst.childNodes[1].textContent));
			} else {
				pbtimes.push(0);
			}
		}
		$("#download").removeAttribute("disabled")
		downloadable = finalFile.join('\n') + '\n' + names.join('\n') + '\n' + pbtimes.join('\n') + '\n' + golds.join('\n') + '\n';
	};
	reader.readAsBinaryString(file);
}

function $(selector) {
	return document.querySelector(selector);
}

function tsMs(timestamp) {
	let temp = timestamp.split(':');
	let sms = temp[temp.length - 1].split('.');
	temp[temp.length - 1] = sms[0];
	if (sms.length == 2) {
		temp.push(sms[1]);
	} else {
		temp.push("0000000");
	}
	let result = 0;
	// hours
	result += parseInt(temp[0]) * 1000 * 60 * 60;
	// mintes
	result += parseInt(temp[1]) * 1000 * 60;
	// seconds
	result += parseInt(temp[2]) * 1000;
	// ms
	result += parseInt(temp[3].substring(0, 3));
	return result;
}

function download(filename, text) {
	var element = document.createElement("a");
	element.setAttribute("href", "data:text/plain;charset=utf-8," + encodeURIComponent(text));
	element.setAttribute("download", filename);

	element.style.display = "none";
	document.body.appendChild(element);

	element.click();

	document.body.removeChild(element);
}

function downloadConverted() {
	if (downloadable != undefined) {
		download("splits.sls", downloadable);
	}
}
