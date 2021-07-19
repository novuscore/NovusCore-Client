struct VertexInput
{
    uint vertexID : SV_VertexID;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VertexOutput main(VertexInput input)
{
    /*
        //See: https://web.archive.org/web/20140719063725/http://www.altdev.co/2011/08/08/interesting-vertex-shader-trick/

           1
        ( 0, 2)
        [-1, 3]   [ 3, 3]
            .
            |`.
            |  `.
            |    `.
            '------`
           0         2
        ( 0, 0)   ( 2, 0)
        [-1,-1]   [ 3,-1]

        ID=0 -> Pos=[-1,-1], Tex=(0,0)
        ID=1 -> Pos=[-1, 3], Tex=(0,2)
        ID=2 -> Pos=[ 3,-1], Tex=(2,0)
    */

    VertexOutput output;
    output.uv.x = (input.vertexID == 2) ? 2.0 : 0.0;
    output.uv.y = (input.vertexID == 1) ? 2.0 : 0.0;

    output.position = float4(output.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);

    return output;
}