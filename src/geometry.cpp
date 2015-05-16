#include "geometry.h"
#include <stdio.h>
#include <algorithm>

void GroupObject::push_back(std::shared_ptr<Object> obj)
{
	m_scene.push_back(obj);
}

Intersector GroupObject::intersectRot(Ray ray, bool needRotate)
{
	glm::vec4 vdir(ray.direction.x, ray.direction.y, ray.direction.z, 1);
	glm::vec4 vorigin(ray.origin.x, ray.origin.y, ray.origin.z, 1);
	glm::mat4x4 inv_dir(
		m_inv[0][0], m_inv[0][1], m_inv[0][2], 0, 
		m_inv[1][0], m_inv[1][1], m_inv[1][2], 0,
		m_inv[2][0], m_inv[2][1], m_inv[2][2], 0,
		0          ,  0         ,  0, 1);
	vdir = inv_dir * vdir;
	vorigin = m_inv * vorigin;
	Ray v;
	v.direction = /*glm::normalize*/(glm::vec3(vdir.x, vdir.y, vdir.z));
	v.origin = glm::vec3(vorigin.x, vorigin.y, vorigin.z);
	Intersector res(ray);
	for (unt i = 0; i < m_scene.size(); i++)
	{
		if (m_scene[i]->isLight() && !needRotate)
			continue;
		Intersector r = m_scene[i]->intersect(v);
		if (i > 0)
		{
			switch (m_csg_type)
			{
			case CSG_NONE:
			case CSG_UNION: res.csg_union(r); break;
			case CSG_DIFFERENCE: res.csg_difference(r); break;
			case CSG_INTERSECTION: res.csg_intersection(r); break;
			}
		}
		else
			res.push_back(r);
	}
		res.mul_on_mat(m_mat);
	res.add_col(m_amb, m_dif, m_spec, m_base, m_a);
	return res;
}

void Intersector::add_col(glm::vec3 amb, glm::vec3 dif, glm::vec3 spec, glm::vec3 base, float a)
{
	for (unt i = 0; i < m_intersections.size(); i++)
	{
		m_intersections[i].m_amb += amb;
		m_intersections[i].m_dif += dif;
		m_intersections[i].m_spec += spec;
		m_intersections[i].m_base += base;
		m_intersections[i].m_a += a;
	}
}

void Intersector::mul_on_mat(glm::mat4x4 &mat)
{
	for (unt i = 0; i < m_intersections.size(); i++)
	{
		glm::vec4 vec(m_intersections[i].point.x, m_intersections[i].point.y, m_intersections[i].point.z, 1.f);
		vec = mat * vec;
		m_intersections[i].point.x = vec.x;
		m_intersections[i].point.y = vec.y;
		m_intersections[i].point.z = vec.z;

		if (m_intersections[i].normal.x != 0 || m_intersections[i].normal.y != 0 || m_intersections[i].normal.z != 0)
		{
			glm::vec4 vect(m_intersections[i].normal.x, m_intersections[i].normal.y, m_intersections[i].normal.z, 1.f);
			glm::mat4x4 mat_norm(
				mat[0][0], mat[0][1], mat[0][2], 0,
				mat[1][0], mat[1][1], mat[1][2], 0,
				mat[2][0], mat[2][1], mat[2][2], 0,
				0, 0, 0, 1);
			vect = mat_norm * vect;
			m_intersections[i].normal.x = vect.x;
			m_intersections[i].normal.y = vect.y;
			m_intersections[i].normal.z = vect.z;
		}

		float k;
		if (abs(m_ray.direction.x) > abs(m_ray.direction.y) && abs(m_ray.direction.x) > abs(m_ray.direction.z))
			k = (m_intersections[i].point.x - m_ray.origin.x) / m_ray.direction.x;
		else if (abs(m_ray.direction.y) > abs(m_ray.direction.z))
			k = (m_intersections[i].point.y - m_ray.origin.y) / m_ray.direction.y;
		else
			k = (m_intersections[i].point.z - m_ray.origin.z) / m_ray.direction.z;

		m_intersections[i].param = k;
	}
}

bool Intersector::sort()
{
	struct paramCmpr
	{
		bool operator() (Intersection &i, Intersection &j)
		{
			return i.param < j.param;
		}
	};
	paramCmpr cmpr;
	std::sort(m_intersections.begin(), m_intersections.end(), cmpr);
	return true;
}

enum geo_state { GEO_OUT, GEO_IN_1_, GEO_IN_2_, GEO_IN_1_2_ };

bool Intersector::csg_union(Intersector &in)
{
	if (in.size() == 0)
		return true;
	if (m_intersections.size() == 0)
	{
		push_back(in);
		return true;
	}
	sort();
	in.sort();

	unt i1 = 0, i2 = 0;
	geo_state state = GEO_OUT;
	std::vector<Intersection> new_intersections;
	while (i1 < m_intersections.size() || i2 < in.m_intersections.size())
	{
		if (i2 >= in.m_intersections.size() || (i1 < m_intersections.size() && m_intersections[i1].param < in.m_intersections[i2].param))
		{
			switch (state)
			{
			case GEO_OUT:
				new_intersections.push_back(m_intersections[i1]);
				if (m_intersections[i1].type == _3D_FIG)
					state = GEO_IN_1_;
				break;
			case GEO_IN_1_:
				if (m_intersections[i1].type == _3D_FIG)
				{
					new_intersections.push_back(m_intersections[i1]);
					state = GEO_OUT;
				}
				break;
			case GEO_IN_2_:
				if (m_intersections[i1].type == _3D_FIG)
					state = GEO_IN_1_2_;
				break;
			case GEO_IN_1_2_:
				if (m_intersections[i1].type == _3D_FIG)
					state = GEO_IN_2_;
				break;
			}
			i1++;
		}
		else
		{
			switch (state)
			{
			case GEO_OUT:
				new_intersections.push_back(in.m_intersections[i2]);
				if (in.m_intersections[i2].type == _3D_FIG)
					state = GEO_IN_2_;
				break;
			case GEO_IN_2_:
				if (in.m_intersections[i2].type == _3D_FIG)
				{
					new_intersections.push_back(in.m_intersections[i2]);
					state = GEO_OUT;
				}
				break;
			case GEO_IN_1_:
				if (in.m_intersections[i2].type == _3D_FIG)
					state = GEO_IN_1_2_;
				break;
			case GEO_IN_1_2_:
				if (in.m_intersections[i2].type == _3D_FIG)
					state = GEO_IN_1_;
				break;
			}
			i2++;
		}
	}
	m_intersections = new_intersections;

	return true;
}

bool Intersector::csg_intersection(Intersector &in)
{
	std::vector<Intersection> new_intersections;
	if (in.size() == 0 || m_intersections.size() == 0)
	{
		m_intersections = new_intersections;
		return true;
	}
	sort();
	in.sort();
	
	unt i1 = 0, i2 = 0;
	geo_state state = GEO_OUT;
	while (i1 < m_intersections.size() || i2 < in.m_intersections.size())
	{
		if (i2 >= in.m_intersections.size() || (i1 < m_intersections.size() && m_intersections[i1].param < in.m_intersections[i2].param))
		{
			switch (state)
			{
			case GEO_OUT:
				if (m_intersections[i1].type == _3D_FIG)
					state = GEO_IN_1_;
				break;
			case GEO_IN_1_:
				if (m_intersections[i1].type == _3D_FIG)
					state = GEO_OUT;
				break;
			case GEO_IN_2_:
				new_intersections.push_back(m_intersections[i1]);
				if (m_intersections[i1].type == _3D_FIG)
					state = GEO_IN_1_2_;
				break;
			case GEO_IN_1_2_:
				if (m_intersections[i1].type == _3D_FIG)
				{
					new_intersections.push_back(m_intersections[i1]);
					state = GEO_IN_2_;
				}
				break;
			}
			i1++;
		}
		else
		{
			switch (state)
			{
			case GEO_OUT:
				if (in.m_intersections[i2].type == _3D_FIG)
					state = GEO_IN_2_;
				break;
			case GEO_IN_2_:
				if (in.m_intersections[i2].type == _3D_FIG)
					state = GEO_OUT;
				break;
			case GEO_IN_1_:
				new_intersections.push_back(in.m_intersections[i2]);
				if (in.m_intersections[i2].type == _3D_FIG)
					state = GEO_IN_1_2_;
				break;
			case GEO_IN_1_2_:
				if (in.m_intersections[i2].type == _3D_FIG)
				{
					new_intersections.push_back(in.m_intersections[i2]);
					state = GEO_IN_1_;
				}
				break;
			}
			i2++;
		}
	}
	m_intersections = new_intersections;
	return true;
}

bool Intersector::csg_difference(Intersector &in)
{
	if (in.size() == 0 || m_intersections.size() == 0)
	{
		return true;
	}
	sort();
	in.sort();
	unt i1 = 0, i2 = 0;
	geo_state state = GEO_OUT;
	std::vector<Intersection> new_intersections;
	while (i1 < m_intersections.size() || i2 < in.m_intersections.size())
	{
		if (i2 >= in.m_intersections.size() || (i1 < m_intersections.size() && m_intersections[i1].param < in.m_intersections[i2].param))
		{
			switch (state)
			{
			case GEO_OUT:
				new_intersections.push_back(m_intersections[i1]);
				if (m_intersections[i1].type == _3D_FIG)
					state = GEO_IN_1_;
				break;
			case GEO_IN_1_:
				if (m_intersections[i1].type == _3D_FIG)
				{
					new_intersections.push_back(m_intersections[i1]);
					state = GEO_OUT;
				}
				break;
			case GEO_IN_2_:
				if (m_intersections[i1].type == _3D_FIG)
				{
					state = GEO_IN_1_2_;
				}
				break;
			case GEO_IN_1_2_:
				if (m_intersections[i1].type == _3D_FIG)
				{
					state = GEO_IN_2_;
				}
			}
			i1++;
		}
		else
		{
			switch (state)
			{
			case GEO_OUT:
				if (in.m_intersections[i2].type == _3D_FIG)
					state = GEO_IN_2_;
				break;
			case GEO_IN_2_:
				if (in.m_intersections[i2].type == _3D_FIG)
				{
					state = GEO_OUT;
				}
				break;
			case GEO_IN_1_:
				if (in.m_intersections[i2].type == _3D_FIG)
				{
					new_intersections.push_back(in.m_intersections[i2]);
					state = GEO_IN_1_2_;
				}
				break;
			case GEO_IN_1_2_:
				if (in.m_intersections[i2].type == _3D_FIG)
					new_intersections.push_back(in.m_intersections[i2]);
					state = GEO_IN_1_;
				break;
			}
			i2++;
		}
	}
	m_intersections = new_intersections;

	return true;
}

bool Intersector::push_back(Intersector &points)
{
	m_intersections.insert(m_intersections.end(), points.m_intersections.begin(), points.m_intersections.end());
	return true;
}

bool Intersector::push_back(Intersection &point)
{
	float k;
	if (abs(m_ray.direction.x) > abs(m_ray.direction.y) && abs(m_ray.direction.x) > abs(m_ray.direction.z))
		k = (point.point.x - m_ray.origin.x) / m_ray.direction.x;
	else if (abs(m_ray.direction.y) > abs(m_ray.direction.z))
		k = (point.point.y - m_ray.origin.y) / m_ray.direction.y;
	else
		k = (point.point.z - m_ray.origin.z) / m_ray.direction.z;

	point.param = k;
	m_intersections.push_back(point);

	return true;
}

Intersection Intersector::getFirst()
{
	float min_param = FLT_MAX;
	unt ind = 0;
	for (unt i = 0; i < m_intersections.size(); i++)
	{
		if (m_intersections[i].param >= 0 && m_intersections[i].param < min_param)
		{
			min_param = m_intersections[i].param;
			ind = i;
		}
	}
	return m_intersections[ind];
}

bool Intersector::hasPointsBetween(glm::vec3 a, glm::vec3 b)
{
	glm::vec3 min, max;
	min.x = (a.x < b.x) ? a.x : b.x; max.x = (a.x > b.x) ? a.x : b.x;
	min.y = (a.y < b.y) ? a.y : b.y; max.y = (a.y > b.y) ? a.y : b.y;
	min.z = (a.z < b.z) ? a.z : b.z; max.z = (a.z > b.z) ? a.z : b.z;
	for (unt i = 0; i < m_intersections.size(); i++)
	{
		if ((m_intersections[i].point.x < max.x - 1 && m_intersections[i].point.x > min.x + 1) ||
			(m_intersections[i].point.y < max.y - 1 && m_intersections[i].point.y > min.y + 1) ||
			(m_intersections[i].point.z < max.z - 1 && m_intersections[i].point.z > min.z + 1))
		{
			return true;
		}
	}
	return false;
}

Intersector intersectPlane(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, Ray ray) {
	Intersector res(ray);
	glm::vec3 edge1, edge2;
	glm::vec3 P, Q, T;
	glm::vec3 dir = ray.getDirection();
	glm::vec3 orig = ray.getOrigin();
	float det, inv_det, u, v;
	float t;
	edge1 = p1 - p0;
	edge2 = p2 - p0;
	P = glm::cross(dir, edge2);
	det = glm::dot(edge1, P);

	inv_det = 1.f / det;
	T = orig - p0;
	u = glm::dot(T, P) * inv_det;

	Q = glm::cross(T, edge1);
	v = glm::dot(dir, Q) * inv_det;

	t = glm::dot(edge2, Q) * inv_det;


	if (t > FLT_EPSILON) {
		Intersection inter;
		inter.param = t;
		inter.point = orig + t * dir;
		inter.normal = glm::normalize(glm::cross(edge1, edge2));
		inter.type = _2D_FIG;
		res.push_back(inter);
	}
	return res;
}

Intersector Plane::intersect(Ray ray) {
	return intersectPlane(p0, p1, p2, ray);
}

//Muller-Trumbore
Intersector Triangle::intersect(Ray ray) {
	Intersector res(ray);
	glm::vec3 edge1, edge2;
	glm::vec3 P, Q, T;
	glm::vec3 dir = ray.getDirection();
	glm::vec3 orig = ray.getOrigin();
	float det, inv_det, u, v;
	float t;
	edge1 = p1 - p0;
	edge2 = p2 - p0;
	P = glm::cross(dir, edge2);
	det = glm::dot(edge1, P);
	if (det > -FLT_EPSILON && det < FLT_EPSILON) {
		return res;
	}

	inv_det = 1.f / det;
	T = orig - p0;
	u = glm::dot(T, P) * inv_det;
	if (u < 0.f || u > 1.f) {
		return res;
	}

	Q = glm::cross(T, edge1);
	v = glm::dot(dir, Q) * inv_det;
	if (v < 0.f || u + v  > 1.f) {
		return res;
	}

	t = glm::dot(edge2, Q) * inv_det;

	if (t > FLT_EPSILON) {
		Intersection inter;
		inter.param = t;
		inter.normal = glm::normalize(glm::cross(edge1, edge2));
		inter.point = orig + t * dir;
		inter.type = _2D_FIG;
		res.push_back(inter);
	}
	return res;
}


inline float maxf(float a, float b, float c)
{
	return (a > c) ? ((a > b) ? a : b) : ((c > b) ? c : b);
}


Intersector LightSphere::intersect(Ray ray) {
	glm::vec4 vdir(ray.direction.x, ray.direction.y, ray.direction.z, 1);
	glm::vec4 vorigin(ray.origin.x, ray.origin.y, ray.origin.z, 1);
	glm::mat4x4 inv_dir(
		m_inv[0][0], m_inv[0][1], m_inv[0][2], 0,
		m_inv[1][0], m_inv[1][1], m_inv[1][2], 0,
		m_inv[2][0], m_inv[2][1], m_inv[2][2], 0,
		0, 0, 0, 1);
	vdir = inv_dir * vdir;
	vorigin = m_inv * vorigin;
	Ray v;
	v.direction = /*glm::normalize*/(glm::vec3(vdir.x, vdir.y, vdir.z));
	v.origin = glm::vec3(vorigin.x, vorigin.y, vorigin.z);

	Intersector res(ray);
	glm::vec3 dir = v.getDirection();
	glm::vec3 orig = v.getOrigin();
	float a = glm::dot(dir, dir);
	float b = glm::dot(dir, orig);
	float c = glm::dot(orig, orig) - radius * radius;

	float rad = b * b - a * c;
	if (rad < 0) {
		return res;
	}

	float x1 = (-b - sqrtf(rad)) / a;
	float x2 = (-b + sqrtf(rad)) / a;

	if (fabs(x1) > FLT_EPSILON) {
		Intersection inter;
		inter.param = x1;
		inter.normal = glm::vec3(0,0,0);
		inter.point = orig + x1 * dir;
		inter.type = _3D_FIG;
		inter.m_base.r = maxf(m_amb.r, m_spec.r, m_dif.r);
		inter.m_base.g = maxf(m_amb.g, m_spec.g, m_dif.g);
		inter.m_base.b = maxf(m_amb.b, m_spec.b, m_dif.b);
		res.push_back(inter);
	}

	if (fabs(x2) > FLT_EPSILON) {
		Intersection inter;
		inter.param = x2;
		inter.normal = glm::vec3(0, 0, 0);
		inter.point = orig + x2 * dir;
		inter.type = _3D_FIG;
		inter.m_base.r = maxf(m_amb.r, m_spec.r, m_dif.r);
		inter.m_base.g = maxf(m_amb.g, m_spec.g, m_dif.g);
		inter.m_base.b = maxf(m_amb.b, m_spec.b, m_dif.b);
		res.push_back(inter);
	}

	res.mul_on_mat(m_mat);

	return res;
}


Intersector Sphere::intersect(Ray ray) {
	Intersector res(ray);
	glm::vec3 dir = ray.getDirection();
	glm::vec3 orig = ray.getOrigin();
	float a = glm::dot(dir, dir);
	float b = glm::dot(dir, orig);
	float c = glm::dot(orig, orig) - radius * radius;

	float rad = b * b - a * c;
	if (rad < 0) {
		return res;
	}

	float x1 = (-b - sqrtf(rad)) / a;
	float x2 = (-b + sqrtf(rad)) / a;

		Intersection inter;
		inter.param = x1;
		inter.normal = glm::normalize((orig + x1 * dir) / radius);
		inter.point = orig + x1 * dir;
		inter.type = _3D_FIG;
		res.push_back(inter);

	{
		Intersection inter;
		inter.param = x2;
		inter.normal = glm::normalize((orig + x2 * dir) / radius);
		inter.point = orig + x2 * dir;
		inter.type = _3D_FIG;
		res.push_back(inter);
	}

	return res;
}

bool intcyl(const glm::vec3 &raybase, const glm::vec3 &raycos, float radius, glm::vec3 *in, glm::vec3 *out);

Intersector Cylinder::intersect(Ray ray) {
	Intersector res(ray), r1(ray), r2(ray);

	Intersection in, out;
	in.type = _3D_FIG;
	out.type = _3D_FIG;
	if (intcyl(ray.origin, ray.direction, radius, &(in.point), &(out.point)))
	{
		if (in.point.z <= height && in.point.z >= -height)
		{
			in.normal = glm::normalize(glm::vec3(in.point.x, in.point.y, 0));
			res.push_back(in);
		}
		if (out.point.z <= height && out.point.z >= -height)
		{
			out.normal = glm::normalize(glm::vec3(out.point.x, out.point.y, 0));
			res.push_back(out);
		}
		r1 = intersectPlane(glm::vec3(0, 0, height), glm::vec3(0, 1, height), glm::vec3(1, 0, height), ray);
		if (r1.size() != 0 && r1[0].point.x*r1[0].point.x + r1[0].point.y*r1[0].point.y <= radius*radius)
		{
			r1[0].type = _3D_FIG;
			res.push_back(r1[0]);
		}
		r2 = intersectPlane(glm::vec3(0, 0, -height), glm::vec3(0, 1, -height), glm::vec3(1, 0, -height), ray);
		if (r2.size() != 0 && r2[0].point.x*r2[0].point.x + r2[0].point.y*r2[0].point.y <= radius*radius)
		{
			r2[0].type = _3D_FIG;
			res.push_back(r2[0]);
		}
		return res;
	}

	return res;
}

bool intcon(const glm::vec3 &ori, const glm::vec3 &dir, float r_div_h, glm::vec3 *in, glm::vec3 *out);

Intersector Cone::intersect(Ray ray) {
	Intersector res(ray), r2(ray);

	Intersection in, out;
	in.type = _3D_FIG;
	out.type = _3D_FIG;
	if (intcon(ray.origin, ray.direction, radius / height, &(in.point), &(out.point)))
	{
		if (in.point.z <= 0 && in.point.z >= -height)
		{
			in.normal = glm::normalize(glm::vec3(in.point.x, in.point.y, 
				-radius / height * sqrt(in.point.x*in.point.x + in.point.y*in.point.y)));
			res.push_back(in);
		}
		if (out.point.z <= 0 && out.point.z >= -height)
		{
			out.normal = glm::normalize(glm::vec3(out.point.x, out.point.y,
				-radius / height * sqrt(out.point.x*out.point.x + out.point.y*out.point.y)));
			res.push_back(out);
		}
		r2 = intersectPlane(glm::vec3(0, 0, -height), glm::vec3(0, 1, -height), glm::vec3(1, 0, -height), ray);
		if (r2.size() != 0 && r2[0].point.x*r2[0].point.x + r2[0].point.y*r2[0].point.y <= radius*radius)
		{
			r2[0].type = _3D_FIG;
			res.push_back(r2[0]);
		}
		return res;
	}

	return res;
}


int   SolveP4(double *x, double a, double b, double c, double d);	// solve equation x^4 + a*x^3 + b*x^2 + c*x + d = 0 by Dekart-Euler method

Intersector Torus::intersect(Ray ray) {
	Intersector res(ray);
	glm::vec3 dir = glm::normalize(ray.direction);

	double sv = glm::dot(ray.origin, dir), a = 4 * sv;
	double r1_deg_2 = radius*radius, r2_deg_2 = tube_radius*tube_radius, s_deg_2 = glm::dot(ray.origin, ray.origin);
	double b = 2 * (s_deg_2 + r1_deg_2 - r2_deg_2) - 4 * r1_deg_2 * (1 - dir.z*dir.z) + 4 * sv * sv;
	double c = 8 * r1_deg_2 * ray.origin.z * dir.z + a * (s_deg_2 - r1_deg_2 - r2_deg_2);
	double d = pow(ray.origin.x, 4) + pow(ray.origin.y, 4) + pow(ray.origin.z, 4) + (r1_deg_2 - r2_deg_2)*(r1_deg_2 - r2_deg_2) +
		2 * (ray.origin.x*ray.origin.y + ray.origin.z*(r1_deg_2 - r2_deg_2) + 
		(pow(ray.origin.x, 2) + pow(ray.origin.y, 2))*(s_deg_2 - r1_deg_2 - r2_deg_2));

	double x[4];

	int n = SolveP4(x, a, b, c, d);

	float koef;
	glm::vec3 pnt, norm;
	switch (n)
	{
	case 4:
		pnt = ray.origin + dir * float(x[3]);
		koef = radius / sqrt(pnt.x * pnt.x + pnt.y * pnt.y);
		norm.x = (1 - koef) * pnt.x;
		norm.y = (1 - koef) * pnt.y;
		norm.z = pnt.z;
		res.push_back(Intersection(pnt, normalize(norm), _3D_FIG));
		
		pnt = ray.origin + dir * float(x[3]);
		koef = radius / sqrt(pnt.x * pnt.x + pnt.y * pnt.y);
		norm.x = (1 - koef) * pnt.x;
		norm.y = (1 - koef) * pnt.y;
		norm.z = pnt.z;
		res.push_back(Intersection(pnt, normalize(norm), _3D_FIG));
	case 2:
		res.push_back(Intersection(pnt, normalize(norm), _3D_FIG));
		pnt = ray.origin + dir * float(x[3]);
		koef = radius / sqrt(pnt.x * pnt.x + pnt.y * pnt.y);
		norm.x = (1 - koef) * pnt.x;
		norm.y = (1 - koef) * pnt.y;
		norm.z = pnt.z;

		res.push_back(Intersection(pnt, normalize(norm), _3D_FIG));
		pnt = ray.origin + dir * float(x[3]);
		koef = radius / sqrt(pnt.x * pnt.x + pnt.y * pnt.y);
		norm.x = (1 - koef) * pnt.x;
		norm.y = (1 - koef) * pnt.y;
		norm.z = pnt.z;
		break;
	}

	return res;
}


ObjModel::ObjModel(std::string obj_file)
{
	std::vector<glm::vec3> verts;
	FILE *f = fopen(obj_file.data(), "r");
	char c1, c2, c3;
	while (fscanf(f, "%c", &c1))
	{
		if (isspace(c1) && c1 != ' ' && c1 != '\t')
		{
			check:
			if (fscanf(f, "%c%c", &c2, &c3) < 2)
				return;
			if (c3 != ' ')
				continue;
			if (c2 == 'v')
			{
				float vx, vy, vz;
				unt n = fscanf(f, "%f%f%f", &vx, &vy, &vz);
				if (n >= 3)
				{
					verts.push_back(glm::vec3(vx, vy, vz));
				}
			}
			if (c2 == 'f')
			{
				unt v1, v2, v3, v4;
				unt n = fscanf(f, "%u", &v1);
				if (n >= 1)
				{
					while (fscanf(f, "%c", &c1))
						if (c1 == ' ')
							break;
					n = fscanf(f, "%u", &v2);
					if (n >= 1)
					{
						while (fscanf(f, "%c", &c1))
							if (c1 == ' ')
								break;
						n = fscanf(f, "%u", &v3);
						if (n >= 1)
						{
							m_triangles.push_back(Triangle(verts[v1-1], verts[v2-1], verts[v3-1]));
							while (fscanf(f, "%c", &c1))
								if (isspace(c1))
									if (c1 == ' ')
										break;
									else
										goto check;

							n = fscanf(f, "%u", &v4);
							if (n >= 1)
							{
								m_triangles.push_back(Triangle(verts[v1 - 1], verts[v4 - 1], verts[v3 - 1]));
							}
						}
					}
				}
			}
		}
	}
}

Intersector ObjModel::intersect(Ray ray)
{
	Intersector res(ray);
	for (unt i = 0; i < m_triangles.size(); i++)
	{
		Intersector r = m_triangles[i].intersect(ray);
		if (r.size())
			res.push_back(r[0]);
	}
	return res;
}

