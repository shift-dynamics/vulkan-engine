#ifndef SHIFT_GUI_MESHDATA_H_
#define SHIFT_GUI_MESHDATA_H_

#include <vector>
#include <string>

namespace vulkan_engine {
  
  enum class ShadingType { WIREFRAME, FLAT, PHONG, LINE };

  struct MeshData {
    ShadingType shading_type = ShadingType::PHONG;
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> colors;
    std::vector<float> tangents;
    std::vector<float> bitangents;
    std::vector<float> texture_coordinates;
    std::vector<unsigned int> faces;
    std::string diffuse_map;
    std::string normal_map;
    std::string specular_map;
    std::string displacement_map;
    std::string metalness_map;
    std::string occlusion_map;
    int material_index = -1;
 };

  struct MaterialData {
    float Ka[3] = {0.2, 0.2, 0.2};
    float Kd[3] = {0.6, 0.6, 0.6};
    float Ks[3] = {0.1, 0.1, 0.1};
    float Ke[4] = {0.0, 0.0, 0.0, 0.0};
    float Ns = 32.0;
    float opacity = 1.0;
    float metalness = 0.0;
  };

}

#endif
