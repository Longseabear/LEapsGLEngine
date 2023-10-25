#pragma once

#define MAKE_COLOR_STATIC_VALUE(color, r,g,b,a) \
    constexpr LEapsGL::Color LEapsGL::Color::color = { r, g, b, a };

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

inline MAKE_COLOR_STATIC_VALUE(black, 0.0f, 0.0f, 0.0f, 1.0f);
inline MAKE_COLOR_STATIC_VALUE(blue, 0.0f, 0.0f, 1.0f, 1.0f);
inline MAKE_COLOR_STATIC_VALUE(clear, 0.0f, 0.0f, 0.0f, 0.0f);
inline MAKE_COLOR_STATIC_VALUE(cyan, 0.0f, 1.0f, 1.0f, 0.0f);
inline MAKE_COLOR_STATIC_VALUE(gray, 0.5f, 0.5f, 0.5f, 1.0f);
inline MAKE_COLOR_STATIC_VALUE(green, 0.0f, 0.1f, 0.0f, 1.0f);
inline MAKE_COLOR_STATIC_VALUE(grey, 0.5f, 0.5f, 0.5f, 1.0f);
inline MAKE_COLOR_STATIC_VALUE(magenta, 1.0f, 0.0f, 1.0f, 1.0f);
inline MAKE_COLOR_STATIC_VALUE(red, 1.0f, 0.0f, 0.0f, 1.0f);
inline MAKE_COLOR_STATIC_VALUE(white, 1.0f, 1.0f, 1.0f, 1.0f);
inline MAKE_COLOR_STATIC_VALUE(yellow, 1.0f, 0.92f, 0.016f, 1.0f);
