#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "gltf_model.h"
#include <cstring>
#include <cmath>
#include <limits>

namespace vk_model {

static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
}

static VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

static void endSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

static void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, 
                         VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
                         VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

bool GLTFModel::loadFromFile(const std::string& filename, VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue) {
    m_device = device;
    m_physicalDevice = physicalDevice;

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    if (!ret) {
        return false;
    }

    const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
    
    for (size_t i = 0; i < scene.nodes.size(); i++) {
        loadNode(model.nodes[scene.nodes[i]], model, i, glm::mat4(1.0f));
    }

    if (m_vertices.empty() || m_indices.empty()) {
        return false;
    }

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = 0;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    VkCommandPool commandPool;
    vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);

    VkDeviceSize vertexBufferSize = sizeof(Vertex) * m_vertices.size();
    VkDeviceSize indexBufferSize = sizeof(uint32_t) * m_indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    createBuffer(device, physicalDevice, vertexBufferSize, 
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingMemory);

    void* data;
    vkMapMemory(device, stagingMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, m_vertices.data(), vertexBufferSize);
    vkUnmapMemory(device, stagingMemory);

    createBuffer(device, physicalDevice, vertexBufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 m_vertexBuffer, m_vertexMemory);

    VkCommandBuffer cmd = beginSingleTimeCommands(device, commandPool);
    VkBufferCopy copyRegion{};
    copyRegion.size = vertexBufferSize;
    vkCmdCopyBuffer(cmd, stagingBuffer, m_vertexBuffer, 1, &copyRegion);
    endSingleTimeCommands(device, queue, commandPool, cmd);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    createBuffer(device, physicalDevice, indexBufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingMemory);

    vkMapMemory(device, stagingMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, m_indices.data(), indexBufferSize);
    vkUnmapMemory(device, stagingMemory);

    createBuffer(device, physicalDevice, indexBufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 m_indexBuffer, m_indexMemory);

    cmd = beginSingleTimeCommands(device, commandPool);
    copyRegion.size = indexBufferSize;
    vkCmdCopyBuffer(cmd, stagingBuffer, m_indexBuffer, 1, &copyRegion);
    endSingleTimeCommands(device, queue, commandPool, cmd);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    vkDestroyCommandPool(device, commandPool, nullptr);

    return true;
}

void GLTFModel::loadNode(const tinygltf::Node& node, const tinygltf::Model& model, size_t nodeIndex, const glm::mat4& parentMatrix) {
    glm::mat4 localMatrix = glm::mat4(1.0f);
    
    if (node.matrix.size() == 16) {
        localMatrix = glm::make_mat4(node.matrix.data());
    } else {
        if (node.translation.size() == 3) {
            localMatrix = glm::translate(localMatrix, glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
        }
        if (node.rotation.size() == 4) {
            glm::quat q(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
            localMatrix *= glm::mat4(q);
        }
        if (node.scale.size() == 3) {
            localMatrix = glm::scale(localMatrix, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
        }
    }

    glm::mat4 globalMatrix = parentMatrix * localMatrix;

    Node newNode;
    newNode.matrix = globalMatrix;
    newNode.meshIndex = (size_t)-1;

    if (node.mesh >= 0) {
        const tinygltf::Mesh& mesh = model.meshes[node.mesh];
        Mesh newMesh;
        newMesh.matrix = globalMatrix;

        for (const auto& primitive : mesh.primitives) {
            uint32_t indexStart = static_cast<uint32_t>(m_indices.size());
            uint32_t vertexStart = static_cast<uint32_t>(m_vertices.size());
            uint32_t indexCount = 0;
            uint32_t vertexCount = 0;

            glm::vec3 posMin(FLT_MAX);
            glm::vec3 posMax(-FLT_MAX);

            if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
                const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("POSITION")->second];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                const float* posData = reinterpret_cast<const float*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);

                const float* normalData = nullptr;
                if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                    const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView& normBufferView = model.bufferViews[normAccessor.bufferView];
                    normalData = reinterpret_cast<const float*>(&buffer.data[normAccessor.byteOffset + normBufferView.byteOffset]);
                }

                const float* uvData = nullptr;
                if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                    const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor.bufferView];
                    uvData = reinterpret_cast<const float*>(&buffer.data[uvAccessor.byteOffset + uvBufferView.byteOffset]);
                }

                const float* colorData = nullptr;
                int colorComponents = 4;
                if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
                    const tinygltf::Accessor& colorAccessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
                    const tinygltf::BufferView& colorBufferView = model.bufferViews[colorAccessor.bufferView];
                    colorData = reinterpret_cast<const float*>(&buffer.data[colorAccessor.byteOffset + colorBufferView.byteOffset]);
                    colorComponents = (colorAccessor.type == TINYGLTF_TYPE_VEC3) ? 3 : 4;
                }

                glm::vec4 materialColor = glm::vec4(1.0f);
                if (primitive.material >= 0 && primitive.material < model.materials.size()) {
                    const tinygltf::Material& material = model.materials[primitive.material];
                    if (material.values.find("baseColorFactor") != material.values.end()) {
                        const auto& factor = material.values.at("baseColorFactor").ColorFactor();
                        materialColor = glm::vec4(factor[0], factor[1], factor[2], factor[3]);
                    }
                }

                vertexCount = static_cast<uint32_t>(accessor.count);

                for (size_t v = 0; v < accessor.count; v++) {
                    Vertex vert;
                    vert.pos = glm::vec3(posData[v * 3], posData[v * 3 + 1], posData[v * 3 + 2]);
                    vert.normal = normalData ? glm::vec3(normalData[v * 3], normalData[v * 3 + 1], normalData[v * 3 + 2]) : glm::vec3(0.0f);
                    vert.uv = uvData ? glm::vec2(uvData[v * 2], uvData[v * 2 + 1]) : glm::vec2(0.0f);
                    
                    if (colorData) {
                        if (colorComponents == 3) {
                            vert.color = glm::vec4(colorData[v * 3], colorData[v * 3 + 1], colorData[v * 3 + 2], 1.0f) * materialColor;
                        } else {
                            vert.color = glm::vec4(colorData[v * 4], colorData[v * 4 + 1], colorData[v * 4 + 2], colorData[v * 4 + 3]) * materialColor;
                        }
                    } else {
                        vert.color = materialColor;
                    }

                    m_vertices.push_back(vert);

                    posMin = glm::min(posMin, vert.pos);
                    posMax = glm::max(posMax, vert.pos);
                }

                m_min = glm::min(m_min, posMin);
                m_max = glm::max(m_max, posMax);
            }

            if (primitive.indices >= 0) {
                const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                indexCount = static_cast<uint32_t>(accessor.count);

                const void* indexData = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    const uint32_t* idx = static_cast<const uint32_t*>(indexData);
                    for (size_t i = 0; i < accessor.count; i++) {
                        m_indices.push_back(idx[i] + vertexStart);
                    }
                } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    const uint16_t* idx = static_cast<const uint16_t*>(indexData);
                    for (size_t i = 0; i < accessor.count; i++) {
                        m_indices.push_back(idx[i] + vertexStart);
                    }
                } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                    const uint8_t* idx = static_cast<const uint8_t*>(indexData);
                    for (size_t i = 0; i < accessor.count; i++) {
                        m_indices.push_back(idx[i] + vertexStart);
                    }
                }
            }

            Primitive newPrimitive;
            newPrimitive.firstIndex = indexStart;
            newPrimitive.indexCount = indexCount;
            newPrimitive.firstVertex = vertexStart;
            newPrimitive.vertexCount = vertexCount;
            newPrimitive.min = posMin;
            newPrimitive.max = posMax;
            newMesh.primitives.push_back(newPrimitive);
        }

        newNode.meshIndex = m_meshes.size();
        m_meshes.push_back(newMesh);
    }

    m_nodes.push_back(newNode);

    for (size_t childIdx : node.children) {
        size_t childNodeIdx = m_nodes.size();
        m_nodes[nodeIndex].children.push_back(childNodeIdx);
        loadNode(model.nodes[childIdx], model, childNodeIdx, globalMatrix);
    }
}

void GLTFModel::destroy() {
    if (m_device) {
        if (m_vertexBuffer) vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
        if (m_vertexMemory) vkFreeMemory(m_device, m_vertexMemory, nullptr);
        if (m_indexBuffer) vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
        if (m_indexMemory) vkFreeMemory(m_device, m_indexMemory, nullptr);
    }
    m_vertices.clear();
    m_indices.clear();
    m_meshes.clear();
    m_nodes.clear();
}

void GLTFModel::bindBuffers(VkCommandBuffer cmd) const {
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffer, offsets);
    vkCmdBindIndexBuffer(cmd, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

void GLTFModel::drawNode(VkCommandBuffer cmd, size_t nodeIndex) const {
    const Node& node = m_nodes[nodeIndex];
    
    if (node.meshIndex < m_meshes.size()) {
        const Mesh& mesh = m_meshes[node.meshIndex];
        for (const auto& primitive : mesh.primitives) {
            vkCmdDrawIndexed(cmd, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
        }
    }

    for (size_t childIdx : node.children) {
        drawNode(cmd, childIdx);
    }
}

void GLTFModel::draw(VkCommandBuffer cmd) const {
    bindBuffers(cmd);
    for (size_t i = 0; i < m_nodes.size(); i++) {
        if (std::find_if(m_nodes.begin(), m_nodes.end(), [&](const Node& n) {
            return std::find(n.children.begin(), n.children.end(), i) != n.children.end();
        }) == m_nodes.end()) {
            drawNode(cmd, i);
        }
    }
}

}