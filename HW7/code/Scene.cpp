//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    // TODO Implement Path Tracing Algorithm here
    // 获得光线交点信息
    Intersection p = intersect(ray);
    if (p.happened)
        // return shade(p, ray.direction);
        return shade(p, ray.direction);
    return this->backgroundColor;
}

Vector3f Scene::shade(const Intersection &p, const Vector3f& wo) const
{
    const float& eps = 5e-4;
    
    Vector3f L_dir = {0.0f};
    Vector3f L_indir = {0.0f};

    // 直接光照
    Intersection x;
    float pdf_light;
    sampleLight(x, pdf_light);

    Vector3f ws = (x.coords - p.coords).normalized();    
    if ((intersect(Ray(p.coords, ws)).coords - x.coords).norm() < eps) 
    {
        auto emit = x.emit;
        // 为什么入射光是 ws，而不是 -ws，同理出射光为什么不是 wo
        auto f_r = p.m->eval(ws, -wo, p.normal);
        auto cosn = std::max(0.0f, dotProduct(p.normal, ws));
        auto cosnn = std::max(0.0f, dotProduct(x.normal, -ws));
        auto dist = std::pow((x.coords - p.coords).norm(), 2);

        L_dir = emit * f_r * cosn * cosnn / dist / pdf_light;
    }

    return L_dir;
}

// Vector3f Scene::shade(const Intersection &isec, const Ray &ray) const {
//     static const double eps = 5e-4;

//     Material *m = isec.m;
//     Vector3f normal = isec.normal;

//     // isec is on the light
//     if (m->hasEmission())
//         return m->getEmission();

//     auto L_dir = Vector3f(0.0, 0.0, 0.0);
//     auto L_indir = Vector3f(0.0, 0.0, 0.0);

//     // direct light
//     Intersection pos;
//     float pdf;
//     sampleLight(pos, pdf);

//     auto ws = (pos.coords - isec.coords).normalized();
//     bool notBlocked = (intersect(Ray(isec.coords, ws)).coords - pos.coords).norm() < eps;
//     bool hasDirectLight = false;

//     if (notBlocked) {
//         auto dist = pow((pos.coords - isec.coords).norm(), 2);
//         double cos1 = std::max(0.0f, dotProduct(ws, normal));
//         double cos2 = std::max(0.0f, dotProduct(-ws, pos.normal));
//         L_dir = pos.emit * m->eval(ws, -ray.direction, normal) * cos1 * cos2 / pdf / dist;
        
//         // due to sometimes we won't get the correct ray to reflect light in the specular surface.
//         // thus, we let the ray to choose it's own directon.
//         hasDirectLight = L_dir.norm() > 0.1; 
//     }

//     // indirect light
//     if (get_random_float() <= RussianRoulette) {
//         auto wi = m->sample(ray.direction, normal);

//         Ray ray2(isec.coords, wi);
//         Intersection pos2 = intersect(ray2);

//         if (pos2.happened && (!hasDirectLight || !pos2.m->hasEmission())) {
//             double cos1 = std::max(0.0f, dotProduct(wi, normal));
//             L_indir = shade(pos2, ray2) * m->eval(wi, -ray.direction, normal) * cos1 / std::max(m->pdf(wi, -ray.direction, normal), 0.0001f) / RussianRoulette;
//         }
//     }

//     return Vector3f::Max(Vector3f::Min(L_dir + L_indir, Vector3f(1.0f)), Vector3f(0.0f));
//     // return L_dir + L_indir;
// }