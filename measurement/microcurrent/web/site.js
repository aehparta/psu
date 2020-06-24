var influxdb_query_uri = 'http://' + location.hostname + ':8086/query?db=microcurrent&q=';
var chart;
var dps = [];
var t_div = 0;
var t_max = 15000;
var timestamp = Date.now() - t_max;
var min = -0.000007,
	max = -0.000004;

function update(data) {
	for (var i = 0; i < data.length; i++) {
		dps.push({
			x: Date.parse(data[i][0]),
			y: data[i][1] * 1000000
		});
	}
	timestamp = Date.parse(data[data.length - 1][0]);

	while ((dps[dps.length - 1].x - dps[0].x) > t_max) {
		dps.shift();
	}
	// for (var i = 0; i < dps.length; i++) {
	// 	dps[i].x = i * t_div;
	// }
	// chart.options.axisY.maximum = max;
	// chart.options.axisY.minimum = min;
	chart.render();
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
	chart = new CanvasJS.Chart("graph", {
		zoomEnabled: true,
		exportEnabled: true,
		title: {
			text: "Current"
		},
		axisY: {
			includeZero: false,
			// maximum: 0.00001,
			// minimum: -0.00001,
		},
		data: [{
			type: "spline",
			markerSize: 0,
			dataPoints: dps
		}]
	});

	fetch();
});