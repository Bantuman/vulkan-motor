#pragma once

#include <core/common.hpp>

#include <volk.h>

#include <memory>
#include <string_view>
#include <vector>

class RenderContext;

struct ReflectedDescriptorLayout {
	uint32_t setNumber;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	uint32_t variableArrayIndex;
};

class ShaderProgram {
	public:
		explicit ShaderProgram(RenderContext&, const VkShaderModule* modules,
				const VkShaderStageFlagBits* stageFlags, uint32_t numModules,
				VkPipelineLayout);
		~ShaderProgram();

		ShaderProgram(ShaderProgram&&);
		ShaderProgram& operator=(ShaderProgram&&);

		ShaderProgram(const ShaderProgram&) = delete;
		void operator=(const ShaderProgram&) = delete;

		uint32_t get_num_modules() const;
		const VkShaderModule* get_shader_modules() const;
		const VkShaderStageFlagBits* get_stage_flags() const;
		VkPipelineLayout get_pipeline_layout() const;
	private:
		VkShaderModule m_modules[2];
		VkShaderStageFlagBits m_stageFlags[2];
		uint32_t m_numModules;
		VkPipelineLayout m_layout;

		RenderContext* m_context;
};

class ShaderProgramBuilder {
	public:
		explicit ShaderProgramBuilder() = default;

		NULL_COPY_AND_ASSIGN(ShaderProgramBuilder);

		ShaderProgramBuilder& add_shader(const std::string_view& fileName,
				VkShaderStageFlagBits stage, bool hasDynamicArray = false);

		ShaderProgramBuilder& add_shader(const uint32_t* data, size_t dataSize,
				VkShaderStageFlagBits stage, bool hasDynamicArray = false);

		ShaderProgramBuilder& add_type_override(uint32_t set, uint32_t binding, VkDescriptorType);

		[[nodiscard]] std::shared_ptr<ShaderProgram> build(RenderContext&);
	private:
		struct ReflectionOverride {
			uint32_t set;
			uint32_t binding;
			VkDescriptorType type;
		};

		struct ShaderData {
			const uint32_t* data;
			size_t dataSize;
			VkShaderStageFlagBits stage;
			bool hasDynamicArray;
		};

		std::vector<ShaderData> m_shaderData;
		std::vector<ReflectedDescriptorLayout> m_layouts;
		std::vector<VkPushConstantRange> m_constantRanges;
		std::vector<std::vector<char>> m_ownedFileMemory;
		std::vector<ReflectionOverride> m_overrides;

		bool m_error = false;

		bool reflect_module(const uint32_t*, size_t, VkShaderStageFlagBits, bool);
		void add_layout(const ReflectedDescriptorLayout&);
		VkPipelineLayout build_pipeline_layout(RenderContext&);
};

