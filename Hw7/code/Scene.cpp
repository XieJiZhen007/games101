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
    
    if (p.m->hasEmission())
        return p.m->getEmission();

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
        auto f_r = p.m->eval(ws, -wo, p.normal);
        auto cosn = std::max(0.0f, dotProduct(p.normal, ws));
        auto cosnn = std::max(0.0f, dotProduct(x.normal, -ws));
        auto dist = std::pow((x.coords - p.coords).norm(), 2);

        L_dir = emit * f_r * cosn * cosnn / dist / pdf_light;
    }

    // 间接光照
    if (get_random_float() < RussianRoulette)
    {
        Vector3f wi = p.m->sample(wo, p.normal);

        Intersection q = intersect(Ray(p.coords, wi));
        if (q.happened && q.m->hasEmission() == false)
        {
            auto cosn = std::max(0.0f, dotProduct(p.normal, wi));
            auto f_r = p.m->eval(wi, -wo, p.normal);
            auto pdf = p.m->pdf(wi, -wo, p.normal);
            L_indir = shade(q, wi.normalized()) * f_r * cosn / pdf / RussianRoulette;
        }
    }

    return L_dir + L_indir;
}
