#include "raytracer.h"
#include <string>


int main(int argc, char** argv) {
	std::string scene = "scene.yaml";
	std::string out = "res.bmp";
	std::string norm = "norm.bmp";
	unt x_res = 400;
	unt y_res = 300;
	unt trace_depth = 300;
	toneType tone = X_IS_LOG_255_B;
	for (int i = 1; i < argc; i++) {
		std::string str(argv[i]);
		if (str.substr(0, 8) == "--scene=") {
			scene = str.substr(8, str.size() - 8);
		}
		else if (str.substr(0, 15) == "--resolution_x=") {
			x_res = std::stoi(str.substr(15, str.size() - 15));
		}
		else if (str.substr(0, 15) == "--resolution_y=") {
			y_res = std::stoi(str.substr(15, str.size() - 15));
		}
		else if (str.substr(0, 9) == "--output=") {
			out = str.substr(9, str.size() - 9);
		}
		else if (str.substr(0, 17) == "--output_normals=") {
			norm = str.substr(17, str.size() - 17);
		}
		else if (str.substr(0, 14) == "--trace_depth=") {
			trace_depth = std::stoi(str.substr(14, str.size() - 14));
		}
		else if (str.substr(0, 14) == "--tone_mapping=") {
			tone = (toneType)std::stoi(str.substr(14, str.size() - 15));
		}
	}

	RayTracer rt(scene, out, norm, x_res, y_res, trace_depth, tone);
	rt.traceRays();
	return 0;
}