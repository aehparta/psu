var influxdb_query_uri = 'http://' + location.hostname + ':8086/query?db=microcurrent&q=';
var dps = [];
var t_div = 0;
var t_max = 60;
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
}

$(document).ready(function() {
	/* initialize drawing canvas */
	canvas.init(document.getElementById('canvas'));
	fetch();

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