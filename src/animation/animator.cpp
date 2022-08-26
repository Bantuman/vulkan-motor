#include "animator.hpp"

#include <animation/animation.hpp>
#include <animation/rig.hpp>

#include <core/logging.hpp>

#include <ecs/ecs.hpp>

#include <core/instance.hpp>
#include <core/instance_utils.hpp>
#include <core/model.hpp>
#include <animation/bone_attachment.hpp>

#include <rendering/rigged_mesh.hpp>

using namespace Game;

void Animator::create(ECS::Manager& ecs, ECS::Entity entity) {
	ecs.add_component<Animator>(entity);
}

/*static void calc_joint_transform(Rig& rig, Math::Matrix4x4* finalBoneTransforms,
		const Animation& anim, float time, uint32_t boneIndex, Bone& bone,
		const Math::Matrix4x4& parentTransform);*/

void Game::update_animators(ECS::Manager& ecs, float deltaTime) {
	ecs.run_system<Instance, Animator>([&](auto, auto& inst, auto& animator) {
		if (inst.m_parent == ECS::INVALID_ENTITY) {
			return;
		}

		auto& parent = ecs.get_component<Instance>(inst.m_parent);

		if (parent.m_classID != InstanceClass::MODEL) {
			return;
		}

		auto& model = ecs.get_component<Model>(inst.m_parent);

		if (model.get_primary_part() == ECS::INVALID_ENTITY) {
			return;
		}

		auto& instPrimaryPart = ecs.get_component<Instance>(model.get_primary_part());

		if (animator.m_currentAnim) {
			auto& anim = *animator.m_currentAnim;

			animator.m_animTime += deltaTime;

			for_each_descendant(ecs, instPrimaryPart, [&](auto descEntity, auto& desc) {
				if (desc.m_classID == InstanceClass::BONE) {
					auto& ba = ecs.get_component<BoneAttachment>(descEntity);
					Math::BoneTransform res{};
					anim.get_transform(desc.m_name, animator.m_animTime, res);
					ba.set_transform(ecs, desc,
							ba.get_local_transform().fast_inverse() * res.to_transform());
				}
			});

			if (animator.m_animTime >= anim.get_duration()) {
				animator.m_animTime -= anim.get_duration();
			}
		}
	});
}

/*static void calc_joint_transform(Rig& rig, Math::Matrix4x4* finalBoneTransforms,
		const Animation& anim, float time, uint32_t boneIndex, Bone& bone,
		const Math::Matrix4x4& parentTransform) {
	Math::Transform res;
	anim.get_transform(bone.name, time, res);

	auto globalTransform = parentTransform * res.to_matrix();

	finalBoneTransforms[boneIndex] = globalTransform * bone.inverseBind;

	rig.for_each_child(bone, [&](auto childIndex, auto& child) {
		calc_joint_transform(rig, finalBoneTransforms, anim, time, childIndex, child,
				globalTransform);
	});
}*/

