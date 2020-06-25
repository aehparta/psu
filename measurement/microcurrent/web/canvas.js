var canvas = {
	el: null,
	ctx: null,
	div: 100,
	x: {
		end: 0,
		div: 1
	},
	y: {
		middle: 0,
		div: 0.000001
	},
	drag: {
		x: 0,
		y: 0,
		y_middle: 0,
	},
	data: [],

	init: function(element) {
		this.el = element;
		this.ctx = this.el.getContext('2d');

		var w = this.el.width;
		var h = this.el.height;
		this.el.width = w * 10;
		this.el.height = h * 10;
		this.el.style.width = w;
		this.el.style.height = h;

		this.y.middle = this.el.height / 2;

		$(this.el).on('mousedown', canvas_dragStart);
		$(this.el).on('mousemove', canvas_dragMove);
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
		this.ctx.lineWidth = 6;
		this.ctx.moveTo(0, this.y.middle);
		this.ctx.lineTo(this.el.width, this.y.middle);
		this.ctx.stroke();
		/* y grid down and then up */
		this.ctx.beginPath();
		this.ctx.font = "24px monospace";
		this.ctx.strokeStyle = '#505050';
		this.ctx.fillStyle = '#707070';
		this.ctx.lineWidth = 0.5;
		for (var i = this.y.middle + this.div, v = this.y.div; i < this.el.height; i += this.div, v += this.y.div) {
			this.ctx.moveTo(0, i);
			this.ctx.lineTo(this.el.width, i);
			this.ctx.stroke();

			var vs = this.y.div < 0.001 ? '-' + Number(v * 1000000).toFixed(0) + ' µA' : '-' + Number(v * 1000).toFixed(0) + ' mA';
			this.ctx.fillText(vs, 10, i - 10);
		}
		for (var i = this.y.middle - this.div, v = this.y.div; i >= 0; i -= this.div, v += this.y.div) {
			this.ctx.moveTo(0, i);
			this.ctx.lineTo(this.el.width, i);
			this.ctx.stroke();

			var vs = this.y.div < 0.001 ? '+' + Number(v * 1000000).toFixed(0) + ' µA' : '+' + Number(v * 1000).toFixed(0) + ' mA';
			this.ctx.fillText(vs, 10, i + 30);
		}

		/* y grid */

		/* data */
		this.ctx.beginPath();
		this.ctx.strokeStyle = '#00ff00';
		this.ctx.lineWidth = 1;
		this.ctx.moveTo(this.el.width, this.el.height / 2);
		var right = data[data.length - 1].x;
		for (var i = data.length - 1; i >= 0; i--) {
			var x = (right - data[i].x) / this.x.div;
			var y = this.y.middle - (data[i].y / this.y.div);
			this.ctx.lineTo(x, y);
		}
		this.ctx.stroke();

		this.ctx.restore();
	},

};

function canvas_dragStart(e) {
	if (e.buttons == 1) {
		canvas.drag.x = e.offsetX;
		canvas.drag.y = e.offsetY;
		canvas.drag.y_middle = canvas.y.middle;
	}
};

function canvas_dragMove(e) {
	if (e.buttons != 1) {
		return;
	}

	canvas.y.middle = canvas.drag.y_middle + (e.offsetY - canvas.drag.y);
	canvas.update();
};