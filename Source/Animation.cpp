//
// Animation.cpp
//
// Clark Kromenaker
//
#include "Animation.h"

#include "AnimationNodes.h"
#include "IniParser.h"
#include "Services.h"
#include "StringUtil.h"

Animation::Animation(std::string name, char* data, int dataLength) : Asset(name)
{
    ParseFromData(data, dataLength);
}

Animation::~Animation()
{
	
}

std::vector<AnimNode*>* Animation::GetFrame(int num)
{
    if(mFrames.find(num) != mFrames.end())
    {
        return &mFrames[num];
    }
    return nullptr;
}

void Animation::ParseFromData(char *data, int dataLength)
{
    IniParser parser(data, dataLength);
    IniSection section;
    while(parser.ReadNextSection(section))
    {
        // Header section has only one value: number of frames.
        if(StringUtil::EqualsIgnoreCase(section.name, "HEADER"))
        {
			if(section.lines.size() > 0)
            {
                mFrameCount = section.lines[0].entries.front().GetValueAsInt();
            }
        }
		// Actions section contains vertex animations to start playing on certain frames.
        else if(StringUtil::EqualsIgnoreCase(section.name, "ACTIONS"))
        {
			// First line is number of action entries...but we can just determine that from the number of lines!
			
			// Read in 2+ lines as actions.
            for(int i = 1; i < section.lines.size(); ++i)
            {
				IniLine& line = section.lines[i];
				
                // Each line has up to 10 (!!!) fields. Most are optional, but they must be in a certain order.
				
				// Frame number must be specified.
                // <frame_num>, <act_name>, <x1>, <y1>, <z1>, <angle1>, <x2>, <y2>, <z2>, <angle2>
                int frameNumber = line.entries[0].GetValueAsInt();
                
				// Vertex animation must be specified.
				VertexAnimation* vertexAnim = Services::GetAssets()->LoadVertexAnimation(line.entries[1].key);
				
				// Create and push back the animation node. Remaining fields are optional.
                VertexAnimNode* node = new VertexAnimNode();
                node->frameNumber = frameNumber;
				node->vertexAnimation = vertexAnim;
                mFrames[frameNumber].push_back(node);
				
				// See if there are enough args for the (x1, y1, z1) and (angle1) values.
				if(line.entries.size() < 6) { continue; }
				
				// If an animation defines more entries, it means it is an absolute animation.
				node->absolute = true;
				
				// Note z/y are flipped due to Maya->Game conversion.
				node->offsetFromOrigin.SetX(line.entries[2].GetValueAsFloat());
				node->offsetFromOrigin.SetZ(line.entries[3].GetValueAsFloat());
				node->offsetFromOrigin.SetY(line.entries[4].GetValueAsFloat());
				node->headingFromOrigin = line.entries[5].GetValueAsFloat();
				
				// Next are (x2, z2, y2) and (angle2), if we have enough args.
				// Note z/y are flipped, due to Maya->Game conversion.
				// Based on examining anim files (like RC1 fountain) and expected positions,
				// this appears to be a position to move the model to at the start of the animation?
				if(line.entries.size() < 10) { continue; }
				node->position.SetX(line.entries[6].GetValueAsFloat());
				node->position.SetZ(line.entries[7].GetValueAsFloat());
				node->position.SetY(line.entries[8].GetValueAsFloat());
				node->heading = line.entries[9].GetValueAsFloat();
            }
        }
		// "STextures" changes textures on a scene (BSP) model.
        else if(StringUtil::EqualsIgnoreCase(section.name, "STEXTURES"))
        {
			// First line is number of entries...but we can just determine that from the number of lines!
			
			for(int i = 1; i < section.lines.size(); ++i)
            {
				IniLine& line = section.lines[i];
				
                // <frame_num>, <scn_name>, <scn_model_name>, <texture_name>
                int frameNumber = line.entries[0].GetValueAsInt();
                
                // Read the scene name.
                std::string sceneName = line.entries[1].key;
                
                // Read the scene model name.
                std::string sceneModelName = line.entries[2].key;
                
                // Read the texture name.
				std::string textureName = line.entries[3].key;
				
				// Create and add the anim node.
				SceneTextureAnimNode* node = new SceneTextureAnimNode();
				node->frameNumber = frameNumber;
				node->sceneName = sceneName;
				node->sceneModelName = sceneModelName;
				node->textureName = textureName;
                mFrames[frameNumber].push_back(node);
            }
        }
		// "SVisibility" changes the visibility of a scene (BSP) model.
        else if(StringUtil::EqualsIgnoreCase(section.name, "SVISIBILITY"))
        {
			// First line is number of entries...but we can just determine that from the number of lines!
			
            for(int i = 1; i < section.lines.size(); i++)
            {
                IniLine& line = section.lines[i];
                
                // <frame_num>, <scn_name>, <scn_model_name>, <on/off>
                int frameNumber = line.entries[0].GetValueAsInt();
                
                // Read the scene name.
                std::string sceneName = line.entries[1].key;
                
                // Read the scene model name.
                std::string sceneModelName = line.entries[2].key;
                
                // Read the on/off value.
                bool visible = line.entries[3].GetValueAsBool();
				
				// Create and add the anim node.
				SceneModelVisibilityAnimNode* node = new SceneModelVisibilityAnimNode();
				node->sceneName = sceneName;
				node->sceneModelName = sceneModelName;
				node->visible = visible;
                mFrames[frameNumber].push_back(node);
            }
        }
		// "MTextures" changes textures on a model or actor.
        else if(StringUtil::EqualsIgnoreCase(section.name, "MTEXTURES"))
        {
			// First line is number of entries...but we can just determine that from the number of lines!
			
            for(int i = 1; i < section.lines.size(); i++)
            {
                IniLine& line = section.lines[i];
                
                // <frame_num>, <model_name>, <mesh_index>, <group_index>, <texture_name>
                int frameNumber = line.entries[0].GetValueAsInt();
                
                // Read the model name.
                std::string modelName = line.entries[1].key;
                
                // Read the model mesh index.
                int meshIndex = line.entries[2].GetValueAsInt();
                
                // Read the model mesh group index.
                int submeshIndex = line.entries[3].GetValueAsInt();
                
                // Read the texture name.
                std::string textureName = line.entries[4].key;
				
				// Create and add node.
				ModelTextureAnimNode* node = new ModelTextureAnimNode();
				node->modelName = modelName;
				node->meshIndex = static_cast<unsigned char>(meshIndex);
				node->submeshIndex = static_cast<unsigned char>(submeshIndex);
				node->textureName = textureName;
				mFrames[frameNumber].push_back(node);
            }
        }
		// "MVisibility" changes visibility on a model or actor.
        else if(StringUtil::EqualsIgnoreCase(section.name, "MVISIBILITY"))
        {
			// First line is number of entries...but we can just determine that from the number of lines!
			
            for(int i = 1; i < section.lines.size(); i++)
            {
                IniLine& line = section.lines[i];
                
                // <frame_num>, <model_name>, <on/off>
                int frameNumber = line.entries[0].GetValueAsInt();
                
                // Read the model name.
                std::string modelName = line.entries[1].key;
                
                // Read the on/off value.
                bool visible = line.entries[2].GetValueAsBool();
				
				// Create and add node.
				ModelVisibilityAnimNode* node = new ModelVisibilityAnimNode();
				node->modelName = modelName;
				node->visible = visible;
                mFrames[frameNumber].push_back(node);
            }
        }
		// Triggers sounds to play on certain frames at certain locations.
        else if(StringUtil::EqualsIgnoreCase(section.name, "SOUNDS"))
        {
			// First line is number of entries...but we can just determine that from the number of lines!
			
            for(int i = 1; i < section.lines.size(); i++)
            {
                IniLine& line = section.lines[i];
                
				// Two possible versions of these lines:
                // <frame_num>, <sound_name>, <volume>, <model_name>, <min_dist>, <max_dist>
                // <frame_num>, <sound_name>, <volume>, <x1>, <y1>, <z1>, <min_dist>, <max_dist>
				unsigned int paramCount = static_cast<int>(line.entries.size());
				if(paramCount != 6 && paramCount != 8)
				{
					std::cout << "Invalid param count of " << paramCount << " for animation asset " << GetName() << std::endl;
					continue;
				}
                
                // Read frame number.
                int frameNumber = line.entries[0].GetValueAsInt();
                
                // Read the sound name.
                std::string soundName = line.entries[1].key;
                
                // Read the volume.
                int volume = line.entries[2].GetValueAsInt();
				
				// Create node here - remaining entries are optional.
				SoundAnimNode* node = new SoundAnimNode();
				node->frameNumber = frameNumber;
				node->audio = Services::GetAssets()->LoadAudio(soundName);
				node->volume = volume;
				mFrames[frameNumber].push_back(node);
				
                // If there are 6 parameters, next up is the name of the model that plays the sound.
                // If there are 8 parameters, next up are a sound position (x,y,z).
                if(paramCount == 6)
                {
					node->modelName = line.entries[3].key;
                }
                else
                {
					//TODO: Do the z/y values need to be flipped here as well???
                    int x = line.entries[3].GetValueAsInt();
                    int y = line.entries[4].GetValueAsInt();
                    int z = line.entries[5].GetValueAsInt();
					node->position = Vector3(x, y, z);
                }
                
                // Read in min/max distance for sound.
				int distIndex = paramCount == 6 ? 4 : 6;
                node->minDistance = line.entries[distIndex].GetValueAsInt();
                node->maxDistance = line.entries[distIndex + 1].GetValueAsInt();
            }
        }
		// Allows specifying of additional options that affect the entire animation.
        else if(StringUtil::EqualsIgnoreCase(section.name, "OPTIONS"))
        {
			// First line is number of entries...but we can just determine that from the number of lines!
			
            for(int i = 1; i < section.lines.size(); i++)
            {
				IniLine& line = section.lines[i];
				
                // <frame_num>, <option>, <value>
                
                // Read frame number.
                //int frameNumber = line.entries[0].GetValueAsInt();
                
                // Read in the option.
				std::string option = line.entries[0].key;
                if(StringUtil::EqualsIgnoreCase(option, "SIMPLE"))
                {
                    //int simpleValue = entry->GetValueAsInt();
                }
                else if(StringUtil::EqualsIgnoreCase(option, "NOINTERPOLATE"))
                {
                    
                }
				// Allows us to dictate the rate at which the animation proceeds.
                else if(StringUtil::EqualsIgnoreCase(option, "FRAMERATE"))
                {
                    //mFramesPerSecond = entry->GetValueAsInt();
                }
                else
                {
                    std::cout << "Unexpected option: " << option << std::endl;
                }
            }
        }
		// Contains some options specifically for GK3. I imagine this engine
		// was initially meant to be reused on future projects, so they tried
		// to isolate extremely specific to GK3 stuff here?
        else if(StringUtil::EqualsIgnoreCase(section.name, "GK3"))
        {
            for(int i = 1; i < section.lines.size(); i++)
            {
                IniLine& line = section.lines[i];
                
                // Read frame number.
                int frameNumber = line.entries[0].GetValueAsInt();
                
                // Read in the option.
				std::string keyword = line.entries[1].key;
                if(StringUtil::EqualsIgnoreCase(keyword, "FOOTSTEP"))
                {
                    std::string actorNoun = line.entries[2].key;
					
					// Create and add node.
					FootstepAnimNode* node = new FootstepAnimNode();
					node->actorNoun = actorNoun;
					mFrames[frameNumber].push_back(node);
                }
                else if(StringUtil::EqualsIgnoreCase(keyword, "FOOTSCUFF"))
                {
                    std::string actorNoun = line.entries[2].key;
					
					// Create and add node.
					FootscuffAnimNode* node = new FootscuffAnimNode();
					node->actorNoun = actorNoun;
					mFrames[frameNumber].push_back(node);
                }
                else if(StringUtil::EqualsIgnoreCase(keyword, "STOPSOUNDTRACK"))
                {
                    std::string soundtrackName = line.entries[2].key;
					
					// Create and add node.
					StopSoundtrackAnimNode* node = new StopSoundtrackAnimNode();
					node->soundtrackName = soundtrackName;
					mFrames[frameNumber].push_back(node);
                }
                else if(StringUtil::EqualsIgnoreCase(keyword, "PLAYSOUNDTRACK"))
                {
                    std::string soundtrackName = line.entries[2].key;
					
					// Create and add node.
					PlaySoundtrackAnimNode* node = new PlaySoundtrackAnimNode();
					node->soundtrackName = soundtrackName;
					mFrames[frameNumber].push_back(node);
                }
                else if(StringUtil::EqualsIgnoreCase(keyword, "PLAYSOUNDTRACKTBS"))
                {
                    std::string soundtrackName = line.entries[2].key;
					
					// Create and add node.
					PlaySoundtrackAnimNode* node = new PlaySoundtrackAnimNode();
					node->soundtrackName = soundtrackName;
					mFrames[frameNumber].push_back(node);
                }
                else if(StringUtil::EqualsIgnoreCase(keyword, "STOPALLSOUNDTRACKS"))
                {
					// Create and add node.
					mFrames[frameNumber].push_back(new StopSoundtrackAnimNode());
                }
                else if(StringUtil::EqualsIgnoreCase(keyword, "CAMERA"))
                {
                    std::string cameraPositionName = line.entries[2].key;
					
					// Create and add node.
					CameraAnimNode* node = new CameraAnimNode();
					node->cameraPositionName = cameraPositionName;
					mFrames[frameNumber].push_back(node);
                }
                else if(StringUtil::EqualsIgnoreCase(keyword, "LIPSYNCH"))
                {
                    std::string actorNoun = line.entries[2].key;
                    std::string mouthTexName = line.entries[3].key;
					
					// Create and add node.
					LipSyncAnimNode* node = new LipSyncAnimNode();
					node->actorNoun = actorNoun;
					node->mouthTextureName = mouthTexName;
					mFrames[frameNumber].push_back(node);
                }
                else if(StringUtil::EqualsIgnoreCase(keyword, "FACETEX"))
                {
                    // Read the actor name.
                    std::string actorNoun = line.entries[2].key;
                    
                    // Read texture name.
                    // This sometimes has a forward slash in it, which
                    // indicates "tex"/"alpha_tex".
					std::string textureName = line.entries[3].key;
                    
                    // A value indicating what part of the face is changed. Always H, E, M.
					// M = mouth
					// E = eye
					// H = head/forehead
					std::string facePart = line.entries[4].key;
					StringUtil::ToLower(facePart);
					FaceElement faceElement = FaceElement::Mouth;
					switch(facePart[0])
					{
						case 'm':
							faceElement = FaceElement::Mouth;
							break;
						case 'e':
							faceElement = FaceElement::Eyelids;
							break;
						case 'h':
							faceElement = FaceElement::Forehead;
							break;
					}
					
					// Create and add node.
					FaceTexAnimNode* node = new FaceTexAnimNode();
					node->actorNoun = actorNoun;
					node->textureName = textureName;
					node->faceElement = faceElement;
					mFrames[frameNumber].push_back(node);
                }
                else if(StringUtil::EqualsIgnoreCase(keyword, "UNFACETEX"))
                {
                    // Read the actor name.
                    std::string actorNoun = line.entries[2].key;
					
					// A value indicating what part of the face is changed. Always H, E, M.
					// M = mouth
					// E = eye
					// H = head/forehead
					std::string facePart = line.entries[3].key;
					StringUtil::ToLower(facePart);
					FaceElement faceElement = FaceElement::Mouth;
					switch(facePart[0])
					{
						case 'm':
							faceElement = FaceElement::Mouth;
							break;
						case 'e':
							faceElement = FaceElement::Eyelids;
							break;
						case 'h':
							faceElement = FaceElement::Forehead;
							break;
					}
					
					// Create and add node.
					UnFaceTexAnimNode* node = new UnFaceTexAnimNode();
					node->actorNoun = actorNoun;
					node->faceElement = faceElement;
					mFrames[frameNumber].push_back(node);
                }
                else if(StringUtil::EqualsIgnoreCase(keyword, "GLANCE"))
                {
                    // Read the actor name.
                    std::string actorNoun = line.entries[2].key;
                    
                    // X/Y/Z position. //TODO: Do z/y also need to be flipped?
                    int x = line.entries[3].GetValueAsInt();
					int y = line.entries[4].GetValueAsInt();
                    int z = line.entries[5].GetValueAsInt();
					
					// Create and add node.
					GlanceAnimNode* node = new GlanceAnimNode();
					node->actorNoun = actorNoun;
					node->position = Vector3(x, y, z);
					mFrames[frameNumber].push_back(node);
                }
				else if(StringUtil::EqualsIgnoreCase(keyword, "MOOD"))
				{
					// Read the actor name.
					std::string actorNoun = line.entries[2].key;
					
					// Read the mood name.
					std::string moodName = line.entries[3].key;
					
					// Create and add node.
					MoodAnimNode* node = new MoodAnimNode();
					node->actorNoun = actorNoun;
					node->moodName = moodName;
					mFrames[frameNumber].push_back(node);
				}
                else
                {
                    std::cout << "Unexpected GK3 animation keyword: " << keyword << std::endl;
                }
            }
        }
        else
        {
            std::cout << "Unexpected animation header: " << section.name << std::endl;
        }
    }
}
