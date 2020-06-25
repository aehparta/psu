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

	divider: function(value) {
		/* calculate divider */
		var v = Math.abs(value);
		for (var i in this.unit_prefixes) {
			var p = this.unit_prefixes[i];
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

	prefix: function(value) {
		return this.divider(value).u;
	},

	human: function(value, decimals) {
		if (decimals === undefined) {
			decimals = 0;
		}

		/* get unit prefix divider */
		var p = this.divider(value);
		/* calculate value using unit prefix divider */
		var value = value / p.d;
		/* cut to max decimals and remove zeros */
		value = value.toFixed(decimals);
		if (value.includes('.')) {
			value = this.rtrim(value, '0');
			value = this.rtrim(value, '.');
		}

		return value;
	},

	secondsToHuman: function(value) {
		if (value >= (24 * 3600)) {
			return Number(value / 24 / 3600).toFixed(0) + 'd';
		} else if (value >= 3600) {
			return Number(value / 3600).toFixed(0) + 'h' + ((value % 3600) > 60 ? (value / 60) % 60 + 'm' : '');
		} else if (value >= 60) {
			return Number(value / 60).toFixed(0) + 'm';
		}
		var s = this.human(value, 3);
		var u = this.prefix(value);
		return s + u + 's';
	},

	rtrim: function(v, c) {
		for (var i = v.length - 1; i >= 0; i--) {
			if (v.charAt(i) == c) {
				v = v.substring(0, i);
			} else {
				break;
			}
		}
		return v;
	}
}