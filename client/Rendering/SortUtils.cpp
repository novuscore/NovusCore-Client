#include "SortUtils.h"
#include "FFX_ParallelSort.h"

#include <Renderer/Renderer.h>
#include <Renderer/RenderGraph.h>

#include <Renderer/Descriptors/ComputeShaderDesc.h>
#include <Renderer/Descriptors/ComputePipelineDesc.h>

Renderer::DescriptorSet SortUtils::_setupDescriptorSet;
Renderer::DescriptorSet SortUtils::_countDescriptorSet;
Renderer::DescriptorSet SortUtils::_countReduceDescriptorSet;
Renderer::DescriptorSet SortUtils::_scanDescriptorSet;
Renderer::DescriptorSet SortUtils::_scanAddDescriptorSet;
Renderer::DescriptorSet SortUtils::_scatterDescriptorSet;

// TODO: Add renderer->CreateTemporaryBuffer function which will temporarily allocate a buffer with lifetime of a variable number of frames

void SortUtils::Sort(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, const SortParams& params)
{
    SortBuffers buffers = InitBuffers(renderer, commandList, params.numKeys);

    // Create keysCount buffer
    {
        Renderer::BufferDesc desc;
        desc.name = "SortKeysCount";
        desc.size = sizeof(u32);
        desc.usage = Renderer::BufferUsage::UNIFORM_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION;
        
        buffers.numKeysBuffer = renderer->CreateTemporaryBuffer(desc, 2);

        desc.usage = Renderer::BufferUsage::TRANSFER_SOURCE;
        desc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;
        Renderer::BufferID staging = renderer->CreateBuffer(desc);

        u32* numKeys = static_cast<u32*>(renderer->MapBuffer(staging));
        *numKeys = params.numKeys;
        renderer->UnmapBuffer(staging);

        renderer->CopyBuffer(buffers.numKeysBuffer, 0, staging, 0, desc.size);
    }
    
    // Then we copy the keys and payload into the first buffer
    u32 keysBufferSize = params.numKeys * sizeof(u64);
    u32 valuesBufferSize = params.numKeys * sizeof(u32);
    commandList.CopyBuffer(buffers.keysBuffers.Get(0), 0, params.keysBuffer, 0, keysBufferSize);
    commandList.CopyBuffer(buffers.valuesBuffers.Get(0), 0, params.valuesBuffer, 0, valuesBufferSize);

    commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToComputeShaderRW, buffers.keysBuffers.Get(0));
    commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToComputeShaderRW, buffers.valuesBuffers.Get(0));

    // Setup indirect parameters
    {
        SetupParams setupParams;
        setupParams.maxThreadGroups = params.maxThreadGroups;

        setupParams.numKeysBuffer = buffers.numKeysBuffer;
        setupParams.constantBuffer = buffers.constantBuffer;
        setupParams.countScatterArgsBuffer = buffers.countScatterArgsBuffer;
        setupParams.reduceScanArgsBuffer = buffers.reduceScanArgsBuffer;

        SetupIndirectParameters(renderer, resources, commandList, frameIndex, setupParams);

        commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToComputeShaderRead, buffers.constantBuffer);
        commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToIndirectArguments, buffers.countScatterArgsBuffer);
        commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToIndirectArguments, buffers.reduceScanArgsBuffer);
    }

    ResultBuffers resultBuffers = SortInternal(renderer, resources, commandList, frameIndex, buffers);

    // Copy the result buffers back into the buffers they came from
    commandList.CopyBuffer(params.keysBuffer, 0, resultBuffers.keysBuffer, 0, keysBufferSize);
    commandList.CopyBuffer(params.valuesBuffer, 0, resultBuffers.valuesBuffer, 0, valuesBufferSize);
}

void SortUtils::SortIndirectCount(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, const SortIndirectCountParams& params)
{
    SortBuffers buffers = InitBuffers(renderer, commandList, params.maxNumKeys);

    buffers.numKeysBuffer = params.numKeysBuffer;

    // Then we copy the keys and payload into the first buffer
    u32 keysBufferSize = params.maxNumKeys * sizeof(u64);
    u32 valuesBufferSize = params.maxNumKeys * sizeof(u32);
    commandList.CopyBuffer(buffers.keysBuffers.Get(0), 0, params.keysBuffer, 0, keysBufferSize);
    commandList.CopyBuffer(buffers.valuesBuffers.Get(0), 0, params.valuesBuffer, 0, valuesBufferSize);

    commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToComputeShaderRW, buffers.keysBuffers.Get(0));
    commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToComputeShaderRW, buffers.valuesBuffers.Get(0));

    // Setup indirect parameters
    {
        SetupParams setupParams;
        setupParams.maxThreadGroups = params.maxThreadGroups;

        setupParams.numKeysBuffer = buffers.numKeysBuffer;
        setupParams.constantBuffer = buffers.constantBuffer;
        setupParams.countScatterArgsBuffer = buffers.countScatterArgsBuffer;
        setupParams.reduceScanArgsBuffer = buffers.reduceScanArgsBuffer;

        SetupIndirectParameters(renderer, resources, commandList, frameIndex, setupParams);

        commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToComputeShaderRead, buffers.constantBuffer);
        commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToIndirectArguments, buffers.countScatterArgsBuffer);
        commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToIndirectArguments, buffers.reduceScanArgsBuffer);
    }

    ResultBuffers resultBuffers = SortInternal(renderer, resources, commandList, frameIndex, buffers);

    // Copy the result buffers back into the buffers they came from
    commandList.CopyBuffer(params.keysBuffer, 0, resultBuffers.keysBuffer, 0, keysBufferSize);
    commandList.CopyBuffer(params.valuesBuffer, 0, resultBuffers.valuesBuffer, 0, valuesBufferSize);
}

SortUtils::SortBuffers SortUtils::InitBuffers(Renderer::Renderer* renderer, Renderer::CommandList& commandList, u32 numKeys)
{
    SortUtils::SortBuffers sortBuffers;

    // Create constantBuffer
    {
        Renderer::BufferDesc desc;
        desc.name = "SortConstantBuffer";
        desc.size = sizeof(FFX_ParallelSortCB);
        desc.usage = Renderer::BufferUsage::STORAGE_BUFFER | Renderer::BufferUsage::UNIFORM_BUFFER;

        sortBuffers.constantBuffer = renderer->CreateTemporaryBuffer(desc, 2);
    }

    // Create countScatterArgsBuffer
    {
        Renderer::BufferDesc desc;
        desc.name = "SortCountScatterArgs";
        desc.size = sizeof(u32) * 3;
        desc.usage = Renderer::BufferUsage::STORAGE_BUFFER | Renderer::BufferUsage::INDIRECT_ARGUMENT_BUFFER;

        sortBuffers.countScatterArgsBuffer = renderer->CreateTemporaryBuffer(desc, 2);
    }

    // Create reduceScanArgsBuffer
    {
        Renderer::BufferDesc desc;
        desc.name = "SortKeyCountsBuffer";
        desc.size = sizeof(u32) * 3;
        desc.usage = Renderer::BufferUsage::STORAGE_BUFFER | Renderer::BufferUsage::INDIRECT_ARGUMENT_BUFFER;

        sortBuffers.reduceScanArgsBuffer = renderer->CreateTemporaryBuffer(desc, 2);
    }

    // We need to create sorting buffers for both keys and values, but we need two each which we can ping-pong between during sorting
    u32 keysBufferSize = numKeys * sizeof(u64);
    u32 valuesBufferSize = numKeys * sizeof(u32);

    {
        Renderer::BufferDesc desc;

        desc.size = keysBufferSize;
        desc.usage = Renderer::BufferUsage::STORAGE_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION | Renderer::BufferUsage::TRANSFER_SOURCE;

        for (u32 i = 0; i < sortBuffers.keysBuffers.Num; i++)
        {
            desc.name = "SortKeysBuffer" + std::to_string(i);
            sortBuffers.keysBuffers.Get(i) = renderer->CreateTemporaryBuffer(desc, 2);
        }

        desc.size = valuesBufferSize;
        for (u32 i = 0; i < sortBuffers.valuesBuffers.Num; i++)
        {
            desc.name = "SortValuesBuffer" + std::to_string(i);
            sortBuffers.valuesBuffers.Get(i) = renderer->CreateTemporaryBuffer(desc, 2);
        }
    }

    // We need to create sumTableBuffer and reducedSumTableBuffer to be used by the sorting shaders
    {
        u32 sumTableSize;
        u32 reducedSumTableSize;
        FFX_ParallelSort_CalculateScratchResourceSize(numKeys, sumTableSize, reducedSumTableSize);

        Renderer::BufferDesc desc;
        desc.name = "SortSumTable";
        desc.size = sumTableSize;
        desc.usage = Renderer::BufferUsage::STORAGE_BUFFER;

        sortBuffers.sumTableBuffer = renderer->CreateTemporaryBuffer(desc, 2);

        desc.name = "ReducedSortSumTable";
        desc.size = reducedSumTableSize;

        sortBuffers.reducedSumTableBuffer = renderer->CreateTemporaryBuffer(desc, 2);
    }

    return sortBuffers;
}

SortUtils::ResultBuffers SortUtils::SortInternal(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, SortBuffers& buffers)
{
    Renderer::BufferID readKeyBuffer = buffers.keysBuffers.Get(0);
    Renderer::BufferID readPayloadBuffer = buffers.valuesBuffers.Get(0);
    Renderer::BufferID writeKeyBuffer = buffers.keysBuffers.Get(1);
    Renderer::BufferID writePayloadBuffer = buffers.valuesBuffers.Get(1);

    // Perform radix sort
    for (u32 shiftBits = 0; shiftBits < 64; shiftBits += FFX_PARALLELSORT_SORT_BITS_PER_PASS)
    {
        commandList.PushMarker("ShiftBits:" + std::to_string(shiftBits), Color::White);

        // Sort Count
        {
            CountParams countParams;
            countParams.shiftBits = shiftBits;

            countParams.constantBuffer = buffers.constantBuffer;
            countParams.keysBuffer = readKeyBuffer;
            countParams.sumTableBuffer = buffers.sumTableBuffer;

            countParams.countScatterArgsBuffer = buffers.countScatterArgsBuffer;

            Count(renderer, resources, commandList, frameIndex, countParams);

            commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToComputeShaderRead, buffers.sumTableBuffer);
        }

        // Sort Count Reduce
        {
            CountReduceParams countReduceParams;

            countReduceParams.constantBuffer = buffers.constantBuffer;
            countReduceParams.sumTableBuffer = buffers.sumTableBuffer;
            countReduceParams.reducedSumTableBuffer = buffers.reducedSumTableBuffer;

            countReduceParams.reduceScanArgsBuffer = buffers.reduceScanArgsBuffer;

            CountReduce(renderer, commandList, frameIndex, countReduceParams);

            commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToComputeShaderRead, buffers.reducedSumTableBuffer);
        }

        // Sort Scan
        {
            // First do scan prefix of reduced values
            ScanParams scanParams;

            scanParams.constantBuffer = buffers.constantBuffer;
            scanParams.sumTableBuffer = buffers.sumTableBuffer;
            scanParams.reducedSumTableBuffer = buffers.reducedSumTableBuffer;

            Scan(renderer, commandList, frameIndex, scanParams);

            commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToComputeShaderRead, buffers.reducedSumTableBuffer);

            // Next do scan prefix on the histogram with partail sums that we just did
            ScanAddParams scanAddParams;

            scanAddParams.constantBuffer = buffers.constantBuffer;
            scanAddParams.sumTableBuffer = buffers.sumTableBuffer;
            scanAddParams.reducedSumTableBuffer = buffers.reducedSumTableBuffer;

            scanAddParams.reduceScanArgsBuffer = buffers.reduceScanArgsBuffer;

            ScanAdd(renderer, commandList, frameIndex, scanAddParams);

            commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToComputeShaderRead, buffers.sumTableBuffer);
        }

        // Sort Scatter
        {
            ScatterParams scatterParams;
            scatterParams.shiftBits = shiftBits;

            scatterParams.constantBuffer = buffers.constantBuffer;
            scatterParams.keysBuffer = readKeyBuffer;
            scatterParams.valuesBuffer = readPayloadBuffer;
            scatterParams.sumTableBuffer = buffers.sumTableBuffer;
            scatterParams.writeKeysBuffer = writeKeyBuffer;
            scatterParams.writeValuesBuffer = writePayloadBuffer;

            scatterParams.countScatterArgsBuffer = buffers.countScatterArgsBuffer;

            Scatter(renderer, resources, commandList, frameIndex, scatterParams);
        }

        commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToComputeShaderRead, writeKeyBuffer);
        commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToComputeShaderRead, writePayloadBuffer);

        // Flip the buffers we read/write from for next pass
        std::swap(readKeyBuffer, writeKeyBuffer);
        std::swap(readPayloadBuffer, writePayloadBuffer);

        commandList.PopMarker();
    }

    // Return the buffers with the sorted values, since we just swapped this will be the read buffers
    commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToTransferSrc, readKeyBuffer);
    commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToTransferSrc, readPayloadBuffer);

    ResultBuffers resultBuffers;
    resultBuffers.keysBuffer = readKeyBuffer;
    resultBuffers.valuesBuffer = readPayloadBuffer;

    return resultBuffers;
}

void SortUtils::SetupIndirectParameters(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, const SetupParams& params)
{
    commandList.PushMarker("SetupIndirectParameters", Color::White);
    // Setup pipeline
    Renderer::ComputeShaderDesc shaderDesc;
    shaderDesc.path = "Sorting/SortSetupIndirectParameters.cs.hlsl";

    Renderer::ComputePipelineDesc pipelineDesc;
    pipelineDesc.computeShader = renderer->LoadShader(shaderDesc);
    Renderer::ComputePipelineID pipeline = renderer->CreatePipeline(pipelineDesc);

    commandList.BeginPipeline(pipeline);

    // Bind descriptors
    _setupDescriptorSet.Bind("_numKeys", params.numKeysBuffer);
    _setupDescriptorSet.Bind("_constants", params.constantBuffer);
    _setupDescriptorSet.Bind("_countScatterArgs", params.countScatterArgsBuffer);
    _setupDescriptorSet.Bind("_reduceScanArgs", params.reduceScanArgsBuffer);
    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_setupDescriptorSet, frameIndex);

    // Push constants
    struct SetupConstants
    {
        u32 maxThreadGroups;
    };

    SetupConstants* constants = resources.FrameNew<SetupConstants>();
    constants->maxThreadGroups = params.maxThreadGroups;
    commandList.PushConstant(constants, 0, sizeof(SetupConstants));

    // Dispatch
    commandList.Dispatch(1, 1, 1);

    commandList.EndPipeline(pipeline);
    commandList.PopMarker();
}

void SortUtils::Count(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, const CountParams& params)
{
    commandList.PushMarker("Count", Color::White);
    // Setup pipeline
    Renderer::ComputeShaderDesc shaderDesc;
    shaderDesc.path = "Sorting/SortCount.cs.hlsl";

    Renderer::ComputePipelineDesc pipelineDesc;
    pipelineDesc.computeShader = renderer->LoadShader(shaderDesc);
    Renderer::ComputePipelineID pipeline = renderer->CreatePipeline(pipelineDesc);

    commandList.BeginPipeline(pipeline);

    // Bind descriptors
    _countDescriptorSet.Bind("_constants", params.constantBuffer);
    _countDescriptorSet.Bind("_keys", params.keysBuffer);
    _countDescriptorSet.Bind("_sumTable", params.sumTableBuffer);
    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_countDescriptorSet, frameIndex);

    // Push constants
    struct CountConstants
    {
        u32 shiftBits;
    };

    CountConstants* constants = resources.FrameNew<CountConstants>();
    constants->shiftBits = params.shiftBits;
    commandList.PushConstant(constants, 0, sizeof(CountConstants));

    // Dispatch
    commandList.DispatchIndirect(params.countScatterArgsBuffer, 0);

    commandList.EndPipeline(pipeline);
    commandList.PopMarker();
}

void SortUtils::CountReduce(Renderer::Renderer* renderer, Renderer::CommandList& commandList, u32 frameIndex, const CountReduceParams& params)
{
    commandList.PushMarker("CountReduce", Color::White);
    // Setup pipeline
    Renderer::ComputeShaderDesc shaderDesc;
    shaderDesc.path = "Sorting/SortCountReduce.cs.hlsl";

    Renderer::ComputePipelineDesc pipelineDesc;
    pipelineDesc.computeShader = renderer->LoadShader(shaderDesc);
    Renderer::ComputePipelineID pipeline = renderer->CreatePipeline(pipelineDesc);

    commandList.BeginPipeline(pipeline);

    // Bind descriptors
    _countReduceDescriptorSet.Bind("_constants", params.constantBuffer);
    _countReduceDescriptorSet.Bind("_sumTable", params.sumTableBuffer);
    _countReduceDescriptorSet.Bind("_reducedSumTable", params.reducedSumTableBuffer);
    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_countReduceDescriptorSet, frameIndex);

    // Dispatch
    commandList.DispatchIndirect(params.reduceScanArgsBuffer, 0);

    commandList.EndPipeline(pipeline);
    commandList.PopMarker();
}

void SortUtils::Scan(Renderer::Renderer* renderer, Renderer::CommandList& commandList, u32 frameIndex, const ScanParams& params)
{
    commandList.PushMarker("Scan", Color::White);
    // Setup pipeline
    Renderer::ComputeShaderDesc shaderDesc;
    shaderDesc.path = "Sorting/SortScan.cs.hlsl";

    Renderer::ComputePipelineDesc pipelineDesc;
    pipelineDesc.computeShader = renderer->LoadShader(shaderDesc);
    Renderer::ComputePipelineID pipeline = renderer->CreatePipeline(pipelineDesc);

    commandList.BeginPipeline(pipeline);

    // Bind descriptors
    _scanDescriptorSet.Bind("_constants", params.constantBuffer);
    _scanDescriptorSet.Bind("_scanSrc", params.reducedSumTableBuffer);
    _scanDescriptorSet.Bind("_scanDst", params.reducedSumTableBuffer);
    //_scanDescriptorSet.Bind("_scanScratch", params.scratchBuffer); // Is this needed? This shader shouldn't use it but Vulkan might say that it needs to be bound anyway
    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_scanDescriptorSet, frameIndex);

    // Dispatch
    commandList.Dispatch(1, 1, 1);

    commandList.EndPipeline(pipeline);
    commandList.PopMarker();
}

void SortUtils::ScanAdd(Renderer::Renderer* renderer, Renderer::CommandList& commandList, u32 frameIndex, const ScanAddParams& params)
{
    commandList.PushMarker("ScanAdd", Color::White);
    // Setup pipeline
    Renderer::ComputeShaderDesc shaderDesc;
    shaderDesc.path = "Sorting/SortScanAdd.cs.hlsl";

    Renderer::ComputePipelineDesc pipelineDesc;
    pipelineDesc.computeShader = renderer->LoadShader(shaderDesc);
    Renderer::ComputePipelineID pipeline = renderer->CreatePipeline(pipelineDesc);

    commandList.BeginPipeline(pipeline);

    // Bind descriptors
    _scanAddDescriptorSet.Bind("_constants", params.constantBuffer);
    _scanAddDescriptorSet.Bind("_scanSrc", params.sumTableBuffer);
    _scanAddDescriptorSet.Bind("_scanDst", params.sumTableBuffer);
    _scanAddDescriptorSet.Bind("_scanScratch", params.reducedSumTableBuffer);
    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_scanAddDescriptorSet, frameIndex);

    // Dispatch
    commandList.DispatchIndirect(params.reduceScanArgsBuffer, 0);

    commandList.EndPipeline(pipeline);
    commandList.PopMarker();
}

void SortUtils::Scatter(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex, const ScatterParams& params)
{
    commandList.PushMarker("Scatter", Color::White);
    // Setup pipeline
    Renderer::ComputeShaderDesc shaderDesc;
    shaderDesc.path = "Sorting/SortScatter.cs.hlsl";

    Renderer::ComputePipelineDesc pipelineDesc;
    pipelineDesc.computeShader = renderer->LoadShader(shaderDesc);
    Renderer::ComputePipelineID pipeline = renderer->CreatePipeline(pipelineDesc);

    commandList.BeginPipeline(pipeline);

    // Bind descriptors
    _scatterDescriptorSet.Bind("_constants", params.constantBuffer);
    _scatterDescriptorSet.Bind("_keys", params.keysBuffer);
    _scatterDescriptorSet.Bind("_values", params.valuesBuffer);
    _scatterDescriptorSet.Bind("_sumTable", params.sumTableBuffer);
    _scatterDescriptorSet.Bind("_writeKeys", params.writeKeysBuffer);
    _scatterDescriptorSet.Bind("_writeValues", params.writeValuesBuffer);
    commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_PASS, &_scatterDescriptorSet, frameIndex);

    // Push constants
    struct ScatterConstants
    {
        u32 shiftBits;
    };

    ScatterConstants* constants = resources.FrameNew<ScatterConstants>();
    constants->shiftBits = params.shiftBits;
    commandList.PushConstant(constants, 0, sizeof(ScatterConstants));

    // Dispatch
    commandList.DispatchIndirect(params.countScatterArgsBuffer, 0);

    commandList.EndPipeline(pipeline);
    commandList.PopMarker();
}
