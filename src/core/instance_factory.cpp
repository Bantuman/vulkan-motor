#include "instance_factory.hpp"

#include <core/components.hpp>
#include <ui/ui_components.hpp>
#include <core/instance_utils.hpp>

#include <ecs/ecs.hpp>

using namespace Game;

Instance* Game::InstanceFactory::create(ECS::Manager& ecs, InstanceClass id, ECS::Entity& entity) {
	entity = ecs.create_entity();

	switch (id) {
		case InstanceClass::GAMEWORLD:
			Gameworld::create(ecs, entity);
			break;
		case InstanceClass::AMBIENCE:
			Ambience::create(ecs, entity);
			break;
		case InstanceClass::CUBE_GEOM:
		case InstanceClass::SLOPE_GEOM:
		case InstanceClass::CORNER_SLOPE_GEOM:
		case InstanceClass::SPAWN_LOCATION:
			Geometry::create(ecs, entity);
			break;
		case InstanceClass::IMAGE_PLANE:
			ImagePlane::create(ecs, entity);
			break;
		case InstanceClass::MESH_GEOM:
			MeshGeom::create(ecs, entity);
			break;
		case InstanceClass::CAMERA:
			Camera::create(ecs, entity);
			break;
		case InstanceClass::SKY:
			Sky::create(ecs, entity);
			break;
		case InstanceClass::VIEWPORT_GUI:
			ecs.add_component<ViewportGui>(entity);
			break;
		case InstanceClass::RECT:
			ecs.add_component<Rect2D>(entity);
			break;
		case InstanceClass::TEXT_RECT:
			ecs.add_component<TextRect>(entity);
			break;
		case InstanceClass::TEXT_BUTTON:
			ecs.add_component<TextButton>(entity);
			break;
		case InstanceClass::INPUT_RECT:
			ecs.add_component<InputRect>(entity);
			break;
		case InstanceClass::IMAGE_RECT:
			ecs.add_component<ImageRect>(entity);
			break;
		case InstanceClass::IMAGE_BUTTON:
			ecs.add_component<ImageButton>(entity);
			break;
		case InstanceClass::SCROLLING_RECT:
			ecs.add_component<ScrollingRect>(entity);
			break;
		case InstanceClass::RESIZABLE_RECT:
			ecs.add_component<ResizableRect>(entity);
			break;
		case InstanceClass::VIDEO_RECT:
			ecs.add_component<VideoRect>(entity);
			break;
		case InstanceClass::ATTACHMENT:
			Attachment::create(ecs, entity);
			break;
		case InstanceClass::BONE:
			BoneAttachment::create(ecs, entity);
			break;
		case InstanceClass::MODEL:
			Model::create(ecs, entity);
			break;
		case InstanceClass::ANIMATION_CONTROLLER:
			Animator::create(ecs, entity);
			break;
		default:
			break;
	}

	auto& result = ecs.add_component<Instance>(entity);
	result.m_classID = id;
	result.m_name = Game::get_instance_class_name(id);

	return &result;
}

