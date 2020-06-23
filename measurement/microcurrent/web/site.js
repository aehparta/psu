var timestamp = Date.now();
var chart;
var dps = [];
var t_div = 0;
var t_max = 15.0;
var min = -0.000001, max = 0.00001;


function update(data) {
	for (var i = 0; i < data.length; i++) {
		dps.push({
			x: 0,
			y: data[i]
		});
	}
	while ((dps.length * t_div) > t_max) {
		dps.shift();
	}
	for (var i = 0; i < dps.length; i++) {
		dps[i].x = i * t_div;
	}
	chart.options.axisY.maximum = max;
	chart.options.axisY.minimum = min;
	chart.render();
}

function fetch() {
	$.getJSON('api/data?t=' + timestamp, function(data) {
		if (data.t === undefined || data.t <= timestamp) {
			setTimeout(fetch, 500);
			return;
		}
		if (t_div == 0) {
			t_div = 0.5 / data.data.length;
		}
		if (data.data.length > 0) {
			update(data.data);
		}
		timestamp = data.t;
		fetch();
	});
}

$(document).ready(function() {
	fetch();

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

	// var xVal = 0;
	// var yVal = 100;
	// var updateInterval = 1000;
	// var dataLength = 50; // number of dataPoints visible at any point

	// var updateChart = function(count) {
	// 	count = count || 1;
	// 	// count is number of times loop runs to generate random dataPoints.
	// 	for (var j = 0; j < count; j++) {
	// 		yVal = yVal + Math.round(5 + Math.random() * (-5 - 5));
	// 		dps.push({
	// 			x: xVal,
	// 			y: yVal
	// 		});
	// 		xVal++;
	// 	}
	// 	if (dps.length > dataLength) {
	// 		dps.shift();
	// 	}
	// 	chart.render();
	// };

	// updateChart(dataLength);
	// setInterval(function() {
	// 	updateChart()
	// }, updateInterval);

});