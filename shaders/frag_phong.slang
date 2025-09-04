

struct CoarseVertex
{
    float3 worldPosition;
    float3 worldNormal;
    float2 uv;
};

struct VertexStageOutput
{
    CoarseVertex coarseVertex : CoarseVertex;
    float4       sv_position  : SV_Position;
    float3 camera_position : TEXCOORD0;
};


float3 dirlight( float3 camW, float3 lightDirW, //camera pos, light direction
float3 ambcol, float3 diffcol, float3 speccol, //light color
float3 posW, float3 N, float3 color) { //frag position, normal, color
float3 V = normalize( camW - posW );
float3 L = normalize(lightDirW);
//diffuse
float3 diffuse = max( dot( N, L ), 0.0 ) * diffcol;
//specular
float3 R = normalize( reflect( -L, N ) ); //reflected light
float3 specular = pow( max( dot( V, R ), 0.0), 2.0) * speccol;
//add up to get the result
return (ambcol + diffuse + specular) * color;
}


[[vk::binding(1)]]
Sampler2D texSampler;


[shader("fragment")]
float4 fragmentMain(VertexStageOutput input : VertexStageOutput) : SV_Target 
{
    float4 fragColor = texSampler.Sample(input.coarseVertex.uv);

    float3 lightDirW = float3(2.0f,-1.0f,2.0f);
//colors
float3 ambcol = float3(0.1,0.1,0.1);
float3 diffcol = float3(0.1,0.4,0.2);
float3 speccol = float3(1,0.4,1);
float3 camW =  input.camera_position;
float3 result = dirlight( camW, lightDirW,
ambcol, diffcol, speccol,
input.coarseVertex.worldPosition, input.coarseVertex.worldNormal, fragColor.rgb);
return float4( result, 1.0 );
}







