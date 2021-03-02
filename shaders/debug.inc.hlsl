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
	ByteAddressBuffer offsetBuffer; 
	RWByteAddressBuffer countBuffer;
	RWByteAddressBuffer vertexBuffer;
};

#define SIZEOF_UINT32 4
#define SIZEOF_DEBUG_VERTEX 16

void appendDebugVertex(DebugDrawContext ctx, DebugVertexBufferType type, float3 position, uint color)
{
	uint localOffset;
	ctx.countBuffer.AtomicAdd(type * SIZEOF_UINT32, 1, localOffset);
	// todo: bounds checking

	const uint offset = ctx.offsetBuffer.Load(type * SIZEOF_UINT32) + localOffset;
	ctx.vertexBuffer.Store4(offset * SIZEOF_DEBUG_VERTEX, uint4(asuint(position), color));
}

void debugDrawLine3D(DebugDrawContext ctx, float3 A, float3 B, uint color)
{
	appendDebugVertex(ctx, DBG_VERTEX_BUFFER_LINES_3D, A, color);
	appendDebugVertex(ctx, DBG_VERTEX_BUFFER_LINES_3D, B, color);
}