#include "common.inc.hlsl"

enum DebugVertexBufferType
{
	DBG_VERTEX_BUFFER_LINES_2D,
	DBG_VERTEX_BUFFER_LINES_3D,
	DBG_VERTEX_BUFFER_TRIS_2D,
	DBG_VERTEX_BUFFER_TRIS_3D,
	DBG_VERTEX_BUFFER_COUNT,
};

struct DebugDrawContext
{
	ByteAddressBuffer rangeBuffer; 
	RWByteAddressBuffer counterBuffer;
	RWByteAddressBuffer vertexBuffer;
};

#define SIZEOF_DEBUG_VERTEX 16

void appendDebugVertex(DebugDrawContext ctx, DebugVertexBufferType type, float3 position, uint color)
{
	const uint2 vertexRange = ctx.rangeBuffer.Load2(type * SIZEOF_UINT2);

	uint counter;
	ctx.counterBuffer.InterlockedAdd(type * SIZEOF_UINT, 1, counter);
	if (counter > vertexRange.y) 
	{
		return;
	}
	
	const uint offset = (vertexRange.x + counter) * SIZEOF_DEBUG_VERTEX;
	ctx.vertexBuffer.Store4(offset, uint4(asuint(position), color));
}

void debugDrawLine3D(DebugDrawContext ctx, float3 A, float3 B, uint color)
{
	appendDebugVertex(ctx, DBG_VERTEX_BUFFER_LINES_3D, A, color);
	appendDebugVertex(ctx, DBG_VERTEX_BUFFER_LINES_3D, B, color);
}