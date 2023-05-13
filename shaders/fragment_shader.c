#version 460 core
layout(location = 0) out vec4 FragColor;

uniform float u_time;
uniform vec2 u_resolution;
uniform vec2 u_mouse;

#define MAX_MARCHING_STEPS 100.
#define PRECISION 0.01
#define MAX_DEPTH 50.

struct Ray {
  vec3 ro;
  vec3 rd;
};

float sphere_sdf(vec3 point, vec3 offset, float radius) {

  return length(point - offset) - radius;
}

float floor_sdf(vec3 point) { return point.y + 1.; }

float draw_scene(vec3 point) {

  // Adding multiple objects to a scene
  float dist = sphere_sdf(point, vec3(.2, 0., .4), .1);
  dist = min(dist, sphere_sdf(point, vec3(-.2, 0., -.4), .1));
  dist = min(dist, sphere_sdf(point, vec3(-.4, 0., 0.), .1));
  dist = min(dist, floor_sdf(point));
  // The min function gets the nearest object in the scene for the given point p

  return dist;
}

vec3 calc_normal(vec3 p) {

  vec2 e = vec2(1.0, -1.0) * 0.005;
  return normalize(
      e.xyy * draw_scene(p + e.xyy) + e.yyx * draw_scene(p + e.yyx) +
      e.yxy * draw_scene(p + e.yxy) + e.xxx * draw_scene(p + e.xxx));
}

float ray_march(Ray ray) {

  // Depth(distance) already traveled by the ray
  float depth;
  // The new location for the origin of the ray at the calculated depth
  vec3 point;
  // Get scene distance field
  float distance_field;

  float i = 0.;
  while (i < MAX_MARCHING_STEPS) {

    // Calculate new origin location based on marched depth
    // Multiply depth with normalized vector rd to get get direction traveled
    point = ray.ro + depth * ray.rd;

    // Get the distance field for the current depth
    // return the nearest intersection with a scene object
    distance_field = draw_scene(point);

    // Add the minimum intersection with the scene to the dept to calculate the
    // next point location next iteration
    depth += distance_field;

    // Check how close has the ray marched to the shape distance field
    // If the distance to the shape is smaller than the given precision then
    // stop the loop
    if (distance_field > MAX_DEPTH || distance_field < PRECISION)
      break;

    i += 1.;
  }
  // Return how far has the ray traveled
  // To get the exact point the ro + depth * rd must be calcualted
  return depth;
}

void main() {

  vec2 uv =
      gl_FragCoord.xy / u_resolution.xy; // Normalize screen pixel coord 0 - 1
  uv -= .5;                              // Move center of canvas to -0.5 - 0.5
  uv.x *= u_resolution.x / u_resolution.y; // Fix aspect ratio

  // Initial canvas color
  vec3 color = vec3(0.3);

  // Equivalent with above 3 lines
  // vec2 uv=(gl_FragCoord.xy-.5*u_resolution.xy)/u_resolution.y;

  // Ray origin, 1 units behind the canvas where the camera is
  // This is the camera position
  vec3 ro = vec3(0, .3, 2);

  // Compute the ray direction for every pixel on screen (uv)
  // -1 so each ray is fired toward the canvas from the ray origin
  vec3 rd = normalize(vec3(uv, -1));

  Ray ray = Ray(ro, rd);

  // Returns the distance from the ray marching algorithm
  float distance = ray_march(ray);

  if (distance < MAX_DEPTH) {

    // Final calculated point
    vec3 point = ray.ro + distance * ray.rd;

    // Final point normal
    vec3 point_normal = calc_normal(point);

    // Light location
    vec3 light_position = vec3(0., 3., 5.);
    light_position.xz += vec2(sin(u_time * 2.), cos(u_time * 2.));

    // Calculate the amount the light is hiting the surface of the objects
    vec3 light_vector = normalize(light_position - point);

    // Calculate diffuse lighting by taking the dot product of
    // the light direction and the normal and limit the range to 0 - 1
    float difuse_lighting = clamp(dot(light_vector, point_normal), 0., 1.);
    // difuse_lighting *= 5. / dot(light_position - point, light_position -
    // point);

    // Objects shadow
    // To get the objects shadow we march from the point given by the
    // ray_marching function to the ligth source if the ray_marching function
    // distance is smaller than the distance from the point to the light then we
    // hit something and we can decrease the light level on that point to
    // simulate a shadow
    // The origin of the new ray is the point position plus a small number so
    // that the ray_marching algorithm won't intersect with the surface we are
    // on (the current point is on the surface)
    // The direction of the ray is given by the normalized value from point to
    // the light
    float d = ray_march(Ray(point + .1, light_vector));
    if (d < length(light_position - point))
      difuse_lighting *= .1;

    color = vec3(difuse_lighting);
  }

  FragColor = vec4(color, 1.);
}
