//
// Scene.cpp
//
// Clark Kromenaker
//
#include "Scene.h"

#include <iostream>
#include <limits>

#include "ActionManager.h"
#include "Animator.h"
#include "BSPActor.h"
#include "CharacterManager.h"
#include "Collisions.h"
#include "Color32.h"
#include "Debug.h"
#include "GameCamera.h"
#include "GameProgress.h"
#include "GKActor.h"
#include "InventoryManager.h"
#include "LocationManager.h"
#include "GMath.h"
#include "MeshRenderer.h"
#include "RectTransform.h"
#include "Services.h"
#include "SoundtrackPlayer.h"
#include "StatusOverlay.h"
#include "StringUtil.h"
#include "Walker.h"
#include "WalkerBoundary.h"

extern Mesh* quad;

Scene::Scene(const std::string& name, const std::string& timeblock) : Scene(name, Timeblock(timeblock))
{
    
}

Scene::Scene(const std::string& name, const Timeblock& timeblock) :
	mLocation(name),
	mTimeblock(timeblock)
{
	// Create game camera.
	mCamera = new GameCamera();
	
	// Create animation player.
	Actor* animationActor = new Actor();
	mAnimator = animationActor->AddComponent<Animator>();
}

Scene::~Scene()
{
	if(mSceneData != nullptr)
	{
		Unload();
	}
}

void Scene::Load()
{
	// If this is true, we are calling load when scene is already loaded!
	if(mSceneData != nullptr)
	{
		//TODO: Ignore for now, but maybe we should Unload and then re-Load?
		return;
	}
	
	// Creating scene data loads SIFs, but does nothing else yet!
	mSceneData = new SceneData(mLocation, mTimeblock.ToString());
	
	// It's generally important that we know how our "ego" will be as soon as possible.
	// This is because the scene loading *itself* may check who ego is to do certain things!
	const SceneActor* egoSceneActor = mSceneData->DetermineWhoEgoWillBe();
	//TODO: If no ego, I guess we fail loading!?
	if(egoSceneActor == nullptr)
	{
		std::cout << "No ego actor could be predicted for scene!" << std::endl;
	}
	else
	{
		mEgoName = egoSceneActor->noun;
	}
	
	// Set location.
	Services::Get<LocationManager>()->SetLocation(mLocation);
	
	// Increment location counter IMMEDIATELY.
	// We know this b/c various scripts that need to run on "1st time enter" or similar check if count==1.
	// For those to evaluate correctly, we need to do this BEFORE we even parse scene data or anything.
	Services::Get<LocationManager>()->IncLocationCount(mEgoName, mLocation, mTimeblock);
    
    // Create status overlay actor. Do this after setting location for accurate location!
    new StatusOverlay();
	
	// Based on location, timeblock, and game progress, resolve what data we will load into the current scene.
	// After calling this, SceneData will have interpreted all data from SIFs and determined exactly what we should and should not load/use for the scene right now.
	mSceneData->ResolveSceneData();
	
	// Set BSP to be rendered.
    Services::GetRenderer()->SetBSP(mSceneData->GetBSP());
    
    // Figure out if we have a skybox, and set it to be rendered.
    Services::GetRenderer()->SetSkybox(mSceneData->GetSkybox());
	
	// Position the camera at the the default position and heading.
	const SceneCamera* defaultRoomCamera = mSceneData->GetDefaultRoomCamera();
	if(defaultRoomCamera != nullptr)
	{
    	mCamera->SetPosition(defaultRoomCamera->position);
    	mCamera->SetRotation(Quaternion(Vector3::UnitY, defaultRoomCamera->angle.x));
	}
	
	// If a camera bounds model exists for this scene, pass it along to the camera.
	Model* cameraBoundsModel = Services::GetAssets()->LoadModel(mSceneData->GetCameraBoundsModelName());
	if(cameraBoundsModel != nullptr)
	{
		mCamera->SetBounds(cameraBoundsModel);
		
		// For debugging - we can visualize the camera bounds mesh, if desired.
		GKActor* cameraBoundsActor = new GKActor();
		MeshRenderer* cameraBoundsMeshRenderer = cameraBoundsActor->GetMeshRenderer();
		cameraBoundsMeshRenderer->SetModel(cameraBoundsModel);
		cameraBoundsMeshRenderer->SetEnabled(false);
		//cameraBoundsMeshRenderer->DebugDrawAABBs();
	}
	
	// Create soundtrack player and get it playing!
	Soundtrack* soundtrack = mSceneData->GetSoundtrack();
	if(soundtrack != nullptr)
	{
		Actor* actor = new Actor();
		mSoundtrackPlayer = actor->AddComponent<SoundtrackPlayer>();
		mSoundtrackPlayer->Play(soundtrack);
	}
	
	// For debugging - render walker bounds overlay on game world.
	//TODO: Move to construction system!
	/*
	{
		WalkerBoundary* walkerBoundary = mSceneData->GetWalkerBoundary();
		if(walkerBoundary != nullptr)
		{
			Actor* walkerBoundaryActor = new Actor();
			
			MeshRenderer* walkerBoundaryMeshRenderer = walkerBoundaryActor->AddComponent<MeshRenderer>();
			walkerBoundaryMeshRenderer->SetMesh(quad);
			
			Material m;
			m.SetDiffuseTexture(walkerBoundary->GetTexture());
			walkerBoundaryMeshRenderer->SetMaterial(0, m);
			
			Vector3 size = walkerBoundary->GetSize();
			Vector3 offset = walkerBoundary->GetOffset();
			offset.x = -offset.x + size.x * 0.5f;
			offset.z = -offset.y + size.y * 0.5f;
			offset.y = 0.1f; // Offset slightly up to avoid z-fighting with floor (in most scenes).
			
			walkerBoundaryActor->SetPosition(offset);
			walkerBoundaryActor->SetRotation(Quaternion(Vector3::UnitX, Math::kPiOver2));
			walkerBoundaryActor->SetScale(size);
		}
	}
	*/
	
	// Create actors for the scene.
	const std::vector<const SceneActor*>& sceneActorDatas = mSceneData->GetActors();
	for(auto& actorDef : sceneActorDatas)
	{
		// NEVER spawn an ego who is not our current ego!
		if(actorDef->ego && actorDef != egoSceneActor) { continue; }
		
		// The actor's 3-letter identifier (GAB, GRA, etc) can be derived from the name of the model.
		std::string identifier;
		if(actorDef->model != nullptr)
		{
			identifier = actorDef->model->GetNameNoExtension();
		}
		
		// Create actor.
		GKActor* actor = new GKActor(identifier);
		mActors.push_back(actor);
		mObjects.push_back(actor);
		
		// Set noun (GABRIEL, GRACE, etc).
		actor->SetNoun(actorDef->noun);
		
		// Set actor's initial position and rotation.
		if(!actorDef->positionName.empty())
		{
			const ScenePosition* scenePos = mSceneData->GetScenePosition(actorDef->positionName);
			if(scenePos != nullptr)
			{
				actor->SetPosition(scenePos->position);
				actor->SetHeading(scenePos->heading);
			}
			else
			{
				std::cout << "Invalid position for actor: " << actorDef->positionName << std::endl;
			}
		}
		
		// Put to floor right away.
		actor->SnapToFloor();
		
		// Set actor's graphical appearance.
		actor->GetMeshRenderer()->SetModel(actorDef->model);
		
		// Save actor's GAS references.
		actor->SetIdleFidget(actorDef->idleGas);
		actor->SetTalkFidget(actorDef->talkGas);
		actor->SetListenFidget(actorDef->listenGas);
		
		// Start in idle state.
		actor->StartFidget(GKActor::FidgetType::Idle);
		
		//TODO: Apply init anim.
		
		//TODO: If hidden, hide.
		
		// If this is our ego, save a reference to it.
		if(actorDef->ego && actorDef == egoSceneActor)
		{
			mEgo = actor;
		}
	}
	
	// Iterate over scene model data and prep the scene.
	// First, we want to hide and scene models that are set to "hidden".
	// Second, we want to spawn any non-scene models.
	const std::vector<const SceneModel*> sceneModelDatas = mSceneData->GetModels();
	for(auto& modelDef : sceneModelDatas)
	{
		switch(modelDef->type)
		{
			// "Scene" type models are ones that are baked into the BSP geometry.
			case SceneModel::Type::Scene:
			{
				BSPActor* actor = mSceneData->GetBSP()->CreateBSPActor(modelDef->name);
				mBSPActors.push_back(actor);
				
				actor->SetNoun(modelDef->noun);
				actor->SetVerb(modelDef->verb);
				
				// If it should be hidden by default, tell the BSP to hide it.
				if(modelDef->hidden)
				{
					actor->SetActive(false);
					//mSceneData->GetBSP()->SetVisible(modelDef->name, false);
				}
				break;
			}
				
			// "HitTest" type models should be hidden, but still interactive.
			case SceneModel::Type::HitTest:
			{
				BSPActor* actor = mSceneData->GetBSP()->CreateBSPActor(modelDef->name);
				mBSPActors.push_back(actor);
				
				actor->SetNoun(modelDef->noun);
				actor->SetVerb(modelDef->verb);
				
				// Hit tests, if hidden, are completely deactivated.
				// However, if not hidden, they are still not visible, but they DO receive ray casts.
				if(modelDef->hidden)
				{
					actor->SetActive(false);
				}
				else
				{
					actor->SetVisible(false);
				}
				
				//std::cout << "Hide " << modelDef->name << std::endl;
				//mSceneData->GetBSP()->SetVisible(modelDef->name, false);
				break;
			}
				
			// "Prop" and "GasProp" models both render their own model geometry.
			// Only difference for a "GasProp" is that it uses a provided Gas file too.
			case SceneModel::Type::Prop:
			case SceneModel::Type::GasProp:
			{
				// Create actor.
				GKActor* prop = new GKActor();
				prop->SetNoun(modelDef->noun);
				
				// Set model.
				prop->GetMeshRenderer()->SetModel(modelDef->model);
				mProps.push_back(prop);
				mObjects.push_back(prop);
				
				// If it's a "gas prop", use provided gas as the fidget for the actor.
				if(modelDef->type == SceneModel::Type::GasProp)
				{
					prop->SetIdleFidget(modelDef->gas);
				}
				break;
			}
				
			default:
				std::cout << "Unaccounted for model type: " << (int)modelDef->type << std::endl;
				break;
		}
	}
	
	// After all models have been created, run through and execute init anims.
	// Want to wait until after creating all actors, in case init anims need to touch created actors!
	for(auto& modelDef : sceneModelDatas)
	{
		// Run any init anims specified.
		// These are usually needed to correctly position the model.
		if((modelDef->type == SceneModel::Type::Prop || modelDef->type == SceneModel::Type::GasProp)
		   && modelDef->initAnim != nullptr)
		{
			mAnimator->Sample(modelDef->initAnim, 0);
		}
	}
	
	// Check for and run "scene enter" actions.
	Services::Get<ActionManager>()->ExecuteAction("SCENE", "ENTER");
}

void Scene::Unload()
{
	Services::GetRenderer()->SetBSP(nullptr);
	Services::GetRenderer()->SetSkybox(nullptr);
	
	delete mSceneData;
	mSceneData = nullptr;
}

bool Scene::InitEgoPosition(const std::string& positionName)
{
	if(mEgo == nullptr) { return false; }
    
	// Get position.
    const ScenePosition* position = GetPosition(positionName);
	if(position == nullptr) { return false; }
    
    // Set position and heading.
    mEgo->SetPosition(position->position);
    mEgo->SetHeading(position->heading);
	
	// Make sure Ego stays grounded.
	mEgo->SnapToFloor();
	
	// Should also set camera position/angle.
	// Output a warning if specified position has no camera though.
	if(position->cameraName.empty())
	{
		Services::GetReports()->Log("Warning", "No camera information is supplied in position '" + positionName + "'.");
		return true;
	}
	
	// Move the camera to desired position/angle.
	SetCameraPosition(position->cameraName);
	return true;
}

void Scene::SetCameraPosition(const std::string& cameraName)
{
	// Find camera or fail. Any *named* camera type is valid.
	const SceneCamera* camera = mSceneData->GetRoomCamera(cameraName);
	if(camera == nullptr)
	{
		camera = mSceneData->GetCinematicCamera(cameraName);
		if(camera == nullptr)
		{
			camera = mSceneData->GetDialogueCamera(cameraName);
		}
	}
	
	// If couldn't find a camera with this name, error out!
	if(camera == nullptr)
	{
		Services::GetReports()->Log("Error", "Error: '" + cameraName + "' is not a valid room camera.");
		return;
	}
	
	// Set position/angle.
	mCamera->SetPosition(camera->position);
	mCamera->SetAngle(camera->angle);
}

SceneCastResult Scene::Raycast(const Ray& ray, bool interactiveOnly, const GKObject* ignore) const
{
	SceneCastResult result;
	
	// Check props/actors before BSP.
	// Later, we'll check BSP and see if we hit something obscuring a prop/actor.
	for(auto& object : mObjects)
	{
		// If only interested in interactive objects, skip non-interactive objects.
		if(interactiveOnly && !object->CanInteract()) { continue; }
		
		// Raycast to see if this is the closest thing we've hit.
		// If so, we save it as our current result.
		MeshRenderer* meshRenderer = object->GetMeshRenderer();
		RaycastHit hitInfo;
		if(meshRenderer != nullptr && meshRenderer->Raycast(ray, hitInfo))
		{
			if(hitInfo.t < result.hitInfo.t)
			{
				result.hitInfo = hitInfo;
				result.hitObject = object;
			}
		}
	}
	
	// Assign name in hit info, if anything was hit.
	// This is because GKObjects don't currently do this during raycast.
	if(result.hitObject != nullptr)
	{
		result.hitInfo.name = result.hitObject->GetNoun();
	}
	
	// Check BSP for any hit interactable object.
	BSP* bsp = mSceneData->GetBSP();
	if(bsp != nullptr)
	{
		RaycastHit hitInfo;
		if(bsp->RaycastNearest(ray, hitInfo))
		{
			// If "t" is smaller, then the BSP object obscured any previous hit.
			if(hitInfo.t < result.hitInfo.t)
			{
				result.hitInfo = hitInfo;
				
				// See if hit any actor representing BSP object.
				for(auto& bspActor : mBSPActors)
				{
					// If only interested in interactive objects, skip non-interactive objects.
					if(interactiveOnly && !bspActor->CanInteract()) { continue; }
					
					// Check whether this is the BSP actor we hit.
					if(StringUtil::EqualsIgnoreCase(bspActor->GetName(), hitInfo.name))
					{
						result.hitObject = bspActor;
						break;
					}
				}
			}
		}
	}
	return result;
}

void Scene::Interact(const Ray& ray, GKObject* interactHint)
{
	// Ignore scene interaction while the action bar is showing.
	if(Services::Get<ActionManager>()->IsActionBarShowing()) { return; }
	
	// Also ignore scene interaction when inventory is up.
	if(Services::Get<InventoryManager>()->IsInventoryShowing()) { return; }
	
	// Get interacted object.
	GKObject* interacted = interactHint;
	if(interacted == nullptr)
	{
		SceneCastResult result = Raycast(ray, true);
		interacted = result.hitObject;
	}
	
	// If interacted object is null, see if we hit the floor, in which case we want to walk.
	if(interacted == nullptr)
	{
		BSP* bsp = mSceneData->GetBSP();
		if(bsp != nullptr)
		{
			// Cast ray against scene BSP to see if it intersects with anything.
			// If so, it means we clicked on that thing.
			RaycastHit hitInfo;
			if(bsp->RaycastNearest(ray, hitInfo))
			{
				// Clicked on the floor - move ego to position.
				if(StringUtil::EqualsIgnoreCase(hitInfo.name, mSceneData->GetFloorModelName()))
				{
					// Check walker boundary to see whether we can walk to this spot.
					mEgo->WalkTo(ray.GetPoint(hitInfo.t), mSceneData->GetWalkerBoundary(), nullptr);
				}
			}
		}
		return;
	}
	
	// We've got an object to interact with!
	// See if it has a pre-defined verb. If so, we will immediately execute that noun/verb combo.
	if(!interacted->GetVerb().empty())
	{
		std::cout << "Trying to play default verb " << interacted->GetVerb() << std::endl;
		if(Services::Get<ActionManager>()->ExecuteAction(interacted->GetNoun(), interacted->GetVerb()))
		{
			return;
		}
	}
	
	// No pre-defined verb OR no action for that noun/verb combo - try to show action bar.
	Services::Get<ActionManager>()->ShowActionBar(interacted->GetNoun(), std::bind(&Scene::ExecuteAction, this, std::placeholders::_1));
}

float Scene::GetFloorY(const Vector3& position) const
{
	// Calculate ray origin using passed position, but really high in the air!
	Vector3 rayOrigin = position;
	rayOrigin.y = 10000.0f;
	
	// Create ray with origin high in the sky and pointing straight down.
	Ray downRay(rayOrigin, -Vector3::UnitY);
	
	// Raycast straight down and test against the floor BSP.
	// If we hit something, just use the Y hit position as the floor's Y.
	BSP* bsp = mSceneData->GetBSP();
	if(bsp != nullptr)
	{
		RaycastHit hitInfo;
		if(bsp->RaycastSingle(downRay, mSceneData->GetFloorModelName(), hitInfo))
		{
			return downRay.GetPoint(hitInfo.t).y;
		}
	}
	
	// If didn't hit floor, just return 0.
	// TODO: Maybe we should return a default based on the floor BSP's height?
	return 0.0f;
}

GKActor* Scene::GetSceneObjectByModelName(const std::string& modelName) const
{
	for(auto& object : mObjects)
	{
		MeshRenderer* meshRenderer = object->GetMeshRenderer();
		if(meshRenderer != nullptr)
		{
			Model* model = meshRenderer->GetModel();
			if(model != nullptr)
			{
				if(StringUtil::EqualsIgnoreCase(model->GetNameNoExtension(), modelName))
				{
					return object;
				}
			}
		}
	}
	return nullptr;
}

GKActor* Scene::GetActorByNoun(const std::string& noun) const
{
	for(auto& actor : mActors)
	{
		if(StringUtil::EqualsIgnoreCase(actor->GetNoun(), noun))
		{
			return actor;
		}
	}
	Services::GetReports()->Log("Error", "Error: Who the hell is '" + noun + "'?");
	return nullptr;
}

const ScenePosition* Scene::GetPosition(const std::string& positionName) const
{
	const ScenePosition* position = nullptr;
	if(mSceneData != nullptr)
	{
		position = mSceneData->GetScenePosition(positionName);
	}
	if(position == nullptr)
	{
		Services::GetReports()->Log("Error", "Error: '" + positionName + "' is not a valid position. Call DumpPositions() to see valid positions.");
	}
	return position;
}

void Scene::ApplyTextureToSceneModel(const std::string& modelName, Texture* texture)
{
	mSceneData->GetBSP()->SetTexture(modelName, texture);
}

void Scene::SetSceneModelVisibility(const std::string& modelName, bool visible)
{
	mSceneData->GetBSP()->SetVisible(modelName, visible);
}

bool Scene::IsSceneModelVisible(const std::string& modelName) const
{
	return mSceneData->GetBSP()->IsVisible(modelName);
}

bool Scene::DoesSceneModelExist(const std::string& modelName) const
{
	return mSceneData->GetBSP()->Exists(modelName);
}

void Scene::ExecuteAction(const Action* action)
{
	// Ignore nulls.
	if(action == nullptr) { return; }
	
	// Log to "Actions" stream.
	Services::GetReports()->Log("Actions", "Playing NVC " + action->ToString());
	
	// Before executing the NVC, we need to handle any approach.
	switch(action->approach)
	{
		case Action::Approach::WalkTo:
		{
			const ScenePosition* scenePos = mSceneData->GetScenePosition(action->target);
			if(scenePos != nullptr)
			{
				Debug::DrawLine(mEgo->GetPosition(), scenePos->position, Color32::Green, 60.0f);
				mEgo->WalkTo(scenePos->position, scenePos->heading, mSceneData->GetWalkerBoundary(), [this, action]() -> void {
					Services::Get<ActionManager>()->ExecuteAction(action);
				});
			}
			else
			{
				Services::Get<ActionManager>()->ExecuteAction(action);
			}
			break;
		}
		case Action::Approach::Anim: // Example use: R25 Open/Close Window, R25 Open/Close Dresser
		{
			Animation* anim = Services::GetAssets()->LoadAnimation(action->target);
			if(anim != nullptr)
			{
				mEgo->WalkToAnimationStart(anim, mSceneData->GetWalkerBoundary(), [action]() -> void {
					Services::Get<ActionManager>()->ExecuteAction(action);
				});
			}
			else
			{
				Services::Get<ActionManager>()->ExecuteAction(action);
			}
			break;
		}
		case Action::Approach::Near: // Never used in GK3.
		{
			std::cout << "Executed NEAR approach type!" << std::endl;
			Services::Get<ActionManager>()->ExecuteAction(action);
			break;
		}
		case Action::Approach::NearModel: // Example use: RC1 Bookstore Door, Hallway R25 Door
		{
			Services::Get<ActionManager>()->ExecuteAction(action);
			break;
		}
		case Action::Approach::Region: // Only use: RC1 "No Vacancies" Sign
		{
			Services::Get<ActionManager>()->ExecuteAction(action);
			break;
		}
		case Action::Approach::TurnTo: // Never used in GK3.
		{
			std::cout << "Executed TURNTO approach type!" << std::endl;
			Services::Get<ActionManager>()->ExecuteAction(action);
			break;
		}
		case Action::Approach::TurnToModel: // Example use: R25 Couch Sit, most B25
		{
			// Find position of the model.
			Vector3 modelPosition;
			GKActor* actor = GetSceneObjectByModelName(action->target);
			if(actor != nullptr)
			{
				modelPosition = actor->GetPosition();
			}
			else
			{
				modelPosition = mSceneData->GetBSP()->GetPosition(action->target);
			}
			
			// Get vector from Ego to model.
			Debug::DrawLine(mEgo->GetPosition(), modelPosition, Color32::Green, 60.0f);
			Vector3 egoToModel = modelPosition - mEgo->GetPosition();
			
			// Do a "turn to" heading.
			Heading turnToHeading = Heading::FromDirection(egoToModel);
			mEgo->TurnTo(turnToHeading, [this, action]() -> void {
				Services::Get<ActionManager>()->ExecuteAction(action);
			});
			break;
		}
		case Action::Approach::WalkToSee: // Example use: R25 Look Painting/Couch/Dresser, RC1 Look Bench/Bookstore Sign
		{
			// Find position of the model we want to "walk to see".
			Vector3 targetPosition;
			GKActor* actor = GetSceneObjectByModelName(action->target);
			if(actor != nullptr)
			{
				targetPosition = actor->GetPosition();
			}
			else
			{
				targetPosition = mSceneData->GetBSP()->GetPosition(action->target);
			}
			
			// Walk over and execute action once target is visible.
			mEgo->WalkToSee(action->target, targetPosition, mSceneData->GetWalkerBoundary(), [this, action]() -> void {
				Services::Get<ActionManager>()->ExecuteAction(action);
			});
			break;
		}
		case Action::Approach::None:
		{
			// Just do it!
			Services::Get<ActionManager>()->ExecuteAction(action);
			break;
		}
		default:
		{
			Services::GetReports()->Log("Error", "Invalid approach " + std::to_string(static_cast<int>(action->approach)));
			Services::Get<ActionManager>()->ExecuteAction(action);
			break;
		}
	}
}
