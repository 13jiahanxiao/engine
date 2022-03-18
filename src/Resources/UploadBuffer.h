#pragma once
#include"Buffer.h"
#include"../Utility/d3dUtil.h"

template<typename T>
class UploadBuffer final : public Buffer
{
public:
    UploadBuffer(Device* device, UINT elementCount, bool isConstantBuffer) :Buffer(device),
        mIsConstantBuffer(isConstantBuffer)
    {
        mElementByteSize = sizeof(T);
        if (isConstantBuffer)
            mElementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));

        auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto buffer = CD3DX12_RESOURCE_DESC::Buffer(elementCount * mElementByteSize);
        ThrowIfFailed(device->GetDevice()->CreateCommittedResource(
            &prop,
            D3D12_HEAP_FLAG_NONE,
            &buffer,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_Resource)));

        ThrowIfFailed(m_Resource->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
    }

    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;

    ~UploadBuffer()
    {
        if (m_Resource != nullptr)
            m_Resource->Unmap(0, nullptr);

        mMappedData = nullptr;
    }

    ID3D12Resource* GetResource()const
    {
        return m_Resource.Get();
    }

    void CopyData(int elementIndex, const T& data)
    {
        memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
    }

    D3D12_RESOURCE_STATES GetInitState() const override
    {
        return D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const override 
    { 
        return m_Resource->GetGPUVirtualAddress();
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
    BYTE* mMappedData = nullptr;

    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;
};