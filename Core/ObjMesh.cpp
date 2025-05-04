/*****************************************************************//**
 * \file   ObjMesh.cpp
 * \brief  creates a mesh from an obj file
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   March 2025
 *********************************************************************/

#include "pch.hpp"

#include "ObjMesh.hpp"
#include "Mesh.hpp"

#include "tiny_obj_loader.h"

namespace Gep
{
    //Mesh LoadObjMesh(const std::filesystem::path& objPath)
    //{
    //    objl::Loader loader;
    //    if (!loader.LoadFile(objPath.string()))
    //    {
    //        Gep::Log::Error("Failed to load model: [", objPath.string(), "]");
    //        return {};
    //    }

    //    Gep::Mesh mesh;

    //    for (const auto& objlmesh : loader.LoadedMeshes)
    //    {

    //        for (const objl::Vertex& vertex : objlmesh.Vertices)
    //        {
    //            mesh.mVertices.push_back({{ vertex.Position.X, vertex.Position.Y, vertex.Position.Z },
    //                                      { vertex.Normal.X, vertex.Normal.Y, vertex.Normal.Z },
    //                                      { vertex.TextureCoordinate.X, vertex.TextureCoordinate.Y } });
    //        }

    //        for (uint32_t i = 0; i < objlmesh.Indices.size(); i += 3)
    //        {
    //            mesh.mFaces.push_back({ objlmesh.Indices[i], objlmesh.Indices[i + 1], objlmesh.Indices[i + 2] });
    //        }
    //    }

    //    mesh.Normalize();

    //    return mesh;
    //}

    Mesh LoadObjMesh(const std::filesystem::path& path)
    {
        Gep::Mesh mesh;

        tinyobj::ObjReader reader;
        tinyobj::ObjReaderConfig reader_config;
        reader_config.mtl_search_path = path.parent_path().string(); // Set MTL search directory

        if (!reader.ParseFromFile(path.string(), reader_config))
        {
            if (!reader.Error().empty()) {
                Gep::Log::Error("tinyobj error: ", reader.Error());
            }
            Gep::Log::Error("Failed to load model: ", path.string());
            return {};
        }
        if (!reader.Warning().empty()) {
            Gep::Log::Warning("tinyobj warning: ", reader.Warning());
        }

        // Prepare tinyobj objects
        const tinyobj::attrib_t& attrib = reader.GetAttrib();
        const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();
        const std::vector<tinyobj::material_t>& materials = reader.GetMaterials();

        // Iterate through each shape in the OBJ.
        for (const auto& shape : shapes)
        {
            size_t index_offset = 0;
            // The number of vertices per face is stored in num_face_vertices.
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
            {
                int fv = shape.mesh.num_face_vertices[f];  // Typically 3 if triangulated
                std::vector<uint32_t> face_indices;

                // Process each vertex in the face.
                for (size_t v = 0; v < static_cast<size_t>(fv); v++)
                {
                    tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                    Gep::Vertex vertex;

                    // Retrieve position (required)
                    vertex.position = {
                        attrib.vertices[3 * idx.vertex_index + 0],
                        attrib.vertices[3 * idx.vertex_index + 1],
                        attrib.vertices[3 * idx.vertex_index + 2]
                    };

                    // Retrieve normal if available.
                    if (idx.normal_index >= 0 && !attrib.normals.empty()) {
                        vertex.normal = {
                            attrib.normals[3 * idx.normal_index + 0],
                            attrib.normals[3 * idx.normal_index + 1],
                            attrib.normals[3 * idx.normal_index + 2]
                        };
                    }
                    else {
                        vertex.normal = { 0.0f, 0.0f, 0.0f };
                    }

                    // Retrieve texture coordinates if available.
                    if (idx.texcoord_index >= 0 && !attrib.texcoords.empty()) {
                        vertex.texCoord = {
                            attrib.texcoords[2 * idx.texcoord_index + 0],
                            attrib.texcoords[2 * idx.texcoord_index + 1]
                        };
                    }
                    else {
                        vertex.texCoord = { 0.0f, 0.0f };
                    }

                    // Add the vertex to the mesh and record its index.
                    uint32_t vertex_index = static_cast<uint32_t>(mesh.mVertices.size());
                    mesh.mVertices.push_back(vertex);
                    face_indices.push_back(vertex_index);
                }

                // If the face is a triangle, add it directly.
                if (face_indices.size() == 3) {
                    mesh.mFaces.push_back({ face_indices[0], face_indices[1], face_indices[2] });
                }
                // Otherwise, perform simple fan triangulation.
                else if (face_indices.size() > 3) {
                    for (size_t i = 1; i < face_indices.size() - 1; i++) {
                        mesh.mFaces.push_back({ face_indices[0], face_indices[i], face_indices[i + 1] });
                    }
                }

                index_offset += fv;
            }
        }

        mesh.Normalize();

        return mesh;
    }
}
