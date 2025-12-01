#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <vector>
#include <iostream>
#include <cassert>
#include <cstring>
#include <sys/time.h>

#if defined __linux__ || defined __APPLE__
#else
#define M_PI 3.141592653589793
#define INFINITY 1e8
#endif

template<typename T>
class Vec3
{
public:
    T x, y, z;
    Vec3() : x(T(0)), y(T(0)), z(T(0)) {}
    Vec3(T xx) : x(xx), y(xx), z(xx) {}
    Vec3(T xx, T yy, T zz) : x(xx), y(yy), z(zz) {}
    Vec3& normalize()
    {
        T nor2 = length2();
        if (nor2 > 0) {
            T invNor = 1 / sqrt(nor2);
            x *= invNor, y *= invNor, z *= invNor;
        }
        return *this;
    }
    Vec3<T> operator * (const T &f) const { return Vec3<T>(x * f, y * f, z * f); }
    Vec3<T> operator * (const Vec3<T> &v) const { return Vec3<T>(x * v.x, y * v.y, z * v.z); }
    T dot(const Vec3<T> &v) const { return x * v.x + y * v.y + z * v.z; }
    Vec3<T> operator - (const Vec3<T> &v) const { return Vec3<T>(x - v.x, y - v.y, z - v.z); }
    Vec3<T> operator + (const Vec3<T> &v) const { return Vec3<T>(x + v.x, y + v.y, z + v.z); }
    Vec3<T>& operator += (const Vec3<T> &v) { x += v.x, y += v.y, z += v.z; return *this; }
    Vec3<T>& operator *= (const Vec3<T> &v) { x *= v.x, y *= v.y, z *= v.z; return *this; }
    Vec3<T> operator - () const { return Vec3<T>(-x, -y, -z); }
    T length2() const { return x * x + y * y + z * z; }
    T length() const { return sqrt(length2()); }
    friend std::ostream & operator << (std::ostream &os, const Vec3<T> &v)
    {
        os << "[" << v.x << " " << v.y << " " << v.z << "]";
        return os;
    }
};

typedef Vec3<float> Vec3f;
typedef Vec3<float> RGB;

class Sphere
{
public:
    Vec3f center;                           /// position of the sphere
    float radius, radius2;                  /// sphere radius and radius^2
    Vec3f surfaceColor, emissionColor;      /// surface color and emission (light)
    float transparency, reflection;         /// surface transparency and reflectivity
    Sphere(
        const Vec3f &c,
        const float &r,
        const Vec3f &sc,
        const float &refl = 0,
        const float &transp = 0,
        const Vec3f &ec = 0) :
        center(c), radius(r), radius2(r * r), surfaceColor(sc), emissionColor(ec),
        transparency(transp), reflection(refl)
    { }
    bool intersect(const Vec3f &rayorig, const Vec3f &raydir, float &t0, float &t1) const
    {
        Vec3f l = center - rayorig;
        float tca = l.dot(raydir);
        if (tca < 0) return false;
        float d2 = l.dot(l) - tca * tca;
        if (d2 > radius2) return false;
        float thc = sqrt(radius2 - d2);
        t0 = tca - thc;
        t1 = tca + thc;
        
        return true;
    }
};

#define MAX_RAY_DEPTH 5

float mix(const float &a, const float &b, const float &mix)
{
    return b * mix + a * (1 - mix);
}

__device__
Vec3f trace(
    const Vec3f &rayorig,
    const Vec3f &raydir,
    const Sphere *spheres,
    const int &depth)
{
    float tnear = INFINITY;
    const Sphere* sphere = NULL;
    // find intersection of this ray with the sphere in the scene
    for (unsigned i = 0; i < spheres.size(); ++i) {
        float t0 = INFINITY, t1 = INFINITY;
        if (spheres[i].intersect(rayorig, raydir, t0, t1)) {
            if (t0 < 0) t0 = t1;
            if (t0 < tnear) {
                tnear = t0;
                sphere = &spheres[i];
            }
        }
    }
    // if there's no intersection return black or background color
    if (!sphere) return Vec3f(2);
    Vec3f surfaceColor = 0; // color of the ray/surfaceof the object intersected by the ray
    Vec3f phit = rayorig + raydir * tnear; // point of intersection
    Vec3f nhit = phit - sphere->center; // normal at the intersection point
    nhit.normalize(); // normalize normal direction
    
    float bias = 1e-4; // add some bias to the point from which we will be tracing
    bool inside = false;
    if (raydir.dot(nhit) > 0) nhit = -nhit, inside = true;
    if ((sphere->transparency > 0 || sphere->reflection > 0) && depth < MAX_RAY_DEPTH) {
        float facingratio = -raydir.dot(nhit);
        float fresneleffect = mix(pow(1 - facingratio, 3), 1, 0.1);

        Vec3f refldir = raydir - nhit * 2 * raydir.dot(nhit);
        refldir.normalize();

        Vec3f reflection = trace(phit + nhit * bias, refldir, spheres, depth + 1);
        Vec3f refraction = 0;

        // if the sphere is also transparent compute refraction ray (transmission)
        if (sphere->transparency) {
            float ior = 1.1, eta = (inside) ? ior : 1 / ior; // are we inside or outside the surface?
            float cosi = -nhit.dot(raydir);
            float k = 1 - eta * eta * (1 - cosi * cosi);
            Vec3f refrdir = raydir * eta + nhit * (eta *  cosi - sqrt(k));
            refrdir.normalize();
            refraction = trace(phit - nhit * bias, refrdir, spheres, depth + 1);
        }
        surfaceColor = (
            reflection * fresneleffect +
            refraction * (1 - fresneleffect) * sphere->transparency) * sphere->surfaceColor;
    }
    else {
        for (unsigned i = 0; i < spheres.size(); ++i) {
            if (spheres[i].emissionColor.x > 0) {
                Vec3f transmission = 1;
                Vec3f lightDirection = spheres[i].center - phit;
                lightDirection.normalize();
                for (unsigned j = 0; j < spheres.size(); ++j) {
                    if (i != j) {
                        float t0, t1;
                        if (spheres[j].intersect(phit + nhit * bias, lightDirection, t0, t1)) {
                            transmission = 0;
                            break;
                        }
                    }
                }
                surfaceColor += sphere->surfaceColor * transmission *
                std::max(float(0), nhit.dot(lightDirection)) * spheres[i].emissionColor;
            }
        }
    }
    
    return surfaceColor + sphere->emissionColor;
}

__global__
void render(Vec3f* image, unsigned width, unsigned height, const Sphere *spheres)
{
    Vec3f *pixel = image;
    float invWidth = 1 / float(width), invHeight = 1 / float(height);
    float fov = 30, aspectratio = width / float(height);
    float angle = tan(M_PI * 0.5 * fov / 180.);

    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x, ++pixel) {
            float xx = (2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio;
            float yy = (1 - 2 * ((y + 0.5) * invHeight)) * angle;
            Vec3f raydir(xx, yy, -1);
            raydir.normalize();
            *pixel = trace(Vec3f(0), raydir, spheres, 0);
        }
    }
}

void save(Vec3f* image, unsigned width, unsigned height) {
    std::ofstream ofs("image.ppm", std::ios::out | std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (unsigned i = 0; i < width * height; ++i) {
        ofs << (unsigned char)(std::min(float(1), image[i].x) * 255) <<
               (unsigned char)(std::min(float(1), image[i].y) * 255) <<
               (unsigned char)(std::min(float(1), image[i].z) * 255);
    }
    ofs.close();
}

int main(int argc, char **argv)
{
    unsigned width, height;
    unsigned s, l;
    scanf("%u %u\n", &width, &height);
    scanf("%u %u\n", &s, &l);
    Sphere* spheres;
    cudaMallocManaged<Sphere>(&spheres, (s + l) * sizeof(Sphere));
    // spheres
    for(int i=0; i<s; i++) {
      float x, y, z, r, refl, transp;
      scanf("%f %f %f %f ", &x, &y, &z, &r);
      Vec3f position(x, y, z);
      scanf("%f %f %f %f %f\n", &x, &y, &z, &refl, &transp);
      RGB color(x, y, z);
      // position, radius, surface color, reflectivity, transparency, emission color
      spheres[i] = Sphere(position, r, color, refl, transp);
    }
    // lights
    for(int i=0; i<l; i++) {
      float x, y, z, r, refl, transp, ec;
      scanf("%f %f %f %f ", &x, &y, &z, &r);
      Vec3f position(x, y, z);
      scanf("%f %f %f %f %f %f\n", &x, &y, &z, &refl, &transp, &ec);
      RGB color(x, y, z);
      // position, radius, surface color, reflectivity, transparency, emission color
      spheres[i+s] = Sphere(position, r, color, refl, transp, ec);
    }
    Vec3f *image;
    cudaMallocManaged<Vec3f>(&image, width * height * sizeof(Vec3f));

    render<<<1,1>>>(image, width, height, spheres);

    save(image, width, height);

    delete image;

    return 0;
}
