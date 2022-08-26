#include "rig.hpp"

#include <cassert>

#include <core/logging.hpp>

using namespace Game;

static std::vector<Memory::SharedPtr<Rig>> g_rigCache;

// RigCreateInfo

uint32_t RigCreateInfo::add_bone(const std::string& name, Math::Matrix4x4 invBind,
		Math::Matrix4x4 localTransform) {
	if (uint32_t idx = get_bone_index(name); idx != Bone::INVALID_BONE_INDEX) {
		return idx;
	}

	Bone bone{};
	bone.name = name;
	bone.inverseBind = std::move(invBind);
	bone.invLocalTransform = glm::inverse(localTransform);
	bone.localTransform = std::move(localTransform);

	m_bones.push_back(std::move(bone));

	return static_cast<uint32_t>(m_bones.size() - 1);
}

void RigCreateInfo::set_bone_parent(const std::string& parentName, const std::string& childName) {
	uint32_t parentIndex = get_bone_index(parentName);
	uint32_t childIndex = get_bone_index(childName);

	assert(parentIndex != Bone::INVALID_BONE_INDEX && "Parent index is invalid");
	assert(childIndex != Bone::INVALID_BONE_INDEX && "Child index is invalid");

	auto& parent = m_bones[parentIndex];
	auto& child = m_bones[childIndex];

	if (parent.lastChild != Bone::INVALID_BONE_INDEX) {
		auto& lastChild = m_bones[parent.lastChild];
		lastChild.nextChild = childIndex;
	}
	else {
		parent.firstChild = childIndex;
	}

	child.parentIndex = parentIndex;
	child.nextChild = Bone::INVALID_BONE_INDEX;
	parent.lastChild = childIndex;
}

uint32_t RigCreateInfo::get_bone_index(const std::string& name) const {
	for (uint32_t i = 0; i < static_cast<uint32_t>(m_bones.size()); ++i) {
		if (m_bones[i].name.compare(name) == 0) {
			return i;
		}
	}

	return Bone::INVALID_BONE_INDEX;
}

void RigCreateInfo::link_root_nodes() {
	m_firstRootBone = Bone::INVALID_BONE_INDEX;
	uint32_t lastRootBone = Bone::INVALID_BONE_INDEX;

	for (uint32_t i = 0; i < m_bones.size(); ++i) {
		auto& bone = m_bones[i];

		if (bone.parentIndex == Bone::INVALID_BONE_INDEX) {
			if (m_firstRootBone == Bone::INVALID_BONE_INDEX) {
				m_firstRootBone = i;
			}
			else {
				m_bones[lastRootBone].nextChild = lastRootBone;
			}

			lastRootBone = i;
			bone.nextChild = Bone::INVALID_BONE_INDEX;
		}
	}
}

// Rig

Memory::SharedPtr<Rig> Rig::create(RigCreateInfo& createInfo) {
	createInfo.link_root_nodes();

	auto rig = std::make_shared<Rig>(std::move(createInfo.m_bones), createInfo.m_firstRootBone,
			static_cast<uint32_t>(g_rigCache.size()));

	for (auto existingRig : g_rigCache) {
		if (*existingRig == *rig) {
			return existingRig;
		}
	}

	g_rigCache.push_back(rig);

	return rig;
}

Rig::Rig(std::vector<Bone>&& bones, uint32_t firstRootBone, uint32_t rigID)
		: m_bones(std::move(bones))
		, m_firstRootBone(firstRootBone)
		, m_rigID(rigID) {}

uint32_t Rig::get_bone_index(const std::string& name) const {
	for (uint32_t i = 0; i < static_cast<uint32_t>(m_bones.size()); ++i) {
		if (m_bones[i].name.compare(name) == 0) {
			return i;
		}
	}

	return Bone::INVALID_BONE_INDEX;
}

const Bone& Rig::get_bone(uint32_t boneIndex) const {
	return m_bones[boneIndex];
}

size_t Rig::get_num_bones() const {
	return m_bones.size();
}

uint32_t Rig::get_rig_id() const {
	return m_rigID;
}

bool Rig::operator==(const Rig& other) const {
	if (m_bones.size() != other.m_bones.size()) {
		return false;
	}

	for (uint32_t i = 0; i < m_bones.size(); ++i) {
		auto& myBone = m_bones[i];
		auto& otherBone = other.m_bones[i];

		// if one has a parent but the other doesn't - fail
		if ((myBone.parentIndex == Bone::INVALID_BONE_INDEX)
				!= (otherBone.parentIndex == Bone::INVALID_BONE_INDEX)) {
			return false;
		}

		if (myBone.name.compare(otherBone.name) != 0) {
			return false;
		}

		if (myBone.parentIndex != Bone::INVALID_BONE_INDEX) {
			auto& myParent = m_bones[myBone.parentIndex];
			auto& otherParent = other.m_bones[otherBone.parentIndex];

			if (myParent.name.compare(otherParent.name) != 0) {
				return false;
			}
		}
	}

	return true;
}

