//
// GAS.cpp
//
// Clark Kromenaker
//
// A "G-Engine Auto Script". These are basic text-based logic trees
// that usually handle playing of animations.
//
// They are read into an in-memory format (this class) and used as input to
// a "player" or "executor" for the script.
//
#pragma once
#include "Asset.h"

#include <vector>

#include "Vector3.h"

class Animation;
class GasPlayer;

struct GasNode
{
    virtual int Execute(GasPlayer* player) = 0;
    
    // Chance that this node will execute. 0 = no chance, 100 = definitely
    int random = 100;
};

struct AnimGasNode : public GasNode
{
    int Execute(GasPlayer* player) override;
    
    // Animation to do.
    Animation* animation = nullptr;
    
    // If true, relative animations can move the affected model.
    bool moving = false;
};

struct OneOfGasNode : public GasNode
{
    int Execute(GasPlayer* player) override;
    
    std::vector<AnimGasNode*> animNodes;
};

struct WaitGasNode : public GasNode
{
    int Execute(GasPlayer* player) override;
    
    int minWaitTimeSeconds = 0;
    int maxWaitTimeSeconds = 0;
};

struct LabelOrGotoGasNode : public GasNode
{
    int Execute(GasPlayer* player) override { return 0; }
    
    std::string label;
    
    // Indicates if this is a go-to or not.
    // "Loop" is implemented as a go-to with no label.
    bool isGoto = false;
};

struct SetGasNode : public GasNode
{
    char varName = 'A';
    int value = 0;
};

struct IncDecGasNode : public GasNode
{
    char varName = 'A';
    int value = 0;
};

struct IfGasNode : public GasNode
{
    char ifVar = 'A';
    char ifOp = '=';
    int ifValue = 0;
    std::string goToLabel;
};

struct WalkToGasNode : public GasNode
{
    // Can specify a position name OR an X/Y/Z pos.
    // Name takes precendent (used if not empty).
    std::string positionName;
    Vector3 position;
};

struct ChooseWalkGasNode : public GasNode
{
    // Randomly chooses one of these to walk to.
    std::vector<WalkToGasNode*> walkToNodes;
};

struct UseIPosGasNode : public GasNode
{
    std::string positionName;
};

class GAS : public Asset
{
public:
    GAS(std::string name, char* data, int dataLength);
    
    GasNode* GetNode(int index) { return mNodes[index]; }
    int GetNodeCount() { return (int)mNodes.size(); }
    
private:
    std::vector<GasNode*> mNodes;
    
    void ParseFromData(char* data, int dataLength);
};
