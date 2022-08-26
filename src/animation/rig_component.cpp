#include "rig_component.hpp"

#include <animation/rig.hpp>
#include <rendering/renderer/rigged_mesh_renderer.hpp>

#include <core/logging.hpp>

#include <ecs/ecs.hpp>

#include <core/instance.hpp>
#include <core/instance_utils.hpp>
#include <core/model.hpp>
#include <core/mesh_geom.hpp>
#include <animation/bone_attachment.hpp>

using namespace Game;

static void update_rig_bones(ECS::Manager& ecs, Instance& rootPart,
		const Math::Matrix4x4& parentTransform, RigComponent& rc);

RigComponent::RigComponent(Memory::SharedPtr<Rig> rig, ECS::Entity rigContainer)
		: m_rig(std::move(rig))
		, m_finalBoneTransforms(m_rig->get_num_bones(), Math::Matrix4x4(1.f))
		, m_rigContainer(rigContainer) {}

void Game::update_rigs(ECS::Manager& ecs) {
	ecs.run_system<RigComponent>([&](auto eRigComponent, auto& rcomp) {
		if (!ecs.is_valid_entity(rcomp.m_rigContainer)) {
			return;
		}

		auto& instContainer = ecs.get_component<Instance>(rcomp.m_rigContainer);

		if (instContainer.m_classID == InstanceClass::MODEL) {
			auto& model = ecs.get_component<Model>(rcomp.m_rigContainer);

			if (model.get_primary_part() == ECS::INVALID_ENTITY) {
				return;
			}

			auto& instPrimaryPart = ecs.get_component<Instance>(model.get_primary_part());

			update_rig_bones(ecs, instPrimaryPart, Math::Matrix4x4(1.f), rcomp);

			auto* dstData = g_riggedMeshRenderer->get_or_add_rig_instance(*rcomp.m_rig,
					eRigComponent);
			memcpy(dstData, rcomp.m_finalBoneTransforms.data(),
					rcomp.m_rig->get_num_bones() * sizeof(Math::Matrix4x4));
		}
	});
}

static void update_rig_bones(ECS::Manager& ecs, Instance& instBone,
		const Math::Matrix4x4& parentTransform, RigComponent& rc) {
	for_each_child(ecs, instBone, [&](auto entity, auto& child) {
		if (child.m_classID == InstanceClass::BONE) {
			auto& ba = ecs.get_component<BoneAttachment>(entity);
			auto boneIndex = rc.m_rig->get_bone_index(child.m_name);

			auto globalTransform = parentTransform;

			if (boneIndex != Bone::INVALID_BONE_INDEX) {
				auto& bone = rc.m_rig->get_bone(boneIndex);

				//globalTransform *= ba.get_transform().to_matrix4x4();
				globalTransform *= ba.get_transformed_transform().to_matrix4x4();
				rc.m_finalBoneTransforms[boneIndex] = globalTransform * bone.inverseBind;
			}

			update_rig_bones(ecs, child, globalTransform, rc);
		}
	});
}

