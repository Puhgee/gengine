//
// Material.cpp
//
// Clark Kromenaker
//
#include "Material.h"

#include "Matrix3.h"
#include "Matrix4.h"
#include "Shader.h"
#include "Texture.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

// This is set to a default shader during Renderer init.
Shader* Material::sDefaultShader = nullptr;

Matrix4 Material::sCurrentViewMatrix;
Matrix4 Material::sCurrentProjMatrix;

float Material::sAlphaTestValue = 0.0f;

void Material::SetViewMatrix(const Matrix4& viewMatrix)
{
	sCurrentViewMatrix = viewMatrix;
}

void Material::SetProjMatrix(const Matrix4& projMatrix)
{
	sCurrentProjMatrix = projMatrix;
}

void Material::UseAlphaTest(bool use)
{
	sAlphaTestValue = use ? 0.1f : 0.0f;
}

Material::Material() : mShader(sDefaultShader)
{
    SetColor(Color32::White);
}

Material::Material(Shader* shader) : mShader(shader)
{
    SetColor(Color32::White);
}

void Material::Activate(const Matrix4& objectToWorldMatrix)
{
    // Must activate shader BEFORE setting uniforms to get correct results.
    // See https://stackoverflow.com/questions/42357380/why-must-i-use-a-shader-program-before-i-can-set-its-uniforms
    mShader->Activate();
    
	// Set built-in transform matrices.
    mShader->SetUniformMatrix4("gObjectToWorldMatrix", objectToWorldMatrix);
    mShader->SetUniformMatrix4("gViewMatrix", sCurrentViewMatrix);
    mShader->SetUniformMatrix4("gProjMatrix", sCurrentProjMatrix);
	mShader->SetUniformMatrix4("gWorldToProjMatrix", sCurrentProjMatrix * sCurrentViewMatrix);
	
	// Set built-in alpha test value.
	mShader->SetUniformFloat("gAlphaTest", sAlphaTestValue);
	
    // Set user-defined color values.
    for(auto& entry : mColors)
    {
        mShader->SetUniformColor(entry.first.c_str(), entry.second);
    }
    
    // Set user-defined textures.
    int textureUnit = 0;
    for(auto& entry : mTextures)
    {
        mShader->SetUniformInt(entry.first.c_str(), textureUnit);
        entry.second->Activate(textureUnit);
        ++textureUnit;
    }
    
	//TODO: May need to "deactivate" texture units if no texture is defined in material, but a texture sampler exists in the shader.
}

void Material::SetColor(const std::string& name, const Color32& color)
{
    mColors[name] = color;
}

void Material::SetTexture(const std::string& name, Texture* texture)
{
    mTextures[name] = texture;
}

Texture* Material::GetTexture(const std::string& name) const
{
    auto it = mTextures.find(name);
    if(it != mTextures.end())
    {
        return it->second;
    }
    return nullptr;
}

bool Material::IsTranslucent()
{
	//TODO: Maybe use render queue value for this?
	return false;
}
