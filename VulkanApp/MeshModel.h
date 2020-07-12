#pragma once
#include <vector>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "Utilities.h"

class MeshModel {

public:
    MeshModel(std::vector<Mesh> meshlist);
    ~MeshModel();
    MeshModel();

    size_t getMeshCount();
    Mesh* getMesh(size_t index);

    glm::mat4 getModel();
    void setModel(glm::mat4 newmodel);

    static std::vector<std::string> LoadMaterials(const aiScene* scene);
    static std::vector<Mesh> LoadNode(VkDev dev, VkQueue transfer_queue,
                                      VkCommandPool transfer_cmd_pool, aiNode* node,
                                      const aiScene* scene, std::vector<int> mat_to_tex);

    static Mesh LoadMesh(VkDev dev, VkQueue transfer_queue, VkCommandPool transfer_cmd_pool,
                         aiMesh* mesh, const aiScene* scene, std::vector<int> mat_to_tex);

    void destroyMeshModel();

private:
    std::vector<Mesh> meshes;
    glm::mat4 model;
};
