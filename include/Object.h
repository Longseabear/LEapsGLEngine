#pragma once

#include <glad/glad.h>
#include <string>

using namespace std;

namespace LEapsGL {
	class Object {
	public:
		string ToString();
	private:
		string name;
	};
};

