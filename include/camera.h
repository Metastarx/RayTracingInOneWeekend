#ifndef CAMERA_H
#define CAMERA_H

#include "hittable.h"
#include "material.h"


class camera {
  public:
    double aspect_ratio = 1.0;  // Ratio of image width over height
    int    image_width  = 100;  // Rendered image width in pixel count
    int    samples_per_pixel = 10;
    int    max_depth    =10; 

    double vfov = 90;  // Vertical view angle (field of view)
    point3 lookfrom = point3(0,0,0);
    point3 lookat = point3(0,0,0);
    vec3   vup      = vec3(0,1,0);

    double defocus_angle = 0;  // 每个像素射线的变化角度
    double focus_dist = 10;    // 从相机观察点到完美聚焦平面的距离


    void render(const hittable& world) {
        initialize();

        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

        for (int j = 0; j < image_height; j++) {
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
            for (int i = 0; i < image_width; i++) {
                color pixel_color(0,0,0);
                for(int sample = 0;sample < samples_per_pixel;sample++)
                {
                    ray r = get_ray(i,j);
                    pixel_color += ray_color(r,max_depth,world);
                }

                write_color(std::cout, pixel_samples_scale * pixel_color);
            }
        }

        std::clog << "\rDone.                 \n";
    }

  private:
    int    image_height;   // Rendered image height
    double pixel_samples_scale; // Color scale factor for a sum of pixel samples
    point3 center;         // Camera center
    point3 pixel00_loc;    // Location of pixel 0, 0
    vec3   pixel_delta_u;  // Offset to pixel to the right
    vec3   pixel_delta_v;  // Offset to pixel below
    vec3   u, v, w;

    vec3   defocus_disk_u;       // 水平半径
    vec3   defocus_disk_v;       // 垂直半径

    //初始化相机
    void initialize() {
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        pixel_samples_scale = 1.0 / samples_per_pixel;

        center = lookfrom;

        // Determine viewport dimensions.

        //auto focal_length = (lookfrom - lookat).length();
        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta/2);
        auto viewport_height = 2 * h * focus_dist;
        //auto viewport_height = 2 * h * focal_length;
        auto viewport_width = viewport_height * (double(image_width)/image_height);

        // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
        w = unit_vector(lookfrom - lookat);//向前的向量
        u = unit_vector(cross(vup, w));// 指向相机右侧。
        v = cross(w, u);//指向相机上方。


        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        // auto viewport_u = vec3(viewport_width, 0, 0);
        // auto viewport_v = vec3(0, -viewport_height, 0);
        vec3 viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
        vec3 viewport_v = viewport_height * -v;  // Vector down viewport vertical edge


        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        //auto viewport_upper_left = center - (focal_length*w) - viewport_u/2 - viewport_v/2;
        auto viewport_upper_left = center - (focus_dist * w) - viewport_u/2 - viewport_v/2;

        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

        // Calculate the camera defocus disk basis vectors.
        auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }

    //获取在视口i j处的射线
    ray get_ray(int i,int j) const {
        // Construct a camera ray originating from the defocus disk and directed at a randomly
        // sampled point around the pixel location i, j.

        auto offset = sample_square();
        auto pixel_sample = pixel00_loc
                          + ((i + offset.x()) * pixel_delta_u)
                          + ((j + offset.y()) * pixel_delta_v);

        //auto ray_origin = center;
        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction);
    }

    vec3 sample_square() const {
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

    point3 defocus_disk_sample() const {
        // Returns a random point in the camera defocus disk.
        auto p = random_in_unit_disk();
        return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }

    

    color ray_color(const ray& r, int depth,const hittable& world) {
        if(depth <= 0)
        {
            return color(0,0,0);
        }

        hit_record rec;

        if (world.hit(r, interval(0.001,infinity), rec)) {
            //知道碰撞点的信息 保存至rec中 根据材质的不同 定义不同的反射信息 比如是漫反射或者是全反射
            ray scattered;
            color attenuation;
            if (rec.mat->scatter(r, rec, attenuation, scattered))
                return attenuation * ray_color(scattered, depth-1, world);

            return color(0,0,0);
        }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }
};

#endif