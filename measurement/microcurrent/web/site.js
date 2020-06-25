var influxdb_query_uri = 'http://' + location.hostname + ':8086/query?db=microcurrent&q=';
var dps = [];
var t_div = 0;
var t_max = 600;
var timestamp = Date.now() - (t_max * 1000);
var min = -0.000007,
	max = -0.000004;

function update(data) {
	for (var i = 0; i < data.length; i++) {
		dps.push({
			x: Date.parse(data[i][0]) / 1000,
			y: data[i][1]
		});
	}
	timestamp = Date.parse(data[data.length - 1][0]);

	while ((dps[dps.length - 1].x - dps[0].x) > t_max) {
		dps.shift();
	}
	canvas.update(dps);
}

function fetch() {
	$.getJSON(influxdb_query_uri + 'SELECT time,I FROM "measurements" WHERE time > ' + (timestamp + '000000')).done(function(data) {
		if (data.results === undefined || data.results.length < 1 || data.results[0].error !== undefined) {
			setTimeout(fetch, 300);
			return;
		}
		update(data.results[0].series[0].values);
		setTimeout(fetch, 300);
	});
}

$(document).ready(function() {
	canvas.init(document.getElementById('canvas'));
	// canvas.test();
	fetch();
});