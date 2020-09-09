#include "TextureLoader.h"
#include <Containers/StringTable.h>
#include <filesystem>
#include <entt.hpp>

#include "../../ECS/Components/Singletons/TextureSingleton.h"

namespace fs = std::filesystem;

bool TextureLoader::Load(entt::registry* registry)
{
    fs::path absolutePath = std::filesystem::absolute("Data/extracted/Textures");
    if (!fs::is_directory(absolutePath))
    {
        NC_LOG_ERROR("Failed to find Textures folder");
        return false;
    }

    fs::path filePath = absolutePath.append("TextureStringTable.nst");
    if (!fs::exists(filePath) || filePath.extension() != ".nst")
    {
        NC_LOG_ERROR("Failed to find TextureStringTable.nst");
        return false;
    }

    FileReader stringTableFile(filePath.string(), filePath.filename().string());
    if (!stringTableFile.Open())
    {
        NC_LOG_ERROR("Failed to open TextureStringTable.nst");
        return false;
    }

    TextureSingleton& textureSingleton = registry->set<TextureSingleton>();

    std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<8388608>();
    stringTableFile.Read(buffer.get(), buffer->size);

    textureSingleton.textureStringTable.Deserialize(buffer.get());
    assert(textureSingleton.textureStringTable.GetNumStrings() > 0); // We always expect to have at least 1 string in our texture stringtable

    NC_LOG_SUCCESS("Loaded Texture StringTable with %u entries", textureSingleton.textureStringTable.GetNumStrings());
    return true;
}