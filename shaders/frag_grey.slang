struct CoarseVertex
{
    float3 fragColor;
    float2 uv;
};

[[vk::binding(1)]]
Sampler2D texSampler;


[shader("fragment")]
float4 fragmentMain(CoarseVertex coarseVertex : CoarseVertex) : SV_Target 
{
    float4 texColor = texSampler.Sample(coarseVertex.uv);
    float average = (texColor.x + texColor.y + texColor.z) / 3.;
    texColor.x = average;
    texColor.y = average;
    texColor.z = average;
    return texColor;
}







