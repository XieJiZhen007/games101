//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include <thread>
#include "Scene.hpp"
#include "Renderer.hpp"


inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

const float EPSILON = 0.00001;

static void RenderThread(const Scene& scene, std::vector<Vector3f> &framebuffer, int yStart, int yEnd, int spp)
{
    float scale = tan(deg2rad(scene.fov * 0.5));
    float imageAspectRatio = scene.width / (float)scene.height;
    Vector3f eye_pos(278, 273, -800);

    for (int j = yStart; j < yEnd; ++j) {
        for (int i = 0; i < scene.width; ++i) {
            float x = (2 * (i + 0.5) / (float)scene.width - 1) *
                      imageAspectRatio * scale;
            float y = (1 - 2 * (j + 0.5) / (float)scene.height) * scale;

            Vector3f dir = normalize(Vector3f(-x, y, 1));
            for (int k = 0; k < spp; k++){
                framebuffer[j * scene.height + i] += scene.castRay(Ray(eye_pos, dir), 0) / spp;  
            }
        }
    }
}

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render(const Scene& scene)
{
    std::vector<Vector3f> framebuffer(scene.width * scene.height);

    // old code
    // float scale = tan(deg2rad(scene.fov * 0.5));
    // float imageAspectRatio = scene.width / (float)scene.height;
    // Vector3f eye_pos(278, 273, -800);
    // int m = 0;

    // change the spp value to change sample ammount
    int spp = 16;
    std::cout << "SPP: " << spp << "\n";

    // old code
    // for (uint32_t j = 0; j < scene.height; ++j) {
    //     for (uint32_t i = 0; i < scene.width; ++i) {
    //         // generate primary ray direction
    //         float x = (2 * (i + 0.5) / (float)scene.width - 1) *
    //                   imageAspectRatio * scale;
    //         float y = (1 - 2 * (j + 0.5) / (float)scene.height) * scale;

    //         Vector3f dir = normalize(Vector3f(-x, y, 1));
    //         for (int k = 0; k < spp; k++){
    //             framebuffer[m] += scene.castRay(Ray(eye_pos, dir), 0) / spp;  
    //         }
    //         m++;
    //     }
    //     UpdateProgress(j / (float)scene.height);
    // }

    // TODO: try to use thread
    
    int maxThread = std::thread::hardware_concurrency();
    std::cout << "maxThread: " << maxThread << std::endl;
    int linePThread = scene.height / maxThread + (scene.height % maxThread > 0);

    std::vector<std::thread> threads;
    for (size_t i = 0; i < maxThread; i++) {
        int yStart = i * linePThread;
        int yEnd = std::min(yStart + linePThread, scene.height);

        // std::ref ==> ?
        threads.push_back(std::thread(RenderThread, std::ref(scene), std::ref(framebuffer), yStart, yEnd, spp));
    }

    for (auto &thread : threads) {
        thread.join();
    }

    UpdateProgress(1.f);

    // save framebuffer to file
    FILE* fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);    
}
