#include "../items/items.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <fstream>
#include <iostream>

#ifndef MATERIAL_HPP
#define MATERIAL_HPP

void saveMaterial(Material mat, const std::string &matName, bool edit = false);

void removeMaterial(const std::string &matName);

Material loadMaterial(const std::string &matName);

std::vector<std::string> loadMaterialNames();

std::vector<Material> loadMaterialsfromFile();

#endif