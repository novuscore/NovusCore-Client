#include "../LoaderSystem.h"
#include "../../Utils/ServiceLocator.h"
#include "../../ECS/Components/Singletons/TextureSingleton.h"

#include <NovusTypes.h>
#include <entt.hpp>
#include <execution>
#include <filesystem>
namespace fs = std::filesystem;

struct TexturePair
{
    u32 hash;
    std::string path;
};

class TextureLoader : Loader
{
public:
    TextureLoader() : Loader("TextureLoader", 999) { }

    bool Init()
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        TextureSingleton& textureSingleton = registry->set<TextureSingleton>();

        fs::path relativeParentPath = "Data/extracted/Textures";
        fs::path absolutePath = std::filesystem::absolute(relativeParentPath).make_preferred();

        std::string absolutePathStr = absolutePath.string();
        size_t subStrIndex = absolutePathStr.length() + 1; // + 1 here for folder seperator

        if (!fs::is_directory(absolutePath))
        {
            NC_LOG_ERROR("Failed to find Textures folder");
            return false;
        }

        static const fs::path fileExtension = ".dds";

        std::vector<std::filesystem::path> paths;
        moodycamel::ConcurrentQueue<TexturePair> texturePairs;

        std::filesystem::recursive_directory_iterator dirpos{ absolutePath };
        std::copy(begin(dirpos), end(dirpos), std::back_inserter(paths));

        std::for_each(std::execution::par, std::begin(paths), std::end(paths), [&subStrIndex, &relativeParentPath, &texturePairs](const std::filesystem::path& path)
        {
            if (!path.has_extension() || path.extension().compare(fileExtension) != 0)
                return;

            std::string texturePath = path.string().substr(subStrIndex);

            TexturePair texturePair;
            texturePair.path = texturePath;
            texturePair.hash = StringUtils::fnv1a_32(texturePath.c_str(), texturePath.length());

            texturePairs.enqueue(texturePair);
        });

        textureSingleton.textureHashToPath.reserve(texturePairs.size_approx());

        TexturePair texturePair;
        while (texturePairs.try_dequeue(texturePair))
        {
            auto itr = textureSingleton.textureHashToPath.find(texturePair.hash);
            if (itr != textureSingleton.textureHashToPath.end())
            {
                NC_LOG_ERROR("Found duplicate texture hash (%u) for Path (%s)", texturePair.hash, texturePair.path.c_str()); // This error cannot be more specific when loading in parallel unless we copy more data.
            }

            textureSingleton.textureHashToPath[texturePair.hash] = (relativeParentPath / texturePair.path).string();
        }

        NC_LOG_SUCCESS("Loaded Texture %u entries", textureSingleton.textureHashToPath.size());
        return true;
    }
};

TextureLoader textureLoader;