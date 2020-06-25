var util = {
	unit_prefixes: [{
		'd': 1e18,
		'u': 'E'
	}, {
		'd': 1e15,
		'u': 'P'
	}, {
		'd': 1e12,
		'u': 'T'
	}, {
		'd': 1e9,
		'u': 'G'
	}, {
		'd': 1e6,
		'u': 'M'
	}, {
		'd': 1e3,
		'u': 'k'
	}, {
		'd': 1,
		'u': ''
	}, {
		'd': 1e-3,
		'u': 'm'
	}, {
		'd': 1e-6,
		'u': 'μ'
	}, {
		'd': 1e-9,
		'u': 'p'
	}, {
		'd': 1e-12,
		'u': 'n'
	}],

	divider: function(value, resolution) {
		if (resolution === undefined) {
			resolution = value;
		}

		/* calculate divider */
		var v = Math.abs(value);
		for (var i in this.unit_prefixes) {
			var p = this.unit_prefixes[i];
			/* check that it is not smaller than resolution */
			if (resolution >= p.d) {
				return p;
			}
			/* check if this is correct divider */
			if (p.d <= v && (v < (p.d * 1000) || p.d >= 1e18)) {
				return p;
			}
		};
		return {
			'd': 1,
			'u': ''
		};
	},

	human: function(value, resolution) {
		if (resolution === undefined) {
			resolution = value;
		}

		/* get unit prefix divider */
		var p = this.divider(value, resolution);
		/* calculate value using unit prefix divider */
		var value = value / p.d;
		/* if resolution is not set, convert to max of 6 characters */
		if (resolution == 0) {
			/* 4 decimals max with '0.' in front */
			var i = 4;
			do {
				try {
					value = Number(value).toFixed(i);
				} catch (e) {
					/* large numbers throw a RangeError exception with toFixed */
				}
				i--;
			} while (value.length > 6 && i >= 0);
			/* trim trailing zeroes off if value has decimals */
			if (value.includes('.')) {
				value = value.replace(/\.[0]+$/gm, '');
				value = '0' <= value[0] && value[0] <= '9' ? value : '0' + value;
			}
		} else {
			/* resolution is set, render by resolution
			 * first find decimals in resolution combined with divider
			 */
			var r = Number(resolution) / p.d;
			var decimals = Math.floor(r) === r ? 0 : (r.toString().split('.')[1].length || 0);
			/* round up to resolution */
			value = parseInt(value / r) * r;
			/* cut decimals just to be sure since float value math operations sometimes are not as precise as we want */
			value = value.toFixed(decimals);
		}

		return value;
	},

	secondsToHuman: function(value, resolution) {
		if (resolution === undefined) {
			resolution = value;
		}
		if (value >= (24 * 3600)) {
			return Number(value / 24 / 3600).toFixed(0) + 'd';
		} else if (value >= 3600) {
			return Number(value / 3600).toFixed(0) + 'h';
		} else if (value >= 60) {
			return Number(value / 60).toFixed(0) + 'm';
		}
		var s = util.human(value, resolution);
		var u = util.divider(value, resolution).u;
		return s + u + 's';
	}
}