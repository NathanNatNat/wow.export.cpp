/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
	License: MIT
 */

#include "WMOShaderMapper.h"

namespace wmo_shader_mapper {

const std::unordered_map<int, WMOShaderEntry> WMOShaderMap = {
	{Diffuse,                 {MapObjDiffuse_T1,          MapObjDiffuse}},
	{Specular,                {MapObjSpecular_T1,         MapObjSpecular}},
	{Metal,                   {MapObjSpecular_T1,         MapObjMetal}},
	{Env,                     {MapObjDiffuse_T1_Refl,     MapObjEnv}},
	{Opaque,                  {MapObjDiffuse_T1,          MapObjOpaque}},
	{EnvMetal,                {MapObjDiffuse_T1_Refl,     MapObjEnvMetal}},
	{TwoLayerDiffuse,         {MapObjDiffuse_Comp,        MapObjTwoLayerDiffuse}},
	{TwoLayerEnvMetal,        {MapObjDiffuse_T1,          MapObjTwoLayerEnvMetal}},
	{TwoLayerTerrain,         {MapObjDiffuse_Comp_Terrain, MapObjTwoLayerTerrain}},
	{DiffuseEmissive,         {MapObjDiffuse_Comp,        MapObjDiffuseEmissive}},
	{WaterWindow,             {WMOVertexShader_None,      WMOPixelShader_None}},
	{MaskedEnvMetal,          {MapObjDiffuse_T1_Env_T2,   MapObjMaskedEnvMetal}},
	{EnvMetalEmissive,        {MapObjDiffuse_T1_Env_T2,   MapObjEnvMetalEmissive}},
	{TwoLayerDiffuseOpaque,   {MapObjDiffuse_Comp,        MapObjTwoLayerDiffuseOpaque}},
	{SubmarineWindow,         {WMOVertexShader_None,      WMOPixelShader_None}},
	{TwoLayerDiffuseEmissive, {MapObjDiffuse_Comp,        MapObjTwoLayerDiffuseEmissive}},
	{DiffuseTerrain,          {MapObjDiffuse_T1,          MapObjDiffuse}},
	{AdditiveMaskedEnvMetal,  {MapObjDiffuse_T1_Env_T2,   MapObjAdditiveMaskedEnvMetal}},
	{TwoLayerDiffuseMod2x,    {MapObjDiffuse_CompAlpha,   MapObjTwoLayerDiffuseMod2x}},
	{TwoLayerDiffuseMod2xNA,  {MapObjDiffuse_Comp,        MapObjTwoLayerDiffuseMod2xNA}},
	{TwoLayerDiffuseAlpha,    {MapObjDiffuse_CompAlpha,   MapObjTwoLayerDiffuseAlpha}},
	{Lod,                     {MapObjDiffuse_T1,          MapObjLod}},
	{Parallax,                {MapObjParallax,            MapObjParallax_PS}},
	{DF_MoreTexture_Unknown,  {MapObjDiffuse_T1,          MapObjDFShader}}
};

} // namespace wmo_shader_mapper
