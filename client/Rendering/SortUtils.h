#pragma once
#include <NovusTypes.h>

#include <Renderer/Descriptors/BufferDesc.h>
#include <Renderer/DescriptorSet.h>
#include <Renderer/FrameResource.h>

namespace Renderer
{
    class Renderer;
    class RenderGraphResources;
    class CommandList;
}

class SortUtils
{
public:
    struct SortParams
    {
        u32 numKeys;
        u32 maxThreadGroups;

        Renderer::BufferID keysBuffer;
        Renderer::BufferID valuesBuffer;
    };

    struct SortIndirectCountParams
    {
        u32 maxNumKeys;
        u32 maxThreadGroups;

        Renderer::BufferID numKeysBuffer;
        Renderer::BufferID keysBuffer;
        Renderer::BufferID valuesBuffer;
    };

    struct SetupParams
    {
        u32 maxThreadGroups;

        Renderer::BufferID numKeysBuffer;
        Renderer::BufferID constantBuffer;
        Renderer::BufferID countScatterArgsBuffer;
        Renderer::BufferID reduceScanArgsBuffer;
    };

    struct CountParams
    {
        u32 shiftBits;

        Renderer::BufferID constantBuffer;
        Renderer::BufferID keysBuffer;
        Renderer::BufferID sumTableBuffer;

        Renderer::BufferID countScatterArgsBuffer;
    };
    struct CountReduceParams
    {
        Renderer::BufferID constantBuffer;
        Renderer::BufferID sumTableBuffer;
        Renderer::BufferID reducedSumTableBuffer;

        Renderer::BufferID reduceScanArgsBuffer;
    };
    struct ScanParams
    {
        Renderer::BufferID constantBuffer;
        Renderer::BufferID sumTableBuffer;
        Renderer::BufferID reducedSumTableBuffer;
    };
    struct ScanAddParams
    {
        Renderer::BufferID constantBuffer;
        Renderer::BufferID sumTableBuffer;
        Renderer::BufferID reducedSumTableBuffer;

        Renderer::BufferID reduceScanArgsBuffer;
    };
    struct ScatterParams
    {
        u32 shiftBits;

        Renderer::BufferID constantBuffer;
        Renderer::BufferID keysBuffer;
        Renderer::BufferID valuesBuffer;
        Renderer::BufferID sumTableBuffer;
        Renderer::BufferID writeKeysBuffer;
        Renderer::BufferID writeValuesBuffer;

        Renderer::BufferID countScatterArgsBuffer;
    };

public:
    static void Sort(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, const SortParams& params);
    static void SortIndirectCount(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, const SortIndirectCountParams& params);

private:
    struct SortBuffers
    {
        Renderer::BufferID numKeysBuffer;
        Renderer::BufferID constantBuffer;
        Renderer::BufferID countScatterArgsBuffer;
        Renderer::BufferID reduceScanArgsBuffer;

        FrameResource<Renderer::BufferID, 2> keysBuffers;
        FrameResource<Renderer::BufferID, 2> valuesBuffers;

        Renderer::BufferID sumTableBuffer;
        Renderer::BufferID reducedSumTableBuffer;
    };

    struct ResultBuffers
    {
        Renderer::BufferID keysBuffer;
        Renderer::BufferID valuesBuffer;
    };

private:
    static SortBuffers InitBuffers(Renderer::Renderer* renderer, Renderer::CommandList& commandList, u32 numKeys);
    static ResultBuffers SortInternal(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, SortBuffers& buffers);

    static void SetupIndirectParameters(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, const SetupParams& params);
    static void Count(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, const CountParams& params);
    static void CountReduce(Renderer::Renderer* renderer, Renderer::CommandList& commandList, u32 frameIndex, const CountReduceParams& params);
    static void Scan(Renderer::Renderer* renderer, Renderer::CommandList& commandList, u32 frameIndex, const ScanParams& params);
    static void ScanAdd(Renderer::Renderer* renderer, Renderer::CommandList& commandList, u32 frameIndex, const ScanAddParams& params);
    static void Scatter(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, const ScatterParams& params);

private:
    static Renderer::DescriptorSet _setupDescriptorSet;
    static Renderer::DescriptorSet _countDescriptorSet;
    static Renderer::DescriptorSet _countReduceDescriptorSet;
    static Renderer::DescriptorSet _scanDescriptorSet;
    static Renderer::DescriptorSet _scanAddDescriptorSet;
    static Renderer::DescriptorSet _scatterDescriptorSet;
};
