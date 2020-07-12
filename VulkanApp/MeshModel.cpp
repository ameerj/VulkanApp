#include "MeshModel.h"

MeshModel::MeshModel(std::vector<Mesh> meshlist) {
    meshes = meshlist;
    model = glm::mat4(1.0f);
}

MeshModel::~MeshModel() {}

MeshModel::MeshModel() {}

size_t MeshModel::getMeshCount() {
    return meshes.size();
}

Mesh* MeshModel::getMesh(size_t index) {
    if (index >= meshes.size()) {
        return nullptr;
    }
    return &meshes[index];
}

glm::mat4 MeshModel::getModel() {
    return model;
}

void MeshModel::setModel(glm::mat4 newmodel) {
    model = newmodel;
}

std::vector<std::string> MeshModel::LoadMaterials(const aiScene* scene) {
    std::vector<std::string> textures(scene->mNumMaterials);
    for (size_t i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* mtl = scene->mMaterials[i];

        textures[i] = "";
        if (mtl->GetTextureCount(aiTextureType_DIFFUSE)) {
            aiString path;
            if (mtl->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
                int idx = std::string(path.data).rfind("\\");
                std::string filename = std::string(path.data).substr(idx + 1);

                textures[i] = filename;
            }
        }
    }
    return textures;
}

std::vector<Mesh> MeshModel::LoadNode(VkDev dev, VkQueue transfer_queue,
                                      VkCommandPool transfer_cmd_pool, aiNode* node,
                                      const aiScene* scene, std::vector<int> mat_to_tex) {
    std::vector<Mesh> meshes;
    for (size_t i = 0; i < node->mNumMeshes; i++) {
        meshes.push_back(LoadMesh(dev, transfer_queue, transfer_cmd_pool,
                                  scene->mMeshes[node->mMeshes[i]], scene, mat_to_tex));
    }
    for (size_t i = 0; i < node->mNumChildren; i++) {
        std::vector<Mesh> childmeshes;
        childmeshes =
            LoadNode(dev, transfer_queue, transfer_cmd_pool, node->mChildren[i], scene, mat_to_tex);
        meshes.insert(meshes.end(), childmeshes.begin(), childmeshes.end());
    }
    return meshes;
}

Mesh MeshModel::LoadMesh(VkDev dev, VkQueue transfer_queue, VkCommandPool transfer_cmd_pool,
                         aiMesh* mesh, const aiScene* scene, std::vector<int> mat_to_tex) {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;

    vertices.resize(mesh->mNumVertices);

    for (size_t i = 0; i < mesh->mNumVertices; i++) {
        vertices[i].pos = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

        if (mesh->mTextureCoords[0]) {
            vertices[i].tex = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
        } else {
            vertices[i].tex = {0.0f, 0.0f};
        }
        vertices[i].col = {1.0f, 1.0f, 1.0f};
    }
    for (size_t i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (size_t j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    Mesh newmesh = Mesh(dev.physical_device, dev.logical_device, transfer_queue, transfer_cmd_pool,
                        &vertices, &indices, mat_to_tex[mesh->mMaterialIndex]);
    return newmesh;
}

void MeshModel::destroyMeshModel() {
    for (auto model : meshes) {
        model.destroyBuffers();
    }
}
