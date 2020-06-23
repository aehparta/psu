var timestamp = Date.now() - 3600000;
var chart;
var dps = [];
var x = 0;
var x_plus = 0;


function update(data) {
	for (var i = 0; i < data.length; i++) {
		dps.push({
			x: x,
			y: data[i]
		});
		x += x_plus;
	}
	chart.render();
}

function fetch() {
	$.getJSON('api/data?t=' + timestamp, function(data) {
		if (data.t === undefined || data.t <= timestamp) {
			return;
		}
		if (x_plus == 0) {
			x_plus = 1 / data.data.length;
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
			includeZero: false
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