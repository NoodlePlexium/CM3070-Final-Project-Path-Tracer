#pragma once 

// EXTERNAL LIBRARIES
#include <GL/glew.h>

// STANDARD LIBRARY
#include <vector>
#include <string>

// PROJECT HEADERS
#include "redflare_mesh.h"


class ModelManager
{
public:
    std::vector<Mesh*> meshes;

    void CopyMeshDataToGPU(const std::vector<Mesh*> &meshes, unsigned int &pathtraceShader)
    {
        std::vector<MeshPartition> meshPartitions;
        uint32_t vertexStart = 0;
        uint32_t indexStart = 0;
        uint32_t materialIndex = 0;
        uint32_t bvhStart = 0;
        for (const Mesh* mesh : meshes) 
        {
            MeshPartition mPart;
            mPart.verticesStart = vertexStart;
            mPart.indicesStart = indexStart;
            mPart.materialIndex = materialIndex;
            mPart.bvhNodeStart = bvhStart;
            vertexStart += mesh->vertices.size();
            indexStart += mesh->indices.size();
            materialIndex += 1;
            bvhStart += mesh->nodesUsed;
            meshPartitions.push_back(mPart);
        }

        size_t vertexBufferSize = 0;
        size_t indexBufferSize = 0;
        size_t materialBufferSize = 0;
        size_t bvhBufferSize = 0;
        for (const Mesh* mesh : meshes) {
            vertexBufferSize += mesh->vertices.size() * sizeof(Vertex);
            indexBufferSize += mesh->indices.size() * sizeof(uint32_t);
            materialBufferSize += sizeof(Material);
            bvhBufferSize += mesh->nodesUsed * sizeof(BVH_Node);
        }

        glUseProgram(pathtraceShader);
        glUniform1i(glGetUniformLocation(pathtraceShader, "u_meshCount"), meshes.size());

        // VERTEX BUFFER
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexBuffer);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, vertexBufferSize, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);  
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vertexBuffer);
        void* mappedVertexBuffer = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, vertexBufferSize, GL_MAP_WRITE_BIT);

        // INDEX BUFFER
        glGenBuffers(1, &indexBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexBuffer);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, indexBufferSize, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);  
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, indexBuffer);
        void* mappedIndexBuffer = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, indexBufferSize, GL_MAP_WRITE_BIT);

        // MATERIAL BUFFER
        glGenBuffers(1, &materialBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialBuffer);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, materialBufferSize, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, materialBuffer);
        void* mappedMaterialBuffer = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, materialBufferSize, GL_MAP_WRITE_BIT);

        // BVH BUFFER
        glGenBuffers(1, &bvhBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvhBuffer);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, bvhBufferSize, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);  // Use glBufferStorage instead
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, bvhBuffer);
        void* mappedBVHBuffer = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bvhBufferSize, GL_MAP_WRITE_BIT);

        // PARTITION BUFFER
        glGenBuffers(1, &partitionBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, partitionBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(MeshPartition) * meshPartitions.size(), meshPartitions.data(), GL_STATIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, partitionBuffer);

        // COPY VERTEX INDEX AND MATERIAL DATA TO THE GPU
        uint32_t vertexOffset = 0;
        uint32_t indexOffset = 0;
        uint32_t materialOffset = 0;
        uint32_t bvhOffset = 0;
        for (const Mesh* mesh : meshes) 
        {
            memcpy((char*)mappedVertexBuffer + vertexOffset, mesh->vertices.data(),
                mesh->vertices.size() * sizeof(Vertex));
            memcpy((char*)mappedIndexBuffer + indexOffset, mesh->indices.data(),
                mesh->indices.size() * sizeof(uint32_t));
            memcpy((char*)mappedMaterialBuffer + materialOffset, &mesh->material,
                sizeof(Material));
            memcpy((char*)mappedBVHBuffer + bvhOffset, mesh->bvhNodes,
                mesh->nodesUsed * sizeof(BVH_Node));

            vertexOffset += mesh->vertices.size() * sizeof(Vertex);
            indexOffset += mesh->indices.size() * sizeof(uint32_t);
            materialOffset += sizeof(Material);
            bvhOffset += mesh->nodesUsed * sizeof(BVH_Node);
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexBuffer);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);  
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexBuffer);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);  
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialBuffer);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);  
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvhBuffer);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);  
    }

    ~ModelManager()
    {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteBuffers(1, &indexBuffer);
        glDeleteBuffers(1, &materialBuffer);
        glDeleteBuffers(1, &partitionBuffer);
    }
private:
    unsigned int vertexBuffer;
    unsigned int indexBuffer;
    unsigned int materialBuffer;
    unsigned int bvhBuffer;
    unsigned int partitionBuffer;
};