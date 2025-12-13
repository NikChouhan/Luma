#include "Graphics/D3D12/CommandList.h"

CommandList::CommandList(const GfxDevice& gfxDevice): gfxDevice_(gfxDevice)
{
    DX_ASSERT(gfxDevice.device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gfxDevice.commandAllocators_->Get(),
        nullptr,
        IID_PPV_ARGS(&commandList_)));
}
//#TODO: Destructor cmd
CommandList::~CommandList()
{

}
