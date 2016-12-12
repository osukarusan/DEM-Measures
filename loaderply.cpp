#include "loaderply.h"
#include <fstream>
#include <iostream>


bool LoaderPLY::loadPLY(const std::string& filename, std::vector<glm::vec3>& verts, std::vector<glm::ivec3>& faces)
{
    std::string foo, format;
    unsigned int numVerts, numFaces;

    std::ifstream fin(filename, std::fstream::in | std::fstream::binary);

    std::getline(fin, foo);             // ply
    fin >> foo >> format >> foo;        // format <format> 1.0
    fin >> foo >> foo >> numVerts;      // element vertex Nv
    std::getline(fin, foo);             // \n
    std::getline(fin, foo);             // property float x
    std::getline(fin, foo);             // property float y
    std::getline(fin, foo);             // property float z
    fin >> foo >> foo >> numFaces;      // element face Nf
    std::getline(fin, foo);             // \n
    std::getline(fin, foo);             // property list uint8 int32 vertex_index
    fin >> foo;                         // end_header
    std::getline(fin, foo);             // \n

    verts.resize(numVerts);
    fin.read((char*)(&verts[0].x), numVerts*sizeof(glm::vec3));
    faces.resize(numFaces);
    for (unsigned int i = 0; i < numFaces; i++) {
        char fsize;
        fin.read((char*)(&fsize), sizeof(char));
        fin.read((char*)(&faces[i][0]), fsize*sizeof(int));
    }

    fin.close();

    return true;
}

bool LoaderPLY::writePLY(const std::string& filename, const std::vector<glm::vec3>& verts, const std::vector<glm::ivec3>& faces)
{
    std::ofstream fout(filename, std::fstream::out | std::fstream::trunc | std::fstream::binary);

    fout << "ply" << std::endl;
    fout << "format binary_little_endian 1.0" << std::endl;
    fout << "element vertex " << verts.size() << std::endl;
    fout << "property float x" << std::endl;
    fout << "property float y" << std::endl;
    fout << "property float z" << std::endl;
    fout << "element face " << faces.size() << std::endl;
    fout << "property list uint8 int32 vertex_index" << std::endl;
    fout << "end_header" << std::endl;

    fout.write((char*)(&verts[0].x), verts.size()*sizeof(glm::vec3));
    char fsize = 3;
    for (unsigned int i = 0; i < faces.size(); i++) {
        fout.write((char*)(&fsize), sizeof(char));
        fout.write((char*)(&faces[i][0]), fsize*sizeof(int));
    }

    fout.close();

    return true;
}
