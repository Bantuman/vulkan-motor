#include "shader_program.hpp"

#include <cassert>
#include <cstring>

#include <core/logging.hpp>

#include <rendering/render_context.hpp>
#include <file/file_system.hpp>

#include <spirv_reflect.h>

// ShaderProgram

ShaderProgram::ShaderProgram(RenderContext& context, const VkShaderModule* modules,
			const VkShaderStageFlagBits* stageFlags, uint32_t numModules,
			VkPipelineLayout layout)
		: m_numModules(numModules)
		, m_layout(layout)
		, m_context(&context) {
	assert(numModules <= 2 && "Must have at most 2 modules");

	memcpy(m_modules, modules, numModules * sizeof(VkShaderModule));
	memcpy(m_stageFlags, stageFlags, numModules * sizeof(VkShaderStageFlagBits));
}

ShaderProgram::~ShaderProgram() {
	for (uint32_t i = 0; i < m_numModules; ++i) {
		m_context->queue_delete([
				device=this->m_context->get_device(), module=this->m_modules[i]] {
			vkDestroyShaderModule(device, module, nullptr);
		});
	}
}

ShaderProgram::ShaderProgram(ShaderProgram&& other)
		: m_numModules(other.m_numModules)
		, m_layout(other.m_layout)
		, m_context(other.m_context) {
	memcpy(m_modules, other.m_modules, m_numModules * sizeof(VkShaderModule));
	memcpy(m_stageFlags, other.m_stageFlags, m_numModules * sizeof(VkShaderStageFlagBits));

	other.m_numModules = 0;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) {
	m_numModules = other.m_numModules;
	memcpy(m_modules, other.m_modules, m_numModules * sizeof(VkShaderModule));
	memcpy(m_stageFlags, other.m_stageFlags, m_numModules * sizeof(VkShaderStageFlagBits));
	m_layout = other.m_layout;

	other.m_numModules = 0;
	
	return *this;
}

uint32_t ShaderProgram::get_num_modules() const {
	return m_numModules;
}

const VkShaderModule* ShaderProgram::get_shader_modules() const {
	return m_modules;
}

const VkShaderStageFlagBits* ShaderProgram::get_stage_flags() const {
	return m_stageFlags;
}

VkPipelineLayout ShaderProgram::get_pipeline_layout() const {
	return m_layout;
}

// ShaderProgramBuilder

ShaderProgramBuilder& ShaderProgramBuilder::add_shader(const std::string_view& fileName,
		VkShaderStageFlagBits stage, bool hasDynamicArray) {
	auto data = g_fileSystem->file_read_bytes(fileName);

	if (data.empty()) {
		m_error = true;
		return *this;
	}

	m_ownedFileMemory.emplace_back(std::move(data));
	auto& fileData = m_ownedFileMemory.back();

	return add_shader(reinterpret_cast<const uint32_t*>(fileData.data()), fileData.size(), stage,
			hasDynamicArray);
}

ShaderProgramBuilder& ShaderProgramBuilder::add_shader(const uint32_t* data, size_t dataSize,
		VkShaderStageFlagBits stage, bool hasDynamicArray) {
	m_shaderData.emplace_back(ShaderData{data, dataSize, stage, hasDynamicArray});

	return *this;
}

ShaderProgramBuilder& ShaderProgramBuilder::add_type_override(uint32_t set, uint32_t binding,
		VkDescriptorType type) {
	m_overrides.push_back({set, binding, type});
	return *this;
}

std::shared_ptr<ShaderProgram> ShaderProgramBuilder::build(RenderContext& context) {
	if (m_error) {
		return {};
	}

	std::vector<VkShaderModule> modules;
	std::vector<VkShaderStageFlagBits> stageFlags;

	bool error = false;

	for (auto [data, dataSize, stage, hasDynamicArray] : m_shaderData) {
		if (!reflect_module(data, dataSize, stage, hasDynamicArray)) {
			error = true;
			break;
		}

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = dataSize;
		createInfo.pCode = data;

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(context.get_device(), &createInfo, nullptr, &shaderModule)
				!= VK_SUCCESS) {
			error = true;
			break;
		}
		else {
			modules.push_back(shaderModule);
			stageFlags.push_back(stage);
		}
	}

	VkPipelineLayout layout = VK_NULL_HANDLE;

	if (!error) {
		layout = build_pipeline_layout(context);
		error = layout == VK_NULL_HANDLE;
	}

	if (error) {
		for (auto shaderModule : modules) {
			vkDestroyShaderModule(context.get_device(), shaderModule, nullptr);
		}

		return {};
	}

	return std::make_shared<ShaderProgram>(context, modules.data(), stageFlags.data(),
			modules.size(), layout);
}

bool ShaderProgramBuilder::reflect_module(const uint32_t* data, size_t dataSize,
		VkShaderStageFlagBits stage, bool hasDynamicArray) {
	SpvReflectShaderModule spvModule;
	auto result = spvReflectCreateShaderModule(dataSize, data, &spvModule);

	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		return false;
	}

	uint32_t count = 0;
	result = spvReflectEnumerateDescriptorSets(&spvModule, &count, NULL);

	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		return false;
	}

	std::vector<SpvReflectDescriptorSet*> reflectedSets(count);
	result = spvReflectEnumerateDescriptorSets(&spvModule, &count, reflectedSets.data());

	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		return false;
	}

	std::vector<ReflectedDescriptorLayout> reflectedLayouts;

	for (size_t i = 0; i < reflectedSets.size(); ++i) {
		auto& reflectedSet = *reflectedSets[i];

		ReflectedDescriptorLayout reflectedLayout{};
		reflectedLayout.setNumber = reflectedSet.set;
		reflectedLayout.variableArrayIndex = ~0u;

		for (uint32_t j = 0; j < reflectedSet.binding_count; ++j) {
			auto& reflectedBinding = *reflectedSet.bindings[j];

			if (hasDynamicArray && reflectedBinding.binding == reflectedSet.binding_count - 1) {
				reflectedLayout.variableArrayIndex = j;
			}

			VkDescriptorSetLayoutBinding binding{};
			binding.descriptorCount = 1;
			binding.binding = reflectedBinding.binding;
			binding.stageFlags = stage;

			for (uint32_t dim = 0; dim < reflectedBinding.array.dims_count; ++dim) {
				binding.descriptorCount *= reflectedBinding.array.dims[dim];
			}

			bool hasOverride = false;

			for (auto& ovr : m_overrides) {
				if (ovr.set == reflectedSet.set && ovr.binding == reflectedBinding.binding) {
					binding.descriptorType = ovr.type;
					hasOverride = true;
					break;
				}
			}

			if (!hasOverride) {
				binding.descriptorType
						= static_cast<VkDescriptorType>(reflectedBinding.descriptor_type);
			}

			reflectedLayout.bindings.emplace_back(std::move(binding));
		}

		reflectedLayouts.emplace_back(std::move(reflectedLayout));
	}

	result = spvReflectEnumeratePushConstantBlocks(&spvModule, &count, nullptr);

	std::vector<SpvReflectBlockVariable*> pushConstants(count);
	result = spvReflectEnumeratePushConstantBlocks(&spvModule, &count, pushConstants.data());

	for (size_t i = 0; i < count; ++i) {
		VkPushConstantRange pcr{};
		pcr.offset = pushConstants[i]->offset;
		pcr.size = pushConstants[i]->size;
		pcr.stageFlags = stage;

		m_constantRanges.push_back(std::move(pcr));
	}

	for (auto& layout : reflectedLayouts) {
		add_layout(layout);
	}

	return true;
}

void ShaderProgramBuilder::add_layout(const ReflectedDescriptorLayout& layoutIn) {
	for (auto& layout : m_layouts) {
		if (layout.setNumber == layoutIn.setNumber) {
			for (auto& bindingIn : layoutIn.bindings) {
				bool foundExistingBindings = false;

				for (auto& binding : layout.bindings) {
					if (binding.binding == bindingIn.binding) {
						foundExistingBindings = true;
						binding.stageFlags |= bindingIn.stageFlags;
						break;
					}
				}

				if (!foundExistingBindings) {
					layout.bindings.push_back(bindingIn);
				}
			}

			return;
		}
	}

	m_layouts.push_back(layoutIn);
}

VkPipelineLayout ShaderProgramBuilder::build_pipeline_layout(RenderContext& ctx) {
	std::vector<VkDescriptorSetLayout> setLayouts;
	std::vector<VkDescriptorBindingFlags> flags;

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags{};
	bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;

	for (auto& descLayoutInfo : m_layouts) {
		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = static_cast<uint32_t>(descLayoutInfo.bindings.size());
		createInfo.pBindings = descLayoutInfo.bindings.data();

		if (descLayoutInfo.variableArrayIndex != ~0u) {
			flags.clear();
			flags.resize(descLayoutInfo.bindings.size());

			flags[descLayoutInfo.variableArrayIndex] =
					VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
					| VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

			bindingFlags.bindingCount = createInfo.bindingCount;
			bindingFlags.pBindingFlags = flags.data();

			createInfo.pNext = &bindingFlags;
		}

		setLayouts.push_back(ctx.descriptor_set_layout_create(createInfo));
	}

	VkPipelineLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
	createInfo.pSetLayouts = setLayouts.data();

	if (!m_constantRanges.empty()) {
		createInfo.pushConstantRangeCount = static_cast<uint32_t>(m_constantRanges.size());
		createInfo.pPushConstantRanges = m_constantRanges.data();
	}

	return ctx.pipeline_layout_create(createInfo);
}

