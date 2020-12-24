#version 450 core
/* pbr.glsl */

layout(constant_id = 0) const int max_transforms_ = 1024;
layout(constant_id = 1) const int max_lights_ = 16;
layout(constant_id = 2) const int max_materials_ = 1024;
layout(constant_id = 3) const int shadow_texture_size_ = 2048;
layout(constant_id = 4) const int max_textures_ = 2048;

layout(push_constant) uniform PushConstants {
  int index;
}
push_constants_;

layout(set = 0, binding = 0) uniform Camera {
  mat4 v;
  mat4 p;
}
camera_;

struct Light {
  vec3 position;
  vec3 color;
  int cast_shadows;
  int directional;
  mat4 shadow_matrix;
};

layout(set = 0, binding = 1) uniform Lights {
  int count;
  int draw_shadows;
  Light light[max_lights_];
}
lights_;

layout(set = 1, binding = 0) uniform sampler2DArrayShadow texture_2d_shadow_;
layout(set = 1,
       binding = 1) uniform samplerCubeArrayShadow texture_2d_shadow_cube_;

struct Transform {
  mat4 m;
  mat3 nm;
  int material_index;
  int cast_shadows;
  int receive_shadows;
};

layout(set = 2, binding = 0) uniform Transforms {
  Transform transform[max_transforms_];
}
transforms_;

struct Material {
  vec3 albedo;
  float occlusion;
  float roughness;
  float metalness;
  float alpha;
  int albedo_texture_index;
  int occlusion_roughness_metalness_texture_index;
  int normal_texture_index;
};

layout(set = 2, binding = 1) uniform Materials {
  Material material[max_materials_];
}
materials_;

layout(set = 3, binding = 0) uniform sampler2D texture_normal_[max_textures_];
layout(set = 3, binding = 1) uniform sampler2D texture_albedo_[max_textures_];
layout(set = 3, binding = 2) uniform sampler2D
  texture_occlusion_roughness_metallic_[max_textures_];

#ifdef VERTEX_SHADER
layout(location = 0) in vec4 position_;
layout(location = 1) in vec3 normal_;
layout(location = 2) in vec2 uv_;
layout(location = 3) in vec3 bitangent_;

layout(location = 0) out vec3 normal_frag_;
layout(location = 1) out vec4 position_frag_;
layout(location = 2) out vec2 uv_frag_;
layout(location = 3) out vec4 eye_direction_camera_space_;
layout(location = 4) out vec3 world_position_;
layout(location = 5) out vec4 shadow_coord_[max_lights_];

void main() {
  int index_ = push_constants_.index;
  normal_frag_ = normalize(transforms_.transform[index_].nm * normal_);
  position_frag_ = position_;
  gl_Position =
    camera_.p * camera_.v * transforms_.transform[index_].m * position_;
  if(lights_.draw_shadows == 1) {
    for(int i = 0; i < lights_.count; i++) {
      shadow_coord_[i] = lights_.light[i].shadow_matrix * position_;
    }
    eye_direction_camera_space_ =
      -camera_.v * transforms_.transform[index_].m * position_;
  }
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) in vec3 normal_frag_;
layout(location = 1) in vec4 position_frag_;
layout(location = 2) in vec2 uv_frag_;
layout(location = 3) in vec4 eye_direction_camera_space_;
layout(location = 4) in vec3 world_position_;
layout(location = 5) in vec4 shadow_coord_[max_lights_];

layout(location = 0) out vec4 color_frag_;

const float PI = 3.14159265359;

// Poisson disk used for poisson sampling
// vec2 poissonDisk[16] = vec2[](
//    vec2(-0.94201624, -0.39906216),
//    vec2(0.94558609, -0.76890725),
//    vec2(-0.094184101, -0.92938870),
//    vec2(0.34495938, 0.29387760),
//    vec2(-0.91588581, 0.45771432),
//    vec2(-0.81544232, -0.87912464),
//    vec2(-0.38277543, 0.27676845),
//    vec2(0.97484398, 0.75648379),
//    vec2(0.44323325, -0.97511554),
//    vec2(0.53742981, -0.47373420),
//    vec2(-0.26496911, -0.41893023),
//    vec2(0.79197514, 0.19090188),
//    vec2(-0.24188840, 0.99706507),
//    vec2(-0.81409955, 0.91437590),
//    vec2(0.19984126, 0.78641367),
//    vec2(0.14383161, -0.1410079 )
// );

float distribution_ggx(vec3 N, vec3 H, float a) {
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;

  float nom = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;

  return nom / denom;
}

float geometry_schlick_ggx(float NdotV, float k) {
  return NdotV / (NdotV * (1.0 - k) + k);
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float k) {
  float NdotV = max(dot(N, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);
  float ggx1 = geometry_schlick_ggx(NdotV, k);
  float ggx2 = geometry_schlick_ggx(NdotL, k);
  return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float offset_lookup(sampler2DArrayShadow texture_shadow, int index,
                    vec4 location, vec2 offset) {
  return texture(texture_shadow, vec4(location.x / location.w + offset.x,
                                      location.y / location.w + offset.y,
                                      float(index), location.z / location.w));
}

void main() {
  int index_ = push_constants_.index;
  vec3 V = vec3(0.0, 0.0, 1.0);
  mat4 mv = camera_.v * transforms_.transform[index_].m;

  int material_index = transforms_.transform[index_].material_index;

  vec3 albedo;
  int albedo_index = materials_.material[material_index].albedo_texture_index;
  if(albedo_index < 0) {
    albedo = materials_.material[material_index].albedo;
  } else {
    albedo = texture(texture_albedo_[albedo_index], uv_frag_).rgb;
  }

  float occlusion;
  float roughness;
  float metalness;
  int orm_index = materials_.material[material_index]
                    .occlusion_roughness_metalness_texture_index;

  if(orm_index < 0) {
    occlusion = materials_.material[material_index].occlusion;
    roughness = materials_.material[material_index].roughness;
    metalness = materials_.material[material_index].metalness;
  } else {
    occlusion =
      texture(texture_occlusion_roughness_metallic_[orm_index], uv_frag_).x;
    roughness =
      texture(texture_occlusion_roughness_metallic_[orm_index], uv_frag_).y;
    metalness =
      texture(texture_occlusion_roughness_metallic_[orm_index], uv_frag_).z;
  }

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metalness);

  vec3 Lo = vec3(0.0);

  for(int i = 0; i < lights_.count; i++) {
    vec3 L;
    switch(lights_.light[i].directional) {
      case 0: /* point light, currently being treated same as spot */
      case 1: {
        /* 1 = spot light
          position indicates position of light source
          used to compute direction of incoming light */
        L = (camera_.v * vec4(lights_.light[i].position - world_position_, 1.0))
              .xyz;
        break;
      }
      case 2: {
        /* 2 = directional light
          position indicates direction of light source
          used to compute direction of incoming light */
        L = (camera_.v * vec4(lights_.light[i].position.xyz, 1.0)).xyz;
        break;
      }
      default:
        break;
    }

    float shadow_test = 1.f;
    if(lights_.draw_shadows == 1 &&
       transforms_.transform[index_].receive_shadows == 1) {
      shadow_test = 0.0;
      for(float y = -1.5; y <= 1.5; y += 1.0) {
        for(float x = -1.5; x <= 1.5; x += 1.0) {
          shadow_test +=
            offset_lookup(texture_2d_shadow_, i, shadow_coord_[i],
                          vec2(x, y) / float(shadow_texture_size_));
        }
      }
      shadow_test /= 16.0;

      // shadowTest = texture(
      //   texture_shadow_u,
      //   vec4(
      //     shadowCoord[i].x / shadowCoord[i].w,
      //     shadowCoord[i].y / shadowCoord[i].w,
      //     float(i),
      //     shadowCoord[i].z / shadowCoord[i].w)
      //     );
      //
      // float cosTheta = clamp(LdotN, 0.0, 1.0);
      // float bias = 0.005 * tan(acos(cosTheta));
      // bias = clamp(bias, 0.0, 0.01);
      //
      // shadowTest = 0;
      // for(int j = 0; j < 4; j++) {
      //   shadowTest -=
      //     0.2 * (1.0 - texture(
      //       texture_shadow_u,
      //       vec4(
      //         (shadowCoord[i].x + poissonDisk[j].x / 700.0) /
      //         shadowCoord[i].w, (shadowCoord[i].y + poissonDisk[j].y / 700.0)
      //         / shadowCoord[i].w, float(i), (shadowCoord[i].z) /
      //         shadowCoord[i].w)));
      // }
    }

    // begin physical based rendering

    // @TODO detect if material has a normal map and do bump/normal mapping
    vec3 N = normal_frag_;

    L = normalize(L);
    vec3 H = normalize(V + L);
    float dist = length(lights_.light[i].position - world_position_);
    float attenuation = 1.0 / (dist * dist);
    vec3 radiance = lights_.light[i].color * attenuation;

    // cook-torrance brdf
    float NDF = distribution_ggx(N, H, roughness);
    float G = geometry_smith(N, V, L, roughness);
    vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metalness;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = numerator / max(denominator, 0.001);

    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
  }

  vec3 ambient = vec3(0.03) * albedo * occlusion;
  vec3 color = ambient + Lo;

  // tone mapping using extended Reinhard operator
  // @TODO use Reinhard operator in luminance space
  // @TODO add whiteness threshold / exposure setting
  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0 / 2.2));

  color_frag_ = vec4(color, materials_.material[material_index].alpha);
}
#endif
