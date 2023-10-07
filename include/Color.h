#pragma once

#define MAKE_COLOR_STATIC_VALUE(color, r,g,b,a) const Color Color::color = {r,g,b,a};
namespace LEapsGL {
	union Color {
		struct { float r, g, b, a; };
		float values[4];

		static const Color black;
		static const Color blue;
		static const Color clear;
		static const Color cyan;
		static const Color gray;
		static const Color green;
		static const Color grey;
		static const Color magenta;
		static const Color red;
		static const Color white;
		static const Color yellow;
	};
};
