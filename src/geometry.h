#ifndef GEOMETRY_H_INCLUDED
#define GEOMETRY_H_INCLUDED

#include <string>
#include <vector>
#include <memory>
#include "utils.h"
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum intersection_type { _2D_FIG, _3D_FIG };

struct Intersection {
	Intersection() { init(); }
	Intersection(glm::vec3 point, glm::vec3 normal, intersection_type tp) :point(point), normal(normal), param(0.f), type(tp){ init(); }
	Intersection(glm::vec3 point, glm::vec3 normal, float param, intersection_type tp) :point(point), normal(normal), param(param), type(tp){ init(); }

	float param;
	glm::vec3 point;
	glm::vec3 normal;
	intersection_type type;
	glm::vec3 m_amb, m_dif, m_spec, m_base;
	float m_a;

private:
	void init(){ m_amb = glm::vec3(0, 0, 0); m_dif = glm::vec3(0, 0, 0); m_spec = glm::vec3(0, 0, 0); m_base = glm::vec3(0, 0, 0); m_a = 0; }
};


struct Intersector {
	std::vector<Intersection> m_intersections;
	Ray m_ray;
	Intersector(Ray &ray) : m_ray(ray) {}

	bool sort();
	bool push_back(Intersection &point);
	bool push_back(Intersector &points);
	void mul_on_mat(glm::mat4x4 &mat);
	void add_col(glm::vec3 amb, glm::vec3 dif, glm::vec3 spec, glm::vec3 base, float a);
	unt size() { return m_intersections.size(); }
	Intersection &operator [] (unt pos){ return m_intersections[pos]; }

	bool csg_union(Intersector &in);
	bool csg_intersection(Intersector &in);
	bool csg_difference(Intersector &in);

	Intersection getFirst();
	bool hasPointsBetween(glm::vec3 a, glm::vec3 b);
};


class Object {
public:
	virtual Intersector intersect(Ray ray) = 0 {};
	virtual bool isLight() { return false; }
	virtual glm::vec3 getPos() { return glm::vec3(0,0,0); }
};

enum csg_type { CSG_NONE, CSG_UNION, CSG_INTERSECTION, CSG_DIFFERENCE };

class GroupObject : public Object  {
public:
	GroupObject() :m_mat(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1), m_inv(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1){ init(); }
	virtual Intersector intersectRot(Ray ray, bool needRotate = true);
	virtual Intersector intersect(Ray ray) { return intersectRot(ray, true); }
	void push_back(std::shared_ptr<Object> obj);

	glm::mat4x4 m_mat;
	glm::mat4x4 m_inv;

	csg_type m_csg_type;
	glm::vec3 m_amb, m_dif, m_spec, m_base;
	float m_a;

	std::vector<std::shared_ptr<Object>> m_scene;
private:
	void init(){ m_amb = glm::vec3(0, 0, 0); m_dif = glm::vec3(0, 0, 0); m_spec = glm::vec3(0, 0, 0); m_base = glm::vec3(0, 0, 0); m_a = 0; }
};


class LightSphere : public GroupObject {
public:
	LightSphere(float r, glm::vec3 pos, glm::vec3 amb, glm::vec3 dif, glm::vec3 spe) : radius(r), pos(pos.x, pos.y, pos.z)
	{
		m_amb = amb; m_dif = dif; m_spec = spe; 
		m_mat = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, pos.z));
		m_inv = inverse(m_mat);
		if (radius < 1)
			radius = 1;
		radius *= 0.8;
	};
	virtual Intersector intersect(Ray ray);
	virtual bool isLight() { return true; }
	virtual glm::vec3 getPos() { return pos; }
	float radius;
private:
	glm::vec3 pos;
};


class Plane : public Object {
public:
	Plane(glm::vec3 &p0, glm::vec3 &p1, glm::vec3 &p2) : p1(p1), p2(p2), p0(p0) {}
	virtual Intersector intersect(Ray ray);
private:
	glm::vec3 p0, p1, p2;
};


class Triangle : public Object {
public:
	Triangle(glm::vec3 &p0, glm::vec3 &p1, glm::vec3 &p2) : p1(p1), p2(p2), p0(p0) {}
	virtual Intersector intersect(Ray ray);
private:
	glm::vec3 p0, p1, p2;
};

class Sphere : public Object {
public:
	Sphere(float r) : radius(r) {};
	virtual Intersector intersect(Ray ray);
private:
	float radius;
};



class Cylinder : public Object {
public:
	Cylinder(float r, float h) : radius(r), height(h*0.5f) {};
	virtual Intersector intersect(Ray ray);
private:
	float radius, height;
};

class Cone : public Object {
public:
	Cone(float r, float h) : radius(r), height(h) {};
	virtual Intersector intersect(Ray ray);
private:
	float radius, height;
};

class Torus : public Object {
public:
	Torus(float r, float rt) : radius(r), tube_radius(rt) {};
	virtual Intersector intersect(Ray ray);
private:
	float radius, tube_radius;
};

class ObjModel : public Object {
public:
	ObjModel(std::string obj_file);

	Intersector intersect(Ray ray);
private:
	std::vector<Triangle> m_triangles;
};


#endif