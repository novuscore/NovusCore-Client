
static float3 axis[8] =
{
    float3(0,0,0),
    float3(1,0,0),
    float3(0,1,0),
    float3(1,1,0),

    float3(0,0,1),
    float3(1,0,1),
    float3(0,1,1),
    float3(1,1,1),
};

float3 transform_to_clip(float3 worldPos, float4x4 viewMat)
{
    float4 worldPoint = float4(worldPos.x, worldPos.y, worldPos.z, 1.0);
    float4 clipPoint = mul(worldPoint, viewMat);
    clipPoint /= clipPoint.w;

    return clipPoint.xyz;
}

bool IsVisible(float3 AABBMin, float3 AABBMax,float3 eye ,Texture2D<float> pyramid, SamplerState samplerState, float4x4 viewMat)
{
    if (eye.x < AABBMax.x && eye.x > AABBMin.x)
    {
        if (eye.y < AABBMax.y && eye.y > AABBMin.y)
        {
            if (eye.z < AABBMax.z && eye.z > AABBMin.z)
            {
                return true;
            }
        }
    }

    float3 center = transform_to_clip(lerp(AABBMin,AABBMax,0.5), viewMat);

    float2 pmin = center.xy;
    float2 pmax = center.xy;
    float2 depth = center.z;// x max, y min

    for (int i = 0; i < 8; i++)
    {
        float pX = lerp(AABBMin.x, AABBMax.x, axis[i].x);
        float pY = lerp(AABBMin.y, AABBMax.y, axis[i].y);
        float pZ = lerp(AABBMin.z, AABBMax.z, axis[i].z);

        
        float3 clipPoint = transform_to_clip(float3(pX,pY,pZ), viewMat);

        pmin.x = min(clipPoint.x, pmin.x);
        pmin.y = min(clipPoint.y, pmin.y);

        pmax.x = max(clipPoint.x, pmax.x);
        pmax.y = max(clipPoint.y, pmax.y);

        depth.x = max(clipPoint.z, depth.x);
        depth.y = min(clipPoint.z, depth.y);
    }

    uint pyrWidth;
    uint pyrHeight;
    pyramid.GetDimensions(pyrWidth, pyrHeight);

    //convert max and min into UV space
    pmin = pmin * float2(0.5f, -0.5f) + float2(0.5f,0.5f);
    pmax = pmax * float2(0.5f, -0.5f) + float2(0.5f,0.5f);
   
    //calculate pixel widths/height
    float boxWidth = abs(pmax.x-pmin.x) * (float)pyrWidth;
    float boxHeight = abs(pmax.y-pmin.y) * (float)pyrHeight;

    float level = ceil(log2(max(boxWidth, boxHeight)));

    float2 psample = lerp(pmin,pmax,0.5);

    float sampleDepth = pyramid.SampleLevel(samplerState, psample, level).x;

    return sampleDepth <= depth.x;
};