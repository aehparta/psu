var canvas = {
	el: null,
	ctx: null,
	pixels_per_step: 100,
	x: {
		/* pixels */
		end: 0,
		/* index in steps */
		step: -1,
		/* seconds each */
		steps: [
			3600, 1800, 600, 300, 120, 60, 30, 10, 5, 2.5, 1, 0.5, 0.25, 0.1, 0.05, 0.025, 0.01
		]
	},
	y: {
		/* pixels */
		middle: 0,
		/* index in steps */
		step: -1,
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

		/* resize */
		this.resize();

		/* default x-axis end and y-axis middle coordinate */
		this.x.end = this.pixels_per_step;
		this.y.middle = this.el.height / 2;

		/* fill x selector and find one second step from all the steps and set it as default */
		for (var i = 0; i < this.x.steps.length; i++) {
			if (this.x.steps[i] == 1) {
				this.x.step = i;
			}

			var option = '<option ' + (this.x.step == i ? 'selected' : '') + ' value="' + i + '">' + util.secondsToHuman(this.x.steps[i]) + '</option>';
			document.getElementById('step-x').insertAdjacentHTML('beforeend', option);
		}

		/* fill y selector and find ten microamp step from all the steps and set it as default */
		for (var i = 0; i < this.y.steps.length; i++) {
			if (this.y.steps[i] == 0.00001) {
				this.y.step = i;
			}

			var v = util.human(this.y.steps[i], 1);
			var u = util.prefix(this.y.steps[i]);
			var option = '<option ' + (this.y.step == i ? 'selected' : '') + ' value="' + i + '">' + v + u + 'A</option>';
			document.getElementById('step-y').insertAdjacentHTML('beforeend', option);
		}

		/* event bindings */
		this.el.onmousedown = this.dragStart;
		this.el.onmousemove = this.dragMove;
		this.el.onwheel = this.zoomMouse;
		document.getElementById('step-x').onchange = this.zoomSelectX;
		document.getElementById('step-y').onchange = this.zoomSelectY;
	},

	resize: function() {
		/* calculate canvas size here dynamically */
		this.el.style.top = document.getElementById('toolbar').clientHeight + 'px';
		this.el.width = window.innerWidth;
		this.el.height = window.innerHeight - document.getElementById('toolbar').clientHeight;
	},

	autoAdjust: function() {
		if (data.length < 2) {
			return;
		}

		var Imin = 0;
		var Imax = 0;
		for (var v in data) {
			Imax = Imax < v.y ? v.y : Imax;
			Imin = Imin > v.y ? v.y : Imin;
		}

		/* select boxes */
		document.getElementById('step-x').value = canvas.x.step;
		document.getElementById('step-y').value = canvas.y.step;
	},

	update: function(data) {
		/* somewhere under the data */
		if (data === undefined) {
			data = this.data;
		} else {
			this.data = data;
		}

		/* do resizing every refresh, should not do anything heavy if window has not been resized */
		this.resize();

		/* clear draw area */
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
		this.ctx.font = '24px monospace';
		this.ctx.strokeStyle = '#505050';
		this.ctx.fillStyle = '#707070';
		this.ctx.lineWidth = 0.5;
		for (var i = this.y.middle + this.pixels_per_step, v = this.y.steps[this.y.step]; i < this.el.height; i += this.pixels_per_step, v += this.y.steps[this.y.step]) {
			this.ctx.moveTo(0, i);
			this.ctx.lineTo(this.el.width, i);
			this.ctx.stroke();

			this.ctx.fillText('-' + util.human(v, 3) + util.prefix(v) + 'A', 10, i - 10);
		}
		for (var i = this.y.middle - this.pixels_per_step, v = this.y.steps[this.y.step]; i >= 0; i -= this.pixels_per_step, v += this.y.steps[this.y.step]) {
			this.ctx.moveTo(0, i);
			this.ctx.lineTo(this.el.width, i);
			this.ctx.stroke();

			this.ctx.fillText('+' + util.human(v, 3) + util.prefix(v) + 'A', 10, i + 30);
		}

		/* x grid */
		this.ctx.beginPath();
		this.ctx.font = '18px monospace';
		this.ctx.strokeStyle = '#505050';
		this.ctx.fillStyle = '#707070';
		this.ctx.lineWidth = 0.5;
		for (var i = this.el.width - this.x.end, v = 0; i >= 0; i -= this.pixels_per_step, v += this.x.steps[this.x.step]) {
			this.ctx.moveTo(i, 0);
			this.ctx.lineTo(i, this.el.height);
			this.ctx.stroke();

			this.ctx.fillText(util.secondsToHuman(v), i + 10, 30);
		}

		/* following are only shown if there is enough data */
		if (data.length >= 2) {
			/* latest timestamp needed later */
			var t_end = data[data.length - 1].x;

			/* calculate values from data */
			var Iavg = data[data.length - 1].y;
			var Iavgall = Iavg;
			var Imin = Iavg;
			var Imax = Iavg;
			var Iavgcount = 1;

			/* update data to canvas */
			this.ctx.beginPath();
			this.ctx.strokeStyle = '#00ff00';
			this.ctx.lineWidth = 1;
			this.ctx.moveTo(this.el.width - this.x.end, this.y.middle - (data[data.length - 1].y * this.pixels_per_step / this.y.steps[this.y.step]));
			for (var i = data.length - 2; i >= 0; i--) {
				var x = this.el.width - this.x.end - ((t_end - data[i].x) * this.pixels_per_step / this.x.steps[this.x.step]);
				var y = this.y.middle - (data[i].y * this.pixels_per_step / this.y.steps[this.y.step]);
				this.ctx.lineTo(x, y);

				if ((t_end - data[i].x) <= 1.0) {
					Iavg += data[i].y;
					Iavgcount++;
				}
				Iavgall += data[i].y;
				Imax = data[i].y > Imax ? data[i].y : Imax;
				Imin = data[i].y < Imin ? data[i].y : Imin;
			}
			this.ctx.stroke();

			/* display multimeter style values */
			Iavg /= Iavgcount;
			Iavgall /= data.length;
			Iavg = Math.abs(Iavg) < 0.001 ? Number(Iavg * 1000000).toFixed(1) + 'μA' : Number(Iavg * 1000).toFixed(3) + 'mA';
			Iavgall = Math.abs(Iavgall) < 0.001 ? Number(Iavgall * 1000000).toFixed(1) + 'μA' : Number(Iavgall * 1000).toFixed(3) + 'mA';
			Imax = Math.abs(Imax) < 0.001 ? Number(Imax * 1000000).toFixed(1) + 'μA' : Number(Imax * 1000).toFixed(3) + 'mA';
			Imin = Math.abs(Imin) < 0.001 ? Number(Imin * 1000000).toFixed(1) + 'μA' : Number(Imin * 1000).toFixed(3) + 'mA';
			this.ctx.save();
			this.ctx.globalAlpha = 0.7;
			this.ctx.textAlign = 'end';
			this.ctx.fillStyle = '#a0a0ff';
			this.ctx.shadowColor = '#000000';
			this.ctx.shadowBlur = 7;
			this.ctx.font = '128px monospace';
			this.ctx.fillText(Iavg, this.el.width - 40, this.el.height - 40);
			this.ctx.font = '48px monospace';
			this.ctx.fillText(Iavgall + ' Ø', this.el.width - 40, this.el.height - 160);
			this.ctx.fillText(Imin + ' ↓', this.el.width - 40, this.el.height - 220);
			this.ctx.fillText(Imax + ' ↑', this.el.width - 40, this.el.height - 280);
			this.ctx.restore();
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

		canvas.x.end = canvas.drag.x_end - (e.offsetX - canvas.drag.x);
		canvas.y.middle = canvas.drag.y_middle + (e.offsetY - canvas.drag.y);
		canvas.update();
	},

	zoom: function(axis, dir) {
		if (axis == 'y') {
			if (dir == 'out') {
				canvas.y.step = canvas.y.step > 0 ? canvas.y.step - 1 : canvas.y.step;
			} else if (dir == 'in') {
				canvas.y.step = (canvas.y.step + 1) < canvas.y.steps.length ? canvas.y.step + 1 : canvas.y.step;
			}
		} else if (axis == 'x') {
			if (dir == 'out') {
				canvas.x.step = canvas.x.step > 0 ? canvas.x.step - 1 : canvas.x.step;
			} else if (dir == 'in') {
				canvas.x.step = (canvas.x.step + 1) < canvas.x.steps.length ? canvas.x.step + 1 : canvas.x.step;
			}
		}
	},

	zoomMouse: function(e) {
		e.preventDefault();

		var offset = 0;

		if (e.ctrlKey) {
			offset = canvas.y.middle - e.offsetY;
			offset *= canvas.y.steps[canvas.y.step];
		} else {
			/* save original x offset in seconds where mouse was */
			offset = canvas.el.width - canvas.x.end - e.offsetX;
			offset = offset < 0 ? 0 : offset;
			offset *= canvas.x.steps[canvas.x.step];
		}

		/* zoom */
		canvas.zoom(e.ctrlKey ? 'y' : 'x', e.deltaY > 0 ? 'out' : 'in');
		document.getElementById('step-x').value = canvas.x.step;
		document.getElementById('step-y').value = canvas.y.step;

		if (e.ctrlKey) {
			offset /= canvas.y.steps[canvas.y.step];
			canvas.y.middle = e.offsetY + offset;
		} else if (offset > 0) {
			/* set view x position to the same second marker where mouse was before zoom */
			offset /= canvas.x.steps[canvas.x.step];
			canvas.x.end = canvas.el.width - e.offsetX - offset;
		}

		canvas.update();
	},

	zoomSelectX: function(e) {
		canvas.x.step = this.value;
		canvas.update();
	},

	zoomSelectY: function(e) {
		canvas.y.step = this.value;
		canvas.update();
	}

};