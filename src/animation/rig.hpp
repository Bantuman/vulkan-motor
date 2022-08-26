#pragma once

#include <string>
#include <vector>

#include <core/memory.hpp>

#include <math/matrix4x4.hpp>

namespace Game {

struct Bone {
	static constexpr const uint32_t INVALID_BONE_INDEX = static_cast<uint32_t>(~0u);

	Math::Matrix4x4 inverseBind;
	Math::Matrix4x4 localTransform;
	Math::Matrix4x4 invLocalTransform;
	std::string name;
	uint32_t parentIndex = INVALID_BONE_INDEX;
	uint32_t firstChild = INVALID_BONE_INDEX;
	uint32_t lastChild = INVALID_BONE_INDEX;
	uint32_t nextChild = INVALID_BONE_INDEX;
};

class RigCreateInfo {
	public:
		uint32_t add_bone(const std::string& name, Math::Matrix4x4 invBind,
				Math::Matrix4x4 localTransform);
		void set_bone_parent(const std::string& parent, const std::string& child);

		uint32_t get_bone_index(const std::string& name) const;

		void link_root_nodes();
	private:
		std::vector<Bone> m_bones;
		uint32_t m_firstRootBone;

		friend class Rig;
};

class Rig {
	public:
		static constexpr const size_t MAX_WEIGHTS = 4;

		static Memory::SharedPtr<Rig> create(RigCreateInfo&);

		explicit Rig(std::vector<Bone>&& bones, uint32_t firstRootBone, uint32_t rigID);

		uint32_t get_bone_index(const std::string& name) const;
		const Bone& get_bone(uint32_t) const;
		size_t get_num_bones() const;
		uint32_t get_rig_id() const;

		bool operator==(const Rig&) const;

		template <typename Functor>
		void for_each_child(const Bone& bone, Functor&& func) {
			auto nextIndex = bone.firstChild;

			while (nextIndex != Bone::INVALID_BONE_INDEX) {
				auto& child = m_bones[nextIndex];
				func(nextIndex, child);
				nextIndex = child.nextChild;
			}
		}

		template <typename Functor>
		void for_each_descendant(const Bone& bone, Functor&& func) {
			for_each_child(bone, [this, func](auto index, auto& child) {
				func(index, child);
				for_each_descendant(child, func);
			});
		}

		template <typename Functor>
		void for_each_root(Functor&& func) {
			auto nextRoot = m_firstRootBone;

			while (nextRoot != Bone::INVALID_BONE_INDEX) {
				auto& root = m_bones[nextRoot];
				func(nextRoot, root);
				nextRoot = root.nextChild;
			}
		}

		template <typename Functor>
		void traverse(Functor&& func) {
			for_each_root([this, func](auto index, auto& root) {
				func(index, root);
				for_each_descendant(root, func);
			});
		}
	private:
		std::vector<Bone> m_bones;
		uint32_t m_firstRootBone;
		uint32_t m_rigID;
};

}

