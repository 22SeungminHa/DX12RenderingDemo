#include "Texture.h"
#include "DDSTextureLoader.h"

void Texture::LoadDDS(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const std::filesystem::path& filePath
)
{
    if (!device || !cmdList)
        return;

    ThrowIfFailed(
        DirectX::CreateDDSTextureFromFile12(
            device,
            cmdList,
            filePath.c_str(),
            texture_,
            uploadBuffer_
        )
    );
}