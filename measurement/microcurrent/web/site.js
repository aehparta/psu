var influxdb_query_uri = 'http://' + location.hostname + ':8086/query?db=microcurrent&q=';
var dps = [];
var t_div = 0;
var t_max = (60);
var timestamp = Date.now() - (t_max * 1000);
var min = -0.000007,
	max = -0.000004;
var paused = false;

/* update data and refresh screen after */
function update(data) {
	/* add new data after old */
	for (var i = 0; i < data.length; i++) {
		dps.push({
			x: Date.parse(data[i][0]) / 1000,
			y: data[i][1]
		});
	}
	/* update timestamp for next fetch */
	timestamp = Date.parse(data[data.length - 1][0]);

	/* remove data that is too old */
	while ((dps[dps.length - 1].x - dps[0].x) > t_max) {
		dps.shift();
	}

	/* update graphics */
	canvas.update(dps);
}

/* fetch new data */
function fetch() {
	/* dont fetch data if paused */
	if (paused) {
		/* reload after timeout */
		setTimeout(fetch, 100);
		return;
	}
	/* fetch new data */
	$.getJSON(influxdb_query_uri + 'SELECT time,I FROM "measurements" WHERE time > ' + (timestamp + '000000')).done(function(data) {
		/* if no data received */
		if (data.results === undefined || data.results.length < 1 || data.results[0].error !== undefined) {
			/* reload after timeout */
			setTimeout(fetch, 300);
			return;
		}
		/* update data */
		update(data.results[0].series[0].values);
		/* hide loader */
		document.getElementById('loading').style.display = 'none';
		/* reload after timeout */
		setTimeout(fetch, 300);
	});
	/* update voltage */
	$.get('/voltage').done(function(data) {
		if (document.activeElement === document.getElementById('voltage')) {
			/* dont update if user has focus on the voltage input */
			return;
		}
		document.getElementById('voltage').value = Number(data).toFixed(2) + 'V';
	});
}

/* set new voltage */
function updateVoltage(v) {
	if (v === undefined) {
		v = document.getElementById('voltage').value;
	}
	v = parseFloat(v);
	console.log(v);
	if (v < 0) {
		v = 0;
	} else if (v > 4.5) {
		v = 0;
	}
	document.getElementById('voltage').value = v.toFixed(2) + 'V';
	$.get('/voltage?v=' + v.toFixed(2));
}

$(document).ready(function() {
	/* initialize drawing canvas */
	canvas.init(document.getElementById('canvas'));
	fetch();

	/* voltage adjustment */
	voltages = [0.25,0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0, 2.25, 2.5, 2.75, 3.0, 3.33];
	for (i in voltages) {
		var option = '<option value="' + voltages[i] + '">' + voltages[i].toFixed(2) + 'V</option>';
		document.getElementById('voltages').insertAdjacentHTML('beforeend', option);
	}
	document.getElementById('voltages').onchange = function() {
		updateVoltage(this.value);
	}
	document.getElementById('voltage-up').onclick = function() {
		var v = parseFloat(document.getElementById('voltage').value);
		v += 0.05;
		if (v > 4.5) {
			v = 4.5;
		}
		updateVoltage(v);
	}
	document.getElementById('voltage-down').onclick = function() {
		var v = parseFloat(document.getElementById('voltage').value);
		v -= 0.05;
		if (v < 0) {
			v = 0;
		}
		updateVoltage(v);
	}
	document.getElementById('voltage').onchange = function() {
		updateVoltage();
	}

	/* pause button */
	document.getElementById('pause').onclick = function() {
		paused = !paused;
		if (paused) {
			this.innerHTML = '<i class="fas fa-play"></i>';
		} else {
			this.innerHTML = '<i class="fas fa-pause"></i>';
			document.getElementById('loading').style.display = 'block';
		}
	};
});