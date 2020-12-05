//
// VertexAnimation.h
//
// Clark Kromenaker
//
// A vertex animation for a particular model/mesh. For GK3, this is the in-memory representation of "ACT" file.
//
// For an animation to work, its vertex data must align with that of a particular model/mesh.
// This includes mesh count, submesh count, vertex count within submeshes, etc.
//
#pragma once
#include "Asset.h"

#include <vector>
#include <unordered_map>

#include "Matrix4.h"
#include "Vector3.h"

struct VertexAnimationVertexPose
{
    int mFrameNumber = 0;
    
    std::vector<Vector3> mVertexPositions;
    VertexAnimationVertexPose* mNext = nullptr;
};

struct VertexAnimationTransformPose
{
    int mFrameNumber = 0;
    
    Quaternion mLocalRotation;
    Vector3 mLocalPosition;
	Vector3 mLocalScale;
	
    VertexAnimationTransformPose* mNext = nullptr;
    
    Matrix4 GetMeshToLocalMatrix()
    {
        return Matrix4::MakeTranslate(mLocalPosition)
		* Matrix4::MakeRotate(mLocalRotation)
		* Matrix4::MakeScale(mLocalScale);
    }
};

class VertexAnimation : public Asset
{
public:
    VertexAnimation(std::string name, char* data, int dataLength);
    
	// Queries the position of a single vertex at a particular time of the animation.
	Vector3 SampleVertexPosition(float time, int framesPerSecond, int meshIndex, int submeshIndex, int vertexIndex);
	
	// Queries positions of ALL vertices for a submesh at a particular time of the animation.
	VertexAnimationVertexPose SampleVertexPose(float time, int framesPerSecond, int meshIndex, int submeshIndex);
	
	// Queries a mesh's transform properties (position, rotation, scale) at a particular time of the animation.
	VertexAnimationTransformPose SampleTransformPose(float time, int framesPerSecond, int meshIndex);
    
	// Length and duration.
	int GetFrameCount() const { return mFrameCount; }
    float GetDuration(int framesPerSecond) const { return (float)mFrameCount / (float)framesPerSecond; }
	
	const std::string& GetModelName() const { return mModelName; }
	
private:
    // The number of frames in this animation.
    int mFrameCount = 0;
	
	// The name of the model that is meant to play this animation.
	// If we ever play the animation on a mismatched model, the graphics will probably glitch out.
	std::string mModelName;
    
	// Each array element is the FIRST vertex pose for each mesh/submesh index.
	// Subsequent poses for the mesh are stored in the "next" of the first pose.
	std::unordered_map<int, std::unordered_map<int, VertexAnimationVertexPose*>> mVertexPoses;
	
	// Each element of array is the FIRST transform poses for each mesh index.
	// Subsequent poses for the mesh are stored in the "next" of the first pose.
    std::vector<VertexAnimationTransformPose*> mTransformPoses;
    
    void ParseFromData(char* data, int dataLength);
    
    float DecompressFloatFromByte(unsigned char val);
    float DecompressFloatFromUShort(unsigned short val);
};
