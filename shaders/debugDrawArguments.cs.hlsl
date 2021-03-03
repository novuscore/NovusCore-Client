#include "debug.inc.hlsl"

[[vk::binding(0, PER_DRAW)]] ByteAddressBuffer _vertexRanges;
[[vk::binding(1, PER_DRAW)]] ByteAddressBuffer _vertexCounters;
[[vk::binding(2, PER_DRAW)]] RWByteAddressBuffer _drawArguments;

void writeDrawArguments(DebugVertexBufferType bufferType)
{
	const uint2 vertexRange = _vertexRanges.Load2(bufferType * SIZEOF_UINT2);
	const uint vertexCounter = _vertexCounters.Load(bufferType * SIZEOF_UINT);

	// vertexCount, instanceCount, firstVertex, firstInstance
	_drawArguments.Store4(bufferType * SIZEOF_DRAW_INDIRECT_ARGUMENTS, uint4(vertexCounter, 1, vertexRange.x, 0));
}

[numthreads(1, 1, 1)]
void main()
{
	for (uint i = 0; i < DBG_VERTEX_BUFFER_COUNT; ++i)
	{
		writeDrawArguments((DebugVertexBufferType)i);
	}
}