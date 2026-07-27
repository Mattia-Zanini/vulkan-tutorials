#include "utils.h"

#include <SDL3/SDL.h>
#include <fstream>
#include <sstream>

void showError(SDL_Window* window, const std::string& errorMessasge)
{
}

// Funzione utility per caricare rapidamente in memoria il contenuto dei file shader in formato GLSL,
// permettendone la compilazione a runtime in SPIR-V tramite la libreria shaderc.
std::string readTextFile(const std::string& filePath)
{
    std::ifstream infile(filePath);
    if (infile.is_open())
    {
        std::stringstream buffer;
        buffer << infile.rdbuf();
        const std::string output = buffer.str();
        infile.close();
        return output;
    }
    return std::string();
}