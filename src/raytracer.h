#ifndef RAYTRACER_H_INCLUDED
#define RAYTRACER_H_INCLUDED
#include "image.h"
#include "camera.h"
#include "geometry.h"

class RayTracer {
public:
	RayTracer(	std::string		scene = "scene.yaml",
				std::string		out = "res.bmp",
				std::string		norm = "norm.bmp",
				unt				x_res = 400,
				unt				y_res = 300,
				unt				trace_depth = 300,
				toneType        tp = X_IS_LOG_255_B);
  void traceRays();
private:
	PointColor traceRay(Ray ray);

  void ScanCamera(FILE *f);
  void ScanObject(FILE *f);
  void ScanLight(FILE *f);
  std::shared_ptr<Object> ParseNode(FILE *f, unt indent, csg_type tp = CSG_UNION);

  Camera camera;
  Image image;
  Image image_normal;
  int width, height, width_shift, height_shift;
  float dist;
  std::string out_name;
  std::string norm_name;

  glm::mat4x4 m_cam_mat, m_cam_inv;

  GroupObject m_scene;
};

#endif