﻿#version 440 core
layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) uniform image2D img_output;

uniform float u_FocusDist;
uniform vec3 u_CameraPosn;
uniform vec3 u_CameraDirn;

struct Ray
{
	vec3 Orig;
	vec3 Dirn;
};
struct Plane3D
{
	vec4 Eqn;
};
struct Sphere 
{
	float Radius;
	vec3 Centre;
};

uniform bool u_ShowNormal;
uniform Sphere u_Sphere;

float DistanceFrom(Ray ray, vec3 pt){
	return length(cross(pt - ray.Orig, ray.Dirn));
};
float DistanceFrom(Plane3D plane, vec3 pt){
	float num = (plane.Eqn.x * pt.x + plane.Eqn.y * pt.y + plane.Eqn.z * pt.z) + plane.Eqn.w;
	float den = length(plane.Eqn.xyz);
	return num / den;
}
Ray CreateRay (vec3 pt1, vec3 pt2)
{
	return Ray (pt1, normalize (pt2 - pt1));
};
Plane3D CreatePlane (vec3 pt1, vec3 pt2, vec3 pt3)
{
	vec4 param = vec4(normalize(cross(pt2 - pt1, pt3 - pt1)), 0);
	param.w = -(param.x * pt1.x + param.y * pt1.y + param.z * pt1.z);
	
	return Plane3D (param);
};
Plane3D CreatePlane (Ray ray, vec3 pt)
{
	vec4 param = vec4(normalize(cross(ray.Dirn, ray.Orig - pt)), 0);
	param.w = -(param.x * pt.x + param.y * pt.y + param.z * pt.z);
	
	return Plane3D (param);
}
vec3 Intersect(Ray ray, Plane3D plane)
{
	float t = -(plane.Eqn.x * (ray.Orig.x) + plane.Eqn.y * (ray.Orig.y) + plane.Eqn.z * (ray.Orig.z) + plane.Eqn.w) 
			/ (plane.Eqn.x * (ray.Dirn.x) + plane.Eqn.y * (ray.Dirn.y) + plane.Eqn.z * (ray.Dirn.z));

	float x = ray.Orig.x + ray.Dirn.x * t;
	float y = ray.Orig.y + ray.Dirn.y * t;
	float z = ray.Orig.z + ray.Dirn.z * t;

	return vec3( x, y, z );
}

float t_RayXPlane (Ray ray, Plane3D plane)
{
	float t = -(plane.Eqn.x * (ray.Orig.x) + plane.Eqn.y * (ray.Orig.y) + plane.Eqn.z * (ray.Orig.z) + plane.Eqn.w) 
			/ (plane.Eqn.x * (ray.Dirn.x) + plane.Eqn.y * (ray.Dirn.y) + plane.Eqn.z * (ray.Dirn.z));
	return t;
}
float RayXPlane (Ray ray, Plane3D plane)
{
	float t = t_RayXPlane (ray, plane);
	return (t > 0 ? length(ray.Dirn*t) : -1.0);
}
float t_RayXSphere (Ray ray, Sphere sphere)
{
	vec3 ray_to_sphere = ray.Orig - sphere.Centre;
	float half_b = dot(ray.Dirn, ray_to_sphere);
	float a = dot(ray.Dirn, ray.Dirn);
	float c = dot(ray_to_sphere, ray_to_sphere) - sphere.Radius*sphere.Radius;
	
	float determinant = half_b*half_b - a*c;

	return (determinant > 0 && half_b < 0 ? ((-half_b - sqrt(determinant))/a) : -1.0);
}
float DepthRayXSphere (Ray ray, Sphere sphere)
{
	float t = t_RayXSphere (ray, sphere);
	return (t > 0 ? length(ray.Dirn*t) : -1.0);
}
vec3 RayColor (Ray ray)
{
	float t = (ray.Dirn.y + 1.0)*0.5;
	return ((1.0 - t)*vec3 (1.0, 1.0, 1.0) + t*vec3 (0.3, 0.4, 1.0));
}

vec4 out_Pixel (ivec2 pixel_coords)
{
	ivec2 img_size = imageSize (img_output);
	float aspectRatio = float (img_size.x)/img_size.y;
	vec3 world_up = vec3 (0, 1, 0);
	vec3 cam_right = cross (u_CameraDirn, world_up);
	vec3 cam_up = cross (cam_right, u_CameraDirn);
	float scr_x = (pixel_coords.x*2.0 - img_size.x)/(2.0*img_size.x);
	scr_x *= aspectRatio;
	float scr_y = (pixel_coords.y*2.0 - img_size.y)/(2.0*img_size.y);

	vec3 point_in_scr_space = u_CameraPosn + u_CameraDirn*u_FocusDist + cam_right*scr_x + cam_up*scr_y;
	
// Ray and other stuff
	Plane3D infinite_floor = CreatePlane (vec3 (0, -2, 0), vec3 (0, -2, 1), vec3 (1, -2, 0));
	Ray cam_ray = CreateRay (u_CameraPosn, point_in_scr_space);
	
// Color
	vec3 color = RayColor (cam_ray); // background
	float min_depth = 10000; // FLT_MAX
	
//Depth Testing with t instead of proper depth
	{
		float t_depth = t_RayXPlane (cam_ray, infinite_floor); // with plane
		if(min_depth > t_depth && t_depth > 0){
			color = vec3 (0.8, 0.1, 0.7f); // plane color
			min_depth = t_depth;
		}
	}{
		float t_depth = t_RayXSphere (cam_ray, u_Sphere); // with sphere
		if(min_depth > t_depth && t_depth > 0){
			vec3 intersection_pt = cam_ray.Orig + cam_ray.Dirn*t_depth;
			color = u_ShowNormal ? normalize(intersection_pt - u_Sphere.Centre) : vec3 (1, 0, 0);
			min_depth = t_depth;
		}
	}
	return vec4 (color, 1.0);
}
void main ()
{
	// get index in global work group i.e x,y position
	ivec2 pixel_coords = ivec2 (gl_GlobalInvocationID.xy);

	// base pixel color for image
	vec4 pixel = out_Pixel (pixel_coords);

	// output to a specific pixel in the image
	imageStore (img_output, pixel_coords, pixel);
}