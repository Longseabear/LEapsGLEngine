#pragma once

namespace LEapsGL {
	union Material {
		struct { float r, g, b, a; };
		float values[4];

	};
};
