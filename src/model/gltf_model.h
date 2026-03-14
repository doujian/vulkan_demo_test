#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <memory>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

namespace vk_model {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 color;
};

struct Primitive {
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t firstVertex;
    uint32_t vertexCount;
    glm::vec3 min;
    glm::vec3 max;
};

struct Mesh {
    std::vector<Primitive> primitives;
    glm::mat4 matrix;
};

struct Node {
    std::vector<size_t> children;
    size_t meshIndex = (size_t)-1;
    glm::mat4 matrix = glm::mat4(1.0f);
};

class GLTFModel {
public:
    bool loadFromFile(const std::string& filename, VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue);
    void destroy();

    const std::vector<Vertex>& getVertices() const { return m_vertices; }
    const std::vector<uint32_t>& getIndices() const { return m_indices; }
    const std::vector<Node>& getNodes() const { return m_nodes; }
    const std::vector<Mesh>& getMeshes() const { return m_meshes; }

    VkBuffer getVertexBuffer() const { return m_vertexBuffer; }
    VkBuffer getIndexBuffer() const { return m_indexBuffer; }
    VkDeviceMemory getVertexMemory() const { return m_vertexMemory; }
    VkDeviceMemory getIndexMemory() const { return m_indexMemory; }

    void bindBuffers(VkCommandBuffer cmd) const;
    void drawNode(VkCommandBuffer cmd, size_t nodeIndex) const;
    void draw(VkCommandBuffer cmd) const;

    glm::vec3 getMin() const { return m_min; }
    glm::vec3 getMax() const { return m_max; }

private:
    void loadNode(const tinygltf::Node& node, const tinygltf::Model& model, size_t nodeIndex, const glm::mat4& parentMatrix);

    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::vector<Node> m_nodes;
    std::vector<Mesh> m_meshes;

    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexMemory = VK_NULL_HANDLE;

    glm::vec3 m_min = glm::vec3(FLT_MAX);
    glm::vec3 m_max = glm::vec3(-FLT_MAX);
};

}