#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

fs::path GetExeDirectory()
{
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);

    return fs::path(path).parent_path();
}

bool IsShaderFile(const fs::path& path)
{
    std::string ext = path.extension().string();

    for (char& c : ext)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    return ext == ".hlsl" || ext == ".hlsli";
}

int main()
{
    const fs::path folderPath = R"(D:\DX12RenderingDemo\Shaders)";

    std::cout << "검색 폴더: " << folderPath.string() << "\n\n";

    int copiedCount = 0;

    for (const auto& entry : fs::directory_iterator(folderPath))
    {
        if (!entry.is_regular_file())
            continue;

        const fs::path& srcPath = entry.path();

        if (!IsShaderFile(srcPath))
            continue;

        fs::path dstPath = srcPath;
        dstPath.replace_extension(".txt");

        try
        {
            fs::copy_file(
                srcPath,
                dstPath,
                fs::copy_options::overwrite_existing);

            std::cout
                << "[OK] "
                << srcPath.filename().string()
                << " -> "
                << dstPath.filename().string()
                << '\n';

            ++copiedCount;
        }
        catch (const fs::filesystem_error& e)
        {
            std::cerr << "[ERROR] " << e.what() << '\n';
        }
    }

    std::cout
        << "\n완료. "
        << copiedCount
        << "개의 셰이더 파일을 복사했습니다.\n";

    system("pause");
    return 0;
}