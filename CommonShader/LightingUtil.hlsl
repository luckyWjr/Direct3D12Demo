#define MaxLights 16
#define PI 3.14

struct Material
{
	float4 albedo;
	float3 fresnelR0;
	float perceptualRoughness;
};

struct Light
{
	float3 strength;
	float falloffStart;	// point/spot light only
	float3 direction;	// directional/spot light only
	float falloffEnd;	// point/spot light only
	float3 position;	// point light only
	float spotPower;	// spot light only
};

// 线性衰减
float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
	// saturate(value) 等价于 min(1.0f, max(0.0f, value))
	return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}


// 使用Schlick近似法对应的菲涅尔方程，求由于菲涅尔效应表面所反射的光线所占的比率
// R0 = ((n - 1) / (n + 1)) ^ 2，n为折射率
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
	float cosIncidentAngle = saturate(dot(normal, lightVec));
	float f0 = 1.0f - cosIncidentAngle;
	float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);
	return reflectPercent;
}

float BlinnPhongNDF(float NdotH, float roughness)
{
	float r2 = max(1e-4f, roughness * roughness);
	float a = (2.0 / r2) - 2.0;
	a = max(a, 1e-4f);

	float normTerm = (a + 2.0) * (0.5 / PI);	// 归一化因子
	float specTerm = pow(NdotH, a);
	return specTerm * normTerm;
}

// 计算反射到摄像机的光的总量，它是漫反射和镜面反射的总和。
float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toCamera, Material material)
{
	float roughness = material.perceptualRoughness * material.perceptualRoughness;
	float3 halfVec = normalize(toCamera + lightVec);
	float NdotH = saturate(dot(halfVec, normal));
	float D = BlinnPhongNDF(NdotH, roughness);
	float3 F = SchlickFresnel(material.fresnelR0, halfVec, lightVec);
	float3 specAlbedo = F * D;
	return (material.albedo.rgb + specAlbedo) * lightStrength;
}

// 平行光计算
float3 ComputeDirectionalLight(Light light, Material material, float3 normal, float3 toCamera)
{
	float3 lightVec = -light.direction;

	// Lambert的余弦定率
	float ndotl = max(dot(lightVec, normal), 0.0f);
	float3 lightStrength = light.strength * ndotl;

	return BlinnPhong(lightStrength, lightVec, normal, toCamera, material);
}

// 点光源计算
float3 ComputePointLight(Light light, Material material, float3 position, float3 normal, float3 toCamera)
{
	float3 lightVec = light.position - position;
	// 表面到点光源的距离
	float d = length(lightVec);
	// 距离超过falloffEnd，则没有光照
	if (d > light.falloffEnd)
		return 0.0f;
	// 归一化lightVec
	lightVec /= d;
	
	// Lambert的余弦定率
	float ndotl = max(dot(lightVec, normal), 0.0f);
	float3 lightStrength = light.strength * ndotl;

	// 根据距离计算光照强度的衰减
	float att = CalcAttenuation(d, light.falloffStart, light.falloffEnd);
	lightStrength *= att;

	return BlinnPhong(lightStrength, lightVec, normal, toCamera, material);
}

// 聚光灯计算
float3 ComputeSpotLight(Light light, Material material, float3 position, float3 normal, float3 toCamera)
{
	// 同点光源计算
	float3 lightVec = light.position - position;
	float d = length(lightVec);
	if (d > light.falloffEnd)
		return 0.0f;
	lightVec /= d;
	float ndotl = max(dot(lightVec, normal), 0.0f);
	float3 lightStrength = light.strength * ndotl;
	float att = CalcAttenuation(d, light.falloffStart, light.falloffEnd);
	lightStrength *= att;

	// 根据与光照方向的夹角，计算光照强度的衰减
	float spotFactor = pow(max(dot(-lightVec, light.direction), 0.0f), light.spotPower);
	lightStrength *= spotFactor;

	return BlinnPhong(lightStrength, lightVec, normal, toCamera, material);
}

float4 ComputeLighting(Light lights[MaxLights], Material material, float3 position, float3 normal, float3 toCamera)
{
	float3 result = 0.0f;
	int i = 0;
#if (NUM_DIRECT_LIGHTS > 0)
	for (i = 0; i < NUM_DIRECT_LIGHTS; ++i)
		result += ComputeDirectionalLight(lights[i], material, normal, toCamera);
#endif
#if (NUM_POINT_LIGHTS > 0)
	for (i = NUM_DIRECT_LIGHTS; i < NUM_DIRECT_LIGHTS + NUM_POINT_LIGHTS; ++i)
		result += ComputePointLight(lights[i], material, position, normal, toCamera);
#endif
#if (NUM_SPOT_LIGHTS > 0)
	for (i = NUM_DIRECT_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIRECT_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
		result += ComputeSpotLight(lights[i], material, position, normal, toCamera);
#endif
	return float4(result, 0.0f);
}
