#include "common.inc.hlsl"

#define SIZEOF_DEBUG_VERTEX 16

[[vk::binding(0, DEBUG)]] ByteAddressBuffer _debug_rangeBuffer;
[[vk::binding(1, DEBUG)]] RWByteAddressBuffer _debug_counterBuffer;
[[vk::binding(2, DEBUG)]] RWByteAddressBuffer _debug_vertexBuffer;

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
	uint offset;
};

bool beginDebugDrawing(inout DebugDrawContext ctx, uint vertexCount)
{
	const uint2 vertexRange = _debug_rangeBuffer.Load2(DBG_VERTEX_BUFFER_LINES_3D * SIZEOF_UINT2);

	uint counter;
	_debug_counterBuffer.InterlockedAdd(DBG_VERTEX_BUFFER_LINES_3D * SIZEOF_UINT, vertexCount, counter);
	if (counter + vertexCount > vertexRange.y)
	{
		return false;
	}

	ctx.offset = (vertexRange.x + counter) * SIZEOF_DEBUG_VERTEX;
	return true;
}

void appendDebugVertex(inout DebugDrawContext ctx, float3 position, uint color)
{
	_debug_vertexBuffer.Store4(ctx.offset, uint4(asuint(position), color));
	ctx.offset += SIZEOF_DEBUG_VERTEX;
}

void appendDebugLine(inout DebugDrawContext ctx, float3 A, float3 B, uint color)
{
	appendDebugVertex(ctx, A, color);
	appendDebugVertex(ctx, B, color);
}

void debugDrawLine3D(float3 A, float3 B, uint color)
{
	DebugDrawContext ctx;
	if (!beginDebugDrawing(ctx, 2))
	{
		return;
	}

	appendDebugLine(ctx, A, B, color);
}

void debugDrawAABB3D(float3 min, float3 max, uint color)
{
	DebugDrawContext ctx;
	if (!beginDebugDrawing(ctx, 24))
	{
		return;
	}

	// bottom
	appendDebugLine(ctx, float3(min.x, min.y, min.z), float3(max.x, min.y, min.z), color);
	appendDebugLine(ctx, float3(min.x, min.y, min.z), float3(min.x, min.y, max.z), color);
	appendDebugLine(ctx, float3(max.x, min.y, max.z), float3(max.x, min.y, min.z), color);
	appendDebugLine(ctx, float3(max.x, min.y, max.z), float3(min.x, min.y, max.z), color);

	// top
	appendDebugLine(ctx, float3(min.x, max.y, min.z), float3(max.x, max.y, min.z), color);
	appendDebugLine(ctx, float3(min.x, max.y, min.z), float3(min.x, max.y, max.z), color);
	appendDebugLine(ctx, float3(max.x, max.y, max.z), float3(max.x, max.y, min.z), color);
	appendDebugLine(ctx, float3(max.x, max.y, max.z), float3(min.x, max.y, max.z), color);

	// sides
	appendDebugLine(ctx, float3(min.x, min.y, min.z), float3(min.x, max.y, min.z), color);
	appendDebugLine(ctx, float3(max.x, min.y, min.z), float3(max.x, max.y, min.z), color);
	appendDebugLine(ctx, float3(max.x, min.y, max.z), float3(max.x, max.y, max.z), color);
	appendDebugLine(ctx, float3(min.x, min.y, max.z), float3(min.x, max.y, max.z), color);
}

void debugDrawMatrix(float4x4 mat, float3 scale)
{
    DebugDrawContext ctx;
    if (!beginDebugDrawing(ctx, 6))
    {
        return;
    }

    const float3 origin = mat[3].xyz;
    appendDebugLine(ctx, origin, origin + (mat[0].xyz * scale), 0xff0000ff);
    appendDebugLine(ctx, origin, origin + (mat[1].xyz * scale), 0x0000ff00);
    appendDebugLine(ctx, origin, origin + (mat[2].xyz * scale), 0x00ff0000);
}