struct VSOutput
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

// VSOutput VSShadePixels(uint vertexId : SV_VertexID)
// {
// 	VSOutput output;
// 	float2 texCoord = float2((vertexID << 1) & 2, vertexID & 2);
// 	output.position = float4(texCoord * float2(2, -2) + float2(-1, 1), 0, 1);
// 	output.texCoord = texCoord;

// 	return output;
// }
// I forgot I dont have access to albedo, emissive, metallic, etc textures of the entire scene,
// and hence can't light the scene now

/*The thing is in the previous raster pass I colored the pixel as finalColor = ambient * albedo + emissive. That's the issue
 * I did the multiplication, it would have worked if there was no multiplication involved. also if there was no multiplication
 * the fact that the lightcolor calculations here will involve performing multiplying:	lighting += albedo * light.Color * light.Intensity * NdotL * attenuation;
 * The emissive added in the previous pass will multiply here and mess up the scene
 * without knowing the values I cant undo it so I have two choices now
 */

// 1. create multiple render targets and make it a deferred renderer altogether
// 2. Perform the lighting calculations in raster pass itself.
// // 3. THE BEST OPTION: Sample the emissive texture here instead and use previous render target values, and apply the lighting calculations on the top of it
// //    The scene already has albedo, perform ligting cals, return the finalcolor (which I hope multiplies on top of the previous Render target
// //    values. and then do the emissive addition

// The hope with third lies in the fact that the render target previous values will multiply with lighting calcs. I dont think it will so
// //<<OPTION 2>>> it is