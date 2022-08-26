#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <volk.h>

#include <core/common.hpp>
#include <core/local.hpp>

class CommandBuffer;

namespace GFX {

struct ScopeTimer {
	uint32_t startTimestamp;
	uint32_t endTimestamp;
	std::string name;
};

struct StatRecorder {
	uint32_t query;
	std::string name;
};

class VulkanScopeTimer final {
	public:
		explicit VulkanScopeTimer(CommandBuffer&, const char* name);
		~VulkanScopeTimer();

		NULL_COPY_AND_ASSIGN(VulkanScopeTimer);
	private:
		CommandBuffer* m_cmd;
		ScopeTimer m_timer;
};

class VulkanPipelineStatRecorder final {
	public:
		explicit VulkanPipelineStatRecorder(CommandBuffer&, const char* name);
		~VulkanPipelineStatRecorder();
	private:
		CommandBuffer* m_cmd;
		StatRecorder m_timer;
};

class VulkanProfiler final {
	public:
		static constexpr const uint32_t QUERY_FRAME_OVERLAP = 3;

		explicit VulkanProfiler(VkDevice device, double timestampPeriod,
				uint32_t perFramePoolSizes = 100);
		~VulkanProfiler();

		NULL_COPY_AND_ASSIGN(VulkanProfiler);

		void grab_queries(std::shared_ptr<CommandBuffer>);

		void add_timer(ScopeTimer);
		void add_stat(StatRecorder);

		double get_stat(const std::string& name) const;

		const std::unordered_map<std::string, double>& get_timing_data() const;
		const std::unordered_map<std::string, int32_t>& get_stat_data() const;

		uint32_t get_timestamp_id();
		uint32_t get_stat_id();

		VkQueryPool get_timer_pool() const;
		VkQueryPool get_stat_pool() const;
	private:
		struct Frame {
			std::vector<ScopeTimer> timers;
			VkQueryPool timerPool;
			uint32_t timerLast;

			std::vector<StatRecorder> statRecorders;
			VkQueryPool statPool;
			uint32_t statLast;
		};

		Frame m_frames[QUERY_FRAME_OVERLAP];

		VkDevice m_device;

		uint32_t m_currentFrame;
		double m_period;

		std::unordered_map<std::string, double> m_timing;
		std::unordered_map<std::string, int32_t> m_stats;

		friend class VulkanScopeTimer;
		friend class VulkanPipelineStatRecorder;
};

}

inline Local<GFX::VulkanProfiler> g_vulkanProfiler;

