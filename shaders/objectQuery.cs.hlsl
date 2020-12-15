struct Constants
{
    uint2 requests[15];
    uint numRequests;
};

struct ObjectData
{
    uint type;
    uint value;
};

[[vk::push_constant]] Constants _constants;
[[vk::binding(0, PER_PASS)]] Texture2D<uint> _texture;
[[vk::binding(1, PER_PASS)]] RWStructuredBuffer<ObjectData> _result;

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint numRequests = _constants.numRequests;

    for (uint i = 0; i < numRequests; i++)
    {
        uint2 pixelData = _constants.requests[i];
        uint objectID = _texture.Load(uint3(pixelData, 0));

        _result[i].type = objectID >> 28;
        _result[i].value = objectID & 0x0FFFFFFF;
    }
}