var canvas = {
	el: null,
	ctx: null,
	pixels_per_step: 100,
	x: {
		/* pixels */
		end: 0,
		/* index in steps */
		step: 0,
		/* seconds each */
		steps: [
			3600, 1800, 600, 300, 120, 60, 30, 10, 5, 2.5, 1, 0.5, 0.25, 0.1, 0.05, 0.025, 0.01
		]
	},
	y: {
		/* pixels */
		middle: 0,
		/* index in steps */
		step: 0,
		/* amps each */
		steps: [
			1, 0.5, 0.25,
			0.1, 0.05, 0.025,
			0.01, 0.005, 0.0025,
			0.001, 0.0005, 0.00025,
			0.0001, 0.00005, 0.000025,
			0.00001, 0.000005, 0.0000025,
			0.000001, 0.0000005, 0.00000025,
		]
	},
	drag: {
		x: 0,
		y: 0,
		x_end: 0,
		y_middle: 0,
	},
	data: [],

	init: function(element) {
		this.el = element;
		this.ctx = this.el.getContext('2d');

		/* multiply canvas size by ten to achieve prettier lines
		 * javascript canvas thing, dunno more about this one
		 * tl;dr: without this everything is very fuzzy because of antialiasing etc
		 */
		var w = this.el.width;
		var h = this.el.height;
		this.el.width = w * 10;
		this.el.height = h * 10;
		this.el.style.width = w;
		this.el.style.height = h;

		/* default x-axis end and y-axis middle coordinate */
		this.x.end = this.pixels_per_step;
		this.y.middle = this.el.height / 2;

		/* find one second step from all the steps and set it as default */
		for (var i = 0; i < this.x.steps.length; i++) {
			if (this.x.steps[i] == 1) {
				this.x.step = i;
				break;
			}
		}

		/* find ten microamp step from all the steps and set it as default */
		for (var i = 0; i < this.y.steps.length; i++) {
			if (this.y.steps[i] == 0.00001) {
				this.y.step = i;
				break;
			}
		}

		/* event bindings */
		this.el.onmousedown = this.dragStart;
		this.el.onmousemove = this.dragMove;
		this.el.onwheel = this.zoom;
	},

	update: function(data) {
		if (data === undefined) {
			data = this.data;
		} else {
			this.data = data;
		}

		this.ctx.clearRect(0, 0, this.el.width, this.el.height);
		this.ctx.save();

		/* y grid */
		this.ctx.beginPath();
		this.ctx.strokeStyle = '#507050';
		this.ctx.lineWidth = 2;
		this.ctx.moveTo(0, this.y.middle);
		this.ctx.lineTo(this.el.width, this.y.middle);
		this.ctx.stroke();
		/* y grid down and then up */
		this.ctx.beginPath();
		this.ctx.font = "24px monospace";
		this.ctx.strokeStyle = '#505050';
		this.ctx.fillStyle = '#707070';
		this.ctx.lineWidth = 0.5;
		for (var i = this.y.middle + this.pixels_per_step, v = this.y.steps[this.y.step]; i < this.el.height; i += this.pixels_per_step, v += this.y.steps[this.y.step]) {
			this.ctx.moveTo(0, i);
			this.ctx.lineTo(this.el.width, i);
			this.ctx.stroke();

			var vs = this.y.steps[this.y.step] < 0.001 ? '-' + Number(v * 1000000).toFixed(0) + ' µA' : '-' + Number(v * 1000).toFixed(0) + ' mA';
			this.ctx.fillText(vs, 10, i - 10);
		}
		for (var i = this.y.middle - this.pixels_per_step, v = this.y.steps[this.y.step]; i >= 0; i -= this.pixels_per_step, v += this.y.steps[this.y.step]) {
			this.ctx.moveTo(0, i);
			this.ctx.lineTo(this.el.width, i);
			this.ctx.stroke();

			var vs = this.y.steps[this.y.step] < 0.001 ? '+' + Number(v * 1000000).toFixed(0) + ' µA' : '+' + Number(v * 1000).toFixed(0) + ' mA';
			this.ctx.fillText(vs, 10, i + 30);
		}

		/* x grid */
		this.ctx.beginPath();
		this.ctx.font = "18px monospace";
		this.ctx.strokeStyle = '#505050';
		this.ctx.fillStyle = '#707070';
		this.ctx.lineWidth = 0.5;
		for (var i = this.el.width - this.x.end, v = 0; i >= 0; i -= this.pixels_per_step, v += this.x.steps[this.x.step]) {
			this.ctx.moveTo(i, 0);
			this.ctx.lineTo(i, this.el.height);
			this.ctx.stroke();

			var vs = Number(Number(v).toFixed(3)) + 's';
			if (this.x.steps[this.x.step] >= 3600) {
				vs = Number(v / 3600).toFixed(0) + 'h';
			} else if (this.x.steps[this.x.step] >= 60) {
				vs = Number(v / 60).toFixed(0) + 'm';
			}
			this.ctx.fillText(vs, i + 10, 30);
		}

		/* data */
		if (data.length >= 2) {
			this.ctx.beginPath();
			this.ctx.strokeStyle = '#00ff00';
			this.ctx.lineWidth = 1;
			this.ctx.moveTo(this.el.width - this.x.end, this.y.middle - (data[data.length - 1].y * this.pixels_per_step / this.y.steps[this.y.step]));
			var t_end = data[data.length - 1].x;
			for (var i = data.length - 2; i >= 0; i--) {
				var x = this.el.width - this.x.end - ((t_end - data[i].x) * this.pixels_per_step / this.x.steps[this.x.step]);
				var y = this.y.middle - (data[i].y * this.pixels_per_step / this.y.steps[this.y.step]);
				this.ctx.lineTo(x, y);
			}
			this.ctx.stroke();
		}

		this.ctx.restore();
	},

	dragStart: function(e) {
		if (e.buttons == 1) {
			canvas.drag.x = e.offsetX;
			canvas.drag.y = e.offsetY;
			canvas.drag.x_end = canvas.x.end;
			canvas.drag.y_middle = canvas.y.middle;
		}
	},

	dragMove: function(e) {
		if (e.buttons != 1) {
			return;
		}

		var end = canvas.drag.x_end - (e.offsetX - canvas.drag.x);
		if (end < canvas.pixels_per_step) {
			canvas.x.end = end;
		}
		canvas.y.middle = canvas.drag.y_middle + (e.offsetY - canvas.drag.y);
		canvas.update();
	},

	zoom: function(e) {
		e.preventDefault();
		if (e.ctrlKey) {
			if (e.deltaY > 0) {
				canvas.y.step = canvas.y.step > 0 ? canvas.y.step - 1 : canvas.y.step;
			} else {
				canvas.y.step = (canvas.y.step + 1) < canvas.y.steps.length ? canvas.y.step + 1 : canvas.y.step;
			}
		} else {
			if (e.deltaY > 0) {
				canvas.x.step = canvas.x.step > 0 ? canvas.x.step - 1 : canvas.x.step;
			} else {
				canvas.x.step = (canvas.x.step + 1) < canvas.x.steps.length ? canvas.x.step + 1 : canvas.x.step;
			}
		}
		canvas.update();
	},
};