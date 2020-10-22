
struct PanelData
{
    float4 color;
    uint4 borderSize;
    uint4 borderInset;
    uint4 slicingOffset;
    float2 dimensions;
};

[[vk::binding(0, PER_PASS)]] SamplerState _sampler;
[[vk::binding(1, PER_DRAW)]] ConstantBuffer<PanelData> _panelData;
[[vk::binding(2, PER_DRAW)]] Texture2D<float4> _texture;
[[vk::binding(3, PER_DRAW)]] Texture2D<float4> _border;

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float Map(float value, float originalMin, float originalMax, float newMin, float newMax)
{
    return (value - originalMin) / (originalMax - originalMin) * (newMax - newMin) + newMin;
}

float NineSliceAxis(float coord, float pixelBorderMin, float pixelBorderMax, float scaledPixelBorderMin, float scaledPixelBorderMax)
{
    // "Min" side of axis
    if (coord < pixelBorderMin)
        return Map(coord, 0, pixelBorderMin, 0, scaledPixelBorderMin);
        
    // Middle part of axis
    if (coord < 1 - pixelBorderMax)
        return Map(coord, pixelBorderMin, 1 - pixelBorderMax, scaledPixelBorderMin, 1 - scaledPixelBorderMax);
    
    // "Max" side of axis
    return Map(coord, 1 - pixelBorderMax, 1, 1 - scaledPixelBorderMax, 1);
}

float4 GetBorderColor(float2 uv)
{
    float2 pixelTextureDimension; // Dimension of the actual texture, without any scaling
    _border.GetDimensions(pixelTextureDimension.x, pixelTextureDimension.y);
    
    // TODO: Maybe toggle BorderColor stuff through the constant buffer instead
    if (pixelTextureDimension.x == 1)
    {
        return float4(0,0,0,0);
    }

    uint sliceWidth = pixelTextureDimension.x / 8;
    uint sliceHeight = pixelTextureDimension.y;
    
    float sliceWidthUV = 1.0f / 8.0f;
    
    float topBorderSize = _panelData.borderSize.x;
    float rightBorderSize = _panelData.borderSize.y;
    float bottomBorderSize = _panelData.borderSize.z;
    float leftBorderSize = _panelData.borderSize.w;
    
    float topBorderUVOffset = topBorderSize / _panelData.dimensions.y;
    float rightBorderUVOffset = rightBorderSize / _panelData.dimensions.x;
    float bottomBorderUVOffset = bottomBorderSize / _panelData.dimensions.y;
    float leftBorderUVOffset = leftBorderSize / _panelData.dimensions.x;
    
    float2 adjustedUV = uv;
    
    // Corners
    if (uv.x < leftBorderUVOffset && uv.y < topBorderUVOffset) // Top left Corner
    {
        float startUV = sliceWidthUV * 4;
        float endUV = sliceWidthUV * 5;
        
        adjustedUV.x = Map(uv.x, 0, leftBorderUVOffset, startUV, endUV);
        adjustedUV.y = Map(uv.y, 0, topBorderUVOffset, 0, 1);
    }
    else if (uv.x < leftBorderUVOffset && uv.y > 1 - bottomBorderUVOffset) // Bottom Left Corner
    {
        float startUV = sliceWidthUV * 6;
        float endUV = sliceWidthUV * 7;
        
        adjustedUV.x = Map(uv.x, 0, leftBorderUVOffset, startUV, endUV);
        adjustedUV.y = Map(uv.y, 1 - bottomBorderUVOffset, 1, 0, 1);
    }
    else if (uv.x > 1 - rightBorderUVOffset && uv.y < topBorderUVOffset) // Top Right Corner
    {
        float startUV = sliceWidthUV * 5;
        float endUV = sliceWidthUV * 6;
        
        adjustedUV.x = Map(uv.x, 1 - rightBorderUVOffset, 1, startUV, endUV);
        adjustedUV.y = Map(uv.y, 0, topBorderUVOffset, 0, 1);
    }
    else if (uv.x > 1 - rightBorderUVOffset && uv.y > 1 - bottomBorderUVOffset) // Bottom Right Corner
    {
        float startUV = sliceWidthUV * 7;
        float endUV = sliceWidthUV * 8;
        
        adjustedUV.x = Map(uv.x, 1 - rightBorderUVOffset, 1, startUV, endUV);
        adjustedUV.y = Map(uv.y, 1 - bottomBorderUVOffset, 1, 0, 1);
    }
    
    // Sides
    else if (uv.x < leftBorderUVOffset) // Left Side
    {
        float startUV = sliceWidthUV * 0;
        float endUV = sliceWidthUV * 1;
        
        adjustedUV.x = Map(uv.x, 0, leftBorderUVOffset, startUV, endUV);
    }
    else if (uv.x > 1 - rightBorderUVOffset) // Right Side
    {
        float startUV = sliceWidthUV * 1;
        float endUV = sliceWidthUV * 2;
        
        adjustedUV.x = Map(uv.x, 1 - rightBorderUVOffset, 1, startUV, endUV);
    }
    else if (uv.y < topBorderUVOffset) // Top Side
    {
        float startUV = sliceWidthUV * 2;
        float endUV = sliceWidthUV * 3;
        
        // X and Y is flipped here on purpose to 90 degree flip border
        adjustedUV.x = Map(uv.y, 0, topBorderUVOffset, startUV, endUV);
        adjustedUV.y = uv.x;
    }
    else if (uv.y > 1 - bottomBorderUVOffset) // Bottom Side
    {
        float startUV = sliceWidthUV * 3;
        float endUV = sliceWidthUV * 4;
        
        // X and Y is flipped here on purpose to 90 degree flip border
        adjustedUV.x = Map(uv.y, 1 - bottomBorderUVOffset, 1, startUV, endUV); 
        adjustedUV.y = uv.x;
    }
    else
    {
        return float4(0,0,0,0);   
    }
    
    
    return _border.SampleLevel(_sampler, adjustedUV, 0);
}

float4 GetColor(float2 uv)
{
    float2 pixel = uv * _panelData.dimensions;
    
    float topBorderInset = _panelData.borderInset.x;
    float rightBorderInset = _panelData.borderInset.y;
    float bottomBorderInset = _panelData.borderInset.z;
    float leftBorderInset = _panelData.borderInset.w;
    
    if (pixel.x < leftBorderInset)
        return float4(0,0,0,0);
    
    if (pixel.x > _panelData.dimensions.x - rightBorderInset)
        return float4(0,0,0,0);
    
    if (pixel.y < topBorderInset)
        return float4(0,0,0,0);
    
    if (pixel.y > _panelData.dimensions.y - bottomBorderInset)
        return float4(0,0,0,0);
    
    return _texture.SampleLevel(_sampler, uv, 0) * _panelData.color;
}

float4 main(VertexOutput input) : SV_Target
{
    float2 pixelTextureDimension; // Dimension of the actual texture, without any scaling
    _texture.GetDimensions(pixelTextureDimension.x, pixelTextureDimension.y);
    
    float2 scaledPixelTextureDimension = _panelData.dimensions; // Dimension of the scaled image in our engine
    
    float topSlicingOffset = _panelData.slicingOffset.x;
    float rightSlicingOffset = _panelData.slicingOffset.y;
    float bottomSlicingOffset = _panelData.slicingOffset.z;
    float leftSlicingOffset = _panelData.slicingOffset.w;
    
    float horizontalPixelBorderMin = leftSlicingOffset / pixelTextureDimension.x;
    float horizontalPixelBorderMax = rightSlicingOffset / pixelTextureDimension.x;
    
    float verticalPixelBorderMin = topSlicingOffset / pixelTextureDimension.y;
    float verticalPixelBorderMax = bottomSlicingOffset / pixelTextureDimension.y;

    float scaledHorizontalPixelBorderMin = leftSlicingOffset / scaledPixelTextureDimension.x;
    float scaledHorizontalPixelBorderMax = rightSlicingOffset / scaledPixelTextureDimension.x;
    
    float scaledVerticalPixelBorderMin = topSlicingOffset / scaledPixelTextureDimension.y;
    float scaledVerticalPixelBorderMax = bottomSlicingOffset / scaledPixelTextureDimension.y;

    float2 scaledUV = float2(
        NineSliceAxis(input.uv.x, scaledHorizontalPixelBorderMin, scaledHorizontalPixelBorderMax, horizontalPixelBorderMin, horizontalPixelBorderMax),
        NineSliceAxis(input.uv.y, scaledVerticalPixelBorderMin, scaledVerticalPixelBorderMax, verticalPixelBorderMin, verticalPixelBorderMax)
    );
    
    float4 borderColor = GetBorderColor(scaledUV);
    float4 backgroundColor = GetColor(scaledUV);

    float4 color = borderColor + backgroundColor;
    
    return saturate(color);
}