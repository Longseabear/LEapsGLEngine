#pragma once
#include <iostream>
#include <string>
#include <vector>

#include <core/World.h>
#include <core/Type.h>
#include <core/Proxy.h>

#include "glm/glm.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <Texture2D.h>
#include <ShaderManager.h>


namespace LEapsGL {
    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
    };
    class Mesh {
        using TextureRequestor = LEapsGL::Texture2DFactory::RequestorType;

    public:
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<TextureRequestor> textures;

        Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<TextureRequestor> textures);
        void Draw();
    private:
        unsigned int VAO, VBO, EBO;

        void setupMesh();
    };

    class Model {
        using TextureRequestor = LEapsGL::Texture2DFactory::RequestorType;

    public:
        Model(const PathString path) {
            loadModel(path.c_str());
        }
        void Draw();

    private:
        vector<Mesh> meshes;
        std::string directory;

        void loadModel(string path);
        void processNode(aiNode* node, const aiScene* scene);
        Mesh processMesh(aiMesh* mesh, const aiScene* scene);
        vector<TextureRequestor> loadMaterialTextures(aiMaterial* mat, aiTextureType aiTexType, TextureType textureType);
    };
}
