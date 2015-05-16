#include "raytracer.h"
#include <glm/gtc/matrix_transform.hpp>

static const float pi_div_180 = 3.1415926535 / 180.f;

void RayTracer::ScanCamera(FILE *f)
{
	char buf[1001];
	float fov_x, fov_y;
	float pos_x, pos_y, pos_z;
	float dir_x, dir_y, dir_z;

	for (unt i = 0; i < 4; i++)
	{
		if (!fgets(buf, 1000, f))
			return;
		std::string row(buf);
		unt pos = 0;
		if ((pos = row.find("fov_x:")) != std::string::npos)
		{
			pos = row.find(":") + 1;
			fov_x = std::stof(row.substr(pos, row.size() - pos)) / 2;
		}
		else if ((pos = row.find("fov_y:")) != std::string::npos)
		{
			pos = row.find(":") + 1;
			fov_y = std::stof(row.substr(pos, row.size() - pos)) / 2;
		}
		else if ((pos = row.find("position:")) != std::string::npos)
		{
			unt pos_start_x = row.find("x:") + 2, pos_end_x = row.find(",");
			unt pos_start_y = row.find("y:") + 2, pos_end_y = row.substr(pos_start_y, row.size() - pos_start_y).find(",");
			unt pos_start_z = row.find("z:") + 2, pos_end_z = row.find("}");
			pos_x = std::stof(row.substr(pos_start_x, pos_end_x));
			pos_y = std::stof(row.substr(pos_start_y, pos_end_y));
			pos_z = std::stof(row.substr(pos_start_z, pos_end_z));
		}
		else if ((pos = row.find("orientation:")) != std::string::npos)
		{
			unt pos_start_x = row.find("h:") + 2, pos_end_x = row.find(",");
			unt pos_start_y = row.find("p:") + 2, pos_end_y = row.substr(pos_start_y, row.size() - pos_start_y).find(",");
			unt pos_start_z = row.find("r:") + 2, pos_end_z = row.find("}");
			dir_x = std::stof(row.substr(pos_start_x, pos_end_x)) * pi_div_180;
			dir_y = std::stof(row.substr(pos_start_y, pos_end_y)) * pi_div_180;
			dir_z = std::stof(row.substr(pos_start_z, pos_end_z)) * pi_div_180;
		}
	}
	
	float tg_y = tan(fov_y * pi_div_180);
	float tg_x = tan(fov_x * pi_div_180);
	float dist_x = width / tg_x;
	float dist_y = height / tg_y;
	dist = (dist_x < dist_y) ? dist_x : dist_y;
	width_shift = (width - dist * tg_x) * 0.5;
	height_shift = (height - dist * tg_y) * 0.5;
	width = dist * tg_x;
	height = dist * tg_y;

	glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(-pos_x, -pos_y, -pos_z));
	view = glm::rotate(view, -dir_x, glm::vec3(1, 0, 0));
	view = glm::rotate(view, -dir_y, glm::vec3(0, 1, 0));
	view = glm::rotate(view, -dir_z, glm::vec3(0, 0, 1));
	m_cam_mat = view;
	m_cam_inv = inverse(m_cam_mat);
}

std::shared_ptr<Object> RayTracer::ParseNode(FILE *f, unt indent, csg_type tp)
{
	std::shared_ptr<GroupObject> node(new GroupObject());
	node->m_csg_type = tp;
	static bool flag = false;
	static char buf[1001];
	while (true)
	{
		if (!flag)
		{
			if (!fgets(buf, 1000, f))
				return node;
			if (buf[0] == '#')
				continue;
		}
		std::string row(buf);
		if (row.find("-") < indent)
		{
			flag = true;
			return node;
		}

		flag = false;
		if (row.find("- lcs:") != std::string::npos)
		{
			unt pos_start_x = row.find("x:") + 2, pos_end_x = row.find(",");
			unt pos_start_y = row.find("y:") + 2, pos_end_y = row.substr(pos_start_y, row.size() - pos_start_y).find(",");
			unt pos_start_z = row.find("z:") + 2, pos_end_z = row.substr(pos_start_z, row.size() - pos_start_z).find(",");
			unt pos_start_h = row.find("h:") + 2, pos_end_h = row.substr(pos_start_h, row.size() - pos_start_h).find(",");
			unt pos_start_p = row.find("p:") + 2, pos_end_p = row.substr(pos_start_p, row.size() - pos_start_p).find(",");
			unt pos_start_r = row.find("r:") + 2, pos_end_r = row.substr(pos_start_r, row.size() - pos_start_r).find(",");
			unt pos_start_sx = row.find("sx:") + 3, pos_end_sx = row.substr(pos_start_sx, row.size() - pos_start_sx).find(",");
			unt pos_start_sy = row.find("sy:") + 3, pos_end_sy = row.substr(pos_start_sy, row.size() - pos_start_sy).find(",");
			unt pos_start_sz = row.find("sz:") + 3, pos_end_sz = row.find("}");
			float x = std::stof(row.substr(pos_start_x, pos_end_x));
			float y = std::stof(row.substr(pos_start_y, pos_end_y));
			float z = std::stof(row.substr(pos_start_z, pos_end_z));
			float h = std::stof(row.substr(pos_start_h, pos_end_h)) * pi_div_180;
			float p = std::stof(row.substr(pos_start_p, pos_end_p)) * pi_div_180;
			float r = std::stof(row.substr(pos_start_r, pos_end_r)) * pi_div_180;
			float sx = 1 / std::stof(row.substr(pos_start_sx, pos_end_sx));
			float sy = 1 / std::stof(row.substr(pos_start_sy, pos_end_sy));
			float sz = 1 / std::stof(row.substr(pos_start_sz, pos_end_sz));

			glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)) * glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, sz));
			view = glm::rotate(view, h, glm::vec3(1, 0, 0));
			view = glm::rotate(view, p, glm::vec3(0, 1, 0));
			view = glm::rotate(view, r, glm::vec3(0, 0, 1));
			node->m_mat = view;
			node->m_inv = glm::inverse(node->m_mat);
		}
		if (row.find("- material:") != std::string::npos)
		{
			unt pos_start_amb_r = row.find("amb_r:") + 6, pos_end_amb_r = row.find(",");
			unt pos_start_amb_g = row.find("amb_g:") + 6, pos_end_amb_g = row.substr(pos_start_amb_g, row.size() - pos_start_amb_g).find(",");
			unt pos_start_amb_b = row.find("amb_b:") + 6, pos_end_amb_b = row.substr(pos_start_amb_b, row.size() - pos_start_amb_b).find(",");

			unt pos_start_dif_r = row.find("dif_r:") + 6, pos_end_dif_r = row.substr(pos_start_dif_r, row.size() - pos_start_dif_r).find(",");
			unt pos_start_dif_g = row.find("dif_g:") + 6, pos_end_dif_g = row.substr(pos_start_dif_g, row.size() - pos_start_dif_g).find(",");
			unt pos_start_dif_b = row.find("dif_b:") + 6, pos_end_dif_b = row.substr(pos_start_dif_b, row.size() - pos_start_dif_b).find(",");

			unt pos_start_spe_r = row.find("spe_r:") + 6, pos_end_spe_r = row.substr(pos_start_spe_r, row.size() - pos_start_spe_r).find(",");
			unt pos_start_spe_g = row.find("spe_g:") + 6, pos_end_spe_g = row.substr(pos_start_spe_g, row.size() - pos_start_spe_g).find(",");
			unt pos_start_spe_b = row.find("spe_b:") + 6, pos_end_spe_b = row.substr(pos_start_spe_b, row.size() - pos_start_spe_b).find(",");

			unt pos_start_col_r = row.find("col_r:") + 6, pos_end_col_r = row.substr(pos_start_col_r, row.size() - pos_start_col_r).find(",");
			unt pos_start_col_g = row.find("col_g:") + 6, pos_end_col_g = row.substr(pos_start_col_g, row.size() - pos_start_col_g).find(",");
			unt pos_start_col_b = row.find("col_b:") + 6, pos_end_col_b = row.substr(pos_start_col_b, row.size() - pos_start_col_b).find(",");
			unt pos_start_a = row.find("a:") + 2, pos_end_a = row.find("}");
			node->m_amb = glm::vec3(std::stof(row.substr(pos_start_amb_r, pos_end_amb_r)), 
				std::stof(row.substr(pos_start_amb_g, pos_end_amb_g)),
				std::stof(row.substr(pos_start_amb_b, pos_end_amb_b)));
			node->m_dif = glm::vec3(std::stof(row.substr(pos_start_dif_r, pos_end_dif_r)),
				std::stof(row.substr(pos_start_dif_g, pos_end_dif_g)),
				std::stof(row.substr(pos_start_dif_b, pos_end_dif_b)));
			node->m_spec = glm::vec3(std::stof(row.substr(pos_start_spe_r, pos_end_spe_r)),
				std::stof(row.substr(pos_start_spe_g, pos_end_spe_g)),
				std::stof(row.substr(pos_start_spe_b, pos_end_spe_b)));
			node->m_base = glm::vec3(std::stof(row.substr(pos_start_col_r, pos_end_col_r)),
				std::stof(row.substr(pos_start_col_g, pos_end_col_g)),
				std::stof(row.substr(pos_start_col_b, pos_end_col_b)));
			node->m_a = std::stof(row.substr(pos_start_a, pos_end_a));
		}
		else if (row.find("- triangle:") != std::string::npos)
		{
			glm::vec3 p0, p1, p2;
			for (unt i = 0; i < 3; i++)
			{
				if (!fgets(buf, 1000, f))
					return node;
				std::string row(buf);
				unt pos_start_x = row.find("x:") + 2, pos_end_x = row.find(",");
				unt pos_start_y = row.find("y:") + 2, pos_end_y = row.substr(pos_start_y, row.size() - pos_start_y).find(",");
				unt pos_start_z = row.find("z:") + 2, pos_end_z = row.find("}");
				((i == 0) ? p0 : ((i == 1) ? p1 : p2)).x = std::stof(row.substr(pos_start_x, pos_end_x));
				((i == 0) ? p0 : ((i == 1) ? p1 : p2)).y = std::stof(row.substr(pos_start_y, pos_end_y));
				((i == 0) ? p0 : ((i == 1) ? p1 : p2)).z = std::stof(row.substr(pos_start_z, pos_end_z));
			}
			node->push_back(std::shared_ptr<Triangle>(new Triangle(p0, p1, p2)));
		}
		else if (row.find("- plane:") != std::string::npos)
		{
			glm::vec3 p0, p1, p2;
			for (unt i = 0; i < 3; i++)
			{
				if (!fgets(buf, 1000, f))
					return node;
				std::string row(buf);
				unt pos_start_x = row.find("x:") + 2, pos_end_x = row.find(",");
				unt pos_start_y = row.find("y:") + 2, pos_end_y = row.substr(pos_start_y, row.size() - pos_start_y).find(",");
				unt pos_start_z = row.find("z:") + 2, pos_end_z = row.find("}");
				((i == 0) ? p0 : ((i == 1) ? p1 : p2)).x = std::stof(row.substr(pos_start_x, pos_end_x));
				((i == 0) ? p0 : ((i == 1) ? p1 : p2)).y = std::stof(row.substr(pos_start_y, pos_end_y));
				((i == 0) ? p0 : ((i == 1) ? p1 : p2)).z = std::stof(row.substr(pos_start_z, pos_end_z));
			}
			node->push_back(std::shared_ptr<Plane>(new Plane(p0, p1, p2)));
		}
		else if (row.find("- sphere:") != std::string::npos)
		{
			float r;
			if (!fgets(buf, 1000, f))
				return node;
			std::string row(buf);
			unt pos_start_r = row.find("radius:") + 7;
			r = std::stof(row.substr(pos_start_r, row.size() - pos_start_r));
			node->push_back(std::shared_ptr<Sphere>(new Sphere(r)));
		}
		else if (row.find("- cylinder:") != std::string::npos)
		{
			float r, h;
			if (!fgets(buf, 1000, f))
				return node;
			std::string row(buf);
			unt pos_start_r = row.find("radius:") + 7;
			r = std::stof(row.substr(pos_start_r, row.size() - pos_start_r));
			if (!fgets(buf, 1000, f))
				return node;
			row = buf;
			pos_start_r = row.find("height:") + 7;
			h = std::stof(row.substr(pos_start_r, row.size() - pos_start_r));
			node->push_back(std::shared_ptr<Cylinder>(new Cylinder(r, h)));
		}
		else if (row.find("- cone:") != std::string::npos)
		{
			float r, h;
			if (!fgets(buf, 1000, f))
				return node;
			std::string row(buf);
			unt pos_start_r = row.find("radius:") + 7;
			r = std::stof(row.substr(pos_start_r, row.size() - pos_start_r));
			if (!fgets(buf, 1000, f))
				return node;
			row = buf;
			pos_start_r = row.find("height:") + 7;
			h = std::stof(row.substr(pos_start_r, row.size() - pos_start_r));
			node->push_back(std::shared_ptr<Cone>(new Cone(r, h)));
		}
		else if (row.find("- torus:") != std::string::npos)
		{
			float r, h;
			if (!fgets(buf, 1000, f))
				return node;
			std::string row(buf);
			unt pos_start_r = row.find("radius:") + 7;
			r = std::stof(row.substr(pos_start_r, row.size() - pos_start_r));
			if (!fgets(buf, 1000, f))
				return node;
			row = buf;
			pos_start_r = row.find("tube_radius:") + 12;
			h = std::stof(row.substr(pos_start_r, row.size() - pos_start_r));
			node->push_back(std::shared_ptr<Torus>(new Torus(r, h)));
		}
		else if (row.find("- obj_model:") != std::string::npos)
		{
			std::string fname;
			if (!fgets(buf, 1000, f))
				return node;
			std::string row(buf);
			unt pos_start_f = row.find("\"") + 1;
			row = row.substr(pos_start_f, row.size() - pos_start_f);
			unt pos_end_f = row.find("\"");
			fname = row.substr(0, pos_end_f);
			node->push_back(std::shared_ptr<ObjModel>(new ObjModel(fname)));
		}
		else
		{
			unt pos = row.find("- node:");
			if (pos != std::string::npos)
				node->push_back(ParseNode(f, pos + 1, CSG_NONE));
			else
			{
				csg_type type = CSG_NONE;
				if (row.find("- csg_union:") != std::string::npos)
					type = CSG_UNION;
				else if (row.find("- csg_difference:") != std::string::npos)
					type = CSG_DIFFERENCE;
				else if (row.find("- csg_intersection:") != std::string::npos)
					type = CSG_INTERSECTION;

				if (type != CSG_NONE)
				{
					pos = row.find("- csg_");
					node->push_back(ParseNode(f, pos + 1, type));
				}
				else
				{
					pos = row.find("- left_node:");
					if (pos != std::string::npos)
						node->push_back(ParseNode(f, pos + 1, CSG_NONE));
					else
					{
						pos = row.find("- right_node:");
						if (pos != std::string::npos)
							node->push_back(ParseNode(f, pos + 1, CSG_NONE));
					}
				}
			}
		}
	}
	return node;
}

void RayTracer::ScanObject(FILE *f)
{
	m_scene.push_back(ParseNode(f, 1));
	m_scene.m_csg_type = CSG_UNION;
}


void RayTracer::ScanLight(FILE *f)
{
	static char buf[1001];
	int flag = 0;
	glm::vec4 pos(0,0,0,1);
	glm::vec3 amb, dif, spe;
	float r;
	while (true)
	{
		if (!fgets(buf, 1000, f))
			return;
		if (buf[0] == '#')
			continue;
		std::string row(buf);
		if (row.find("- node:") != std::string::npos)
		{
			ScanObject(f);
		}
		if (flag == 0)
		{
			if (row.find("- point:") != std::string::npos)
			{
				flag = 2;
				r = 1;
			}
			if (row.find("- sphere:") != std::string::npos)
			{
				flag = 3;
			}
		}
		else
		{
			if (row.find("- pos:") != std::string::npos)
			{
				flag--;
				unt pos_start_x = row.find("x:") + 2, pos_end_x = row.find(",");
				unt pos_start_y = row.find("y:") + 2, pos_end_y = row.substr(pos_start_y, row.size() - pos_start_y).find(",");
				unt pos_start_z = row.find("z:") + 2, pos_end_z = row.find("}");
				pos.x = std::stof(row.substr(pos_start_x, pos_end_x));
				pos.y = std::stof(row.substr(pos_start_y, pos_end_y));
				pos.z = std::stof(row.substr(pos_start_z, pos_end_z));
			}
			else if (row.find("- radius:") != std::string::npos)
			{
				flag--;
				unt pos_start_r = row.find("radius:") + 7;
				r = std::stof(row.substr(pos_start_r, row.size() - pos_start_r));
			}
			else if (row.find("- pow:") != std::string::npos)
			{
				flag--;
				unt pos_start_amb_r = row.find("amb_r:") + 6, pos_end_amb_r = row.find(",");
				unt pos_start_amb_g = row.find("amb_g:") + 6, pos_end_amb_g = row.substr(pos_start_amb_g, row.size() - pos_start_amb_g).find(",");
				unt pos_start_amb_b = row.find("amb_b:") + 6, pos_end_amb_b = row.substr(pos_start_amb_b, row.size() - pos_start_amb_b).find(",");

				unt pos_start_dif_r = row.find("dif_r:") + 6, pos_end_dif_r = row.substr(pos_start_dif_r, row.size() - pos_start_dif_r).find(",");
				unt pos_start_dif_g = row.find("dif_g:") + 6, pos_end_dif_g = row.substr(pos_start_dif_g, row.size() - pos_start_dif_g).find(",");
				unt pos_start_dif_b = row.find("dif_b:") + 6, pos_end_dif_b = row.substr(pos_start_dif_b, row.size() - pos_start_dif_b).find(",");

				unt pos_start_spe_r = row.find("spe_r:") + 6, pos_end_spe_r = row.substr(pos_start_spe_r, row.size() - pos_start_spe_r).find(",");
				unt pos_start_spe_g = row.find("spe_g:") + 6, pos_end_spe_g = row.substr(pos_start_spe_g, row.size() - pos_start_spe_g).find(",");
				unt pos_start_spe_b = row.find("spe_b:") + 6, pos_end_spe_b = row.find("}");

				amb = glm::vec3(std::stof(row.substr(pos_start_amb_r, pos_end_amb_r)),
					std::stof(row.substr(pos_start_amb_g, pos_end_amb_g)),
					std::stof(row.substr(pos_start_amb_b, pos_end_amb_b)));
				dif = glm::vec3(std::stof(row.substr(pos_start_dif_r, pos_end_dif_r)),
					std::stof(row.substr(pos_start_dif_g, pos_end_dif_g)),
					std::stof(row.substr(pos_start_dif_b, pos_end_dif_b)));
				spe = glm::vec3(std::stof(row.substr(pos_start_spe_r, pos_end_spe_r)),
					std::stof(row.substr(pos_start_spe_g, pos_end_spe_g)),
					std::stof(row.substr(pos_start_spe_b, pos_end_spe_b)));
			}
			if (flag == 0)
			{
				m_scene.push_back(std::shared_ptr<LightSphere>(new LightSphere(r, glm::vec3(pos.x, pos.y, pos.z), amb, dif, spe)));
			}
		}
	}
}


RayTracer::RayTracer(	std::string		scene,
						std::string		out,
						std::string		norm,
						unt				x_res,
						unt				y_res,
						unt				trace_depth,
						toneType		tp) :
camera(Camera()),
width(x_res),
height(y_res),
image(Image(x_res, y_res, tp)),
image_normal(Image(x_res, y_res)),
out_name(out),
norm_name(norm),
m_scene()
{
	FILE *f = fopen(scene.data(), "r");
	if (f == NULL)
		return;

	char buf[1001];
	while (fgets(buf, 1000, f))
	{
		std::string row(buf);
		if (row.find("- camera:") != std::string::npos)
		{
			ScanCamera(f);
		}
		else if (row.find("- node:") != std::string::npos)
		{
			ScanObject(f);
		}
		else if (row.find("- light:") != std::string::npos)
		{
			ScanLight(f);
		}
	}
	fclose(f);
}


void RayTracer::traceRays() {
	float u, v;
	Ray ray;
	ray.origin = glm::vec3(0.0f, 0.0f, 0.0f);
	camera.calculateViewingCoordinates();
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			u = (x - 0.5f * width);
			v = (y - 0.5f * height);
			ray.direction = /*glm::normalize*/(glm::vec3(u, dist, v));
			if (x == width / 2 && y == height / 2)
			{
				x = x;
			}
			PointColor col = traceRay(ray);
			image.setPixelColor(x + width_shift, height - (y + height_shift + 1), col.out);
			image_normal.setPixelColor(x + width_shift, height - (y + height_shift + 1), col.norm);
		}
	}
	image.saveImage(out_name.c_str());
	image_normal.saveImage(norm_name.c_str());
}

PointColor RayTracer::traceRay(Ray ray) {
	glm::vec4 vdir(ray.direction.x, ray.direction.y, ray.direction.z, 1);
	glm::vec4 vorigin(ray.origin.x, ray.origin.y, ray.origin.z, 1);
	vdir = m_cam_inv * vdir;
	vorigin = m_cam_inv * vorigin;
	Ray v;
	v.direction = /*glm::normalize*/(glm::vec3(vdir.x, vdir.y, vdir.z));
	v.origin = glm::vec3(vorigin.x, vorigin.y, vorigin.z);

	Intersector in = m_scene.intersect(v);

	in.mul_on_mat(m_cam_mat);

	if (in.size() > 0) {
		Intersection r = in.getFirst();
		if (r.param <= 0)
			return PointColor(Color(0, 0, 0), Color(0, 0, 0));

		if (r.normal.x == 0 && r.normal.y == 0 && r.normal.z == 0)
		{
			float norm_z = glm::normalize(r.normal).z;
			return PointColor(Color(r.m_base.r, r.m_base.g, r.m_base.b), Color((unc)abs(norm_z * 2550.f)));
		}

		r.normal = glm::normalize(r.normal);
		//get light
		for (unt i = 0; i < m_scene.m_scene.size(); i++)
		{
			Ray lightRay;
			if (m_scene.m_scene[i]->isLight())
			{
				glm::vec4 res = m_cam_inv * glm::vec4(r.point, 1);
				lightRay.origin = glm::vec3(res.x, res.y, res.z);
				LightSphere *light = (LightSphere *)m_scene.m_scene[i].operator->();
				glm::vec3 lightPointMin(light->getPos().x - light->radius, light->getPos().y - light->radius, light->getPos().z - light->radius);
				glm::vec3 lightPointMax(light->getPos().x + light->radius, light->getPos().y + light->radius, light->getPos().z + light->radius);
				glm::vec3 lightPoint(lightPointMin);
				int faces[3];
				faces[0] = (fabs(lightRay.origin.z - lightPointMin.z) < fabs(lightRay.origin.z - lightPointMax.z)) ? 0 : 1;
				faces[1] = (fabs(lightRay.origin.y - lightPointMin.y) < fabs(lightRay.origin.y - lightPointMax.y)) ? 2 : 3;
				faces[2] = (fabs(lightRay.origin.x - lightPointMin.x) < fabs(lightRay.origin.x - lightPointMax.x)) ? 4 : 5;
				int last = 0;
				while (true)
				{
					if (last == 3)
						break;
					static const float delta = 10;
					switch (faces[last])
					{
					case 0:
						if (lightPoint.y < lightPointMax.y)
							lightPoint.y += delta;
						else
						{
							lightPoint.y = lightPointMin.y;
							if (lightPoint.x < lightPointMax.x)
								lightPoint.x += delta;
							else
							{
								last++; lightPoint.x = lightPointMin.x; lightPoint.z = lightPointMin.z;
							}
						}
						break;
					case 1:
						lightPoint.z = lightPointMax.z;
						if (lightPoint.y < lightPointMax.y)
							lightPoint.y += delta;
						else
						{
							lightPoint.y = lightPointMin.y;
							if (lightPoint.x < lightPointMax.x)
								lightPoint.x += delta;
							else
							{
								last++; lightPoint.x = lightPointMin.x; lightPoint.z = lightPointMin.z;
							}
						}
						break;
					case 2:
						if (lightPoint.z < lightPointMax.z)
							lightPoint.z += delta;
						else
						{
							lightPoint.z = lightPointMin.z;
							if (lightPoint.x < lightPointMax.x)
								lightPoint.x += delta;
							else
							{
								last++; lightPoint.x = lightPointMin.x; lightPoint.y = lightPointMin.y;
							}
						}
						break;
					case 3:
						lightPoint.y = lightPointMax.y;
						if (lightPoint.z < lightPointMax.z)
							lightPoint.z += delta;
						else
						{
							lightPoint.z = lightPointMin.z;
							if (lightPoint.x < lightPointMax.x)
								lightPoint.x += delta;
							else
							{
								last++; lightPoint.x = lightPointMin.x; lightPoint.y = lightPointMin.y;
							}
						}
						break;
					case 4:
						if (lightPoint.z < lightPointMax.z)
							lightPoint.z += delta;
						else
						{
							lightPoint.z = lightPointMin.z;
							if (lightPoint.y < lightPointMax.y)
								lightPoint.y += delta;
							else
							{
								last++; lightPoint.y = lightPointMin.y; lightPoint.x = lightPointMin.x;
							}
						}
						break;
					case 5:
						lightPoint.x = lightPointMax.x;
						if (lightPoint.z < lightPointMax.z)
							lightPoint.z += delta;
						else
						{
							lightPoint.z = lightPointMin.z;
							if (lightPoint.y < lightPointMax.y)
								lightPoint.y += delta;
							else
							{
								last++;
							}
						}
						break;
					}
					lightRay.direction = glm::normalize(lightPoint - lightRay.origin);
					Intersector lt = m_scene.intersectRot(lightRay, false);
					if (!lt.hasPointsBetween(lightRay.origin, light->getPos()))
					{
						glm::mat4x4 mat_norm(
							m_cam_inv[0][0], m_cam_inv[0][1], m_cam_inv[0][2], 0,
							m_cam_inv[1][0], m_cam_inv[1][1], m_cam_inv[1][2], 0,
							m_cam_inv[2][0], m_cam_inv[2][1], m_cam_inv[2][2], 0,
							0, 0, 0, 1);
						glm::vec4 res = mat_norm * glm::vec4(r.normal, 1);
						glm::vec3 normal = glm::vec3(res.x, res.y, res.z);
						float difCoef = glm::dot(glm::normalize(lightRay.direction), normal);
						if (difCoef <= 0)
							difCoef = 0.0000f;
						glm::vec3 R = 2 * glm::dot(lightRay.direction, normal)*normal - lightRay.direction;
						glm::vec3 V = lightRay.origin - camera.getPosition();
						float specCoef = pow(glm::dot(glm::normalize(R), glm::normalize(V)), r.m_a);
						if (specCoef <= 0)
							specCoef = 0.0000f;
						r.m_base.r += r.m_amb.r * light->m_amb.r;
						r.m_base.r += r.m_dif.r * light->m_dif.r * difCoef;
						r.m_base.r += r.m_spec.r * light->m_spec.r * specCoef;
						r.m_base.g += r.m_amb.g * light->m_amb.g;
						r.m_base.g += r.m_dif.g * light->m_dif.g * difCoef;
						r.m_base.g += r.m_spec.g * light->m_spec.g * specCoef;
						r.m_base.b += r.m_amb.b * light->m_amb.b;
						r.m_base.b += r.m_dif.b * light->m_dif.b * difCoef;
						r.m_base.b += r.m_spec.b * light->m_spec.b * specCoef;
						break;
					}
				}
			}
		}

		float norm_z = glm::normalize(r.normal).z;
		return PointColor(Color(r.m_base.r, r.m_base.g, r.m_base.b), Color((unc)abs(norm_z)));
	}
	else
	{
		return PointColor(Color(0, 0, 0), Color(0, 0, 0));

	}
}