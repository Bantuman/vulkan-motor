#include "vk_profiler.hpp"

#include <rendering/command_buffer.hpp>
#include <rendering/vk_common.hpp>

using namespace GFX;

// VulkanScopeTimer

VulkanScopeTimer::VulkanScopeTimer(CommandBuffer& cmd, const char* name)
		: m_cmd(&cmd)
		, m_timer{g_vulkanProfiler->get_timestamp_id(), 0, name} {
	auto pool = g_vulkanProfiler->get_timer_pool();
	m_cmd->write_timestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pool, m_timer.startTimestamp);
}

VulkanScopeTimer::~VulkanScopeTimer() {
	m_timer.endTimestamp = g_vulkanProfiler->get_timestamp_id();

	auto pool = g_vulkanProfiler->get_timer_pool();
	m_cmd->write_timestamp(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, pool, m_timer.endTimestamp);

	g_vulkanProfiler->add_timer(std::move(m_timer));
}

// VulkanPipelineStatRecorder

VulkanPipelineStatRecorder::VulkanPipelineStatRecorder(CommandBuffer& cmd, const char* name)
		: m_cmd(&cmd)
		, m_timer{g_vulkanProfiler->get_stat_id(), name} {
	m_cmd->begin_query(g_vulkanProfiler->get_stat_pool(), m_timer.query, 0);
}

VulkanPipelineStatRecorder::~VulkanPipelineStatRecorder() {
	m_cmd->end_query(g_vulkanProfiler->get_stat_pool(), m_timer.query);
	g_vulkanProfiler->add_stat(std::move(m_timer));
}

// VulkanProfiler

VulkanProfiler::VulkanProfiler(VkDevice device, double timestampPeriod, uint32_t perFramePoolSizes)
		: m_device(device)
		, m_currentFrame(0)
		, m_period(timestampPeriod) {
	VkQueryPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	createInfo.queryCount = perFramePoolSizes;

	for (uint32_t i = 0; i < QUERY_FRAME_OVERLAP; ++i) {
		VK_CHECK(vkCreateQueryPool(m_device, &createInfo, nullptr, &m_frames[i].timerPool));
		m_frames[i].timerLast = 0;
	}

	createInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
	createInfo.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;

	for (uint32_t i = 0; i < QUERY_FRAME_OVERLAP; ++i) {
		VK_CHECK(vkCreateQueryPool(m_device, &createInfo, nullptr, &m_frames[i].statPool));
		m_frames[i].statLast = 0;
	}
}

VulkanProfiler::~VulkanProfiler() {
	for (uint32_t i = 0; i < QUERY_FRAME_OVERLAP; ++i) {
		vkDestroyQueryPool(m_device, m_frames[i].timerPool, nullptr);
		vkDestroyQueryPool(m_device, m_frames[i].statPool, nullptr);
	}
}

void VulkanProfiler::grab_queries(std::shared_ptr<CommandBuffer> cmd) {
	auto& lastFrame = m_frames[m_currentFrame];
	m_currentFrame = (m_currentFrame + 1) % QUERY_FRAME_OVERLAP;
	auto& frame = m_frames[m_currentFrame];

	cmd->reset_query_pool(frame.timerPool, 0, frame.timerLast);
	cmd->reset_query_pool(frame.statPool, 0, frame.statLast);

	frame.timerLast = 0;
	frame.timers.clear();
	frame.statLast = 0;
	frame.statRecorders.clear();

	std::vector<uint64_t> queryResults(lastFrame.timerLast);

	if (lastFrame.timerLast != 0) {
		// To do this async, VK_QUERY_RESULT_WITH_AVAILABILITY_BIT checks readyness
		vkGetQueryPoolResults(m_device, lastFrame.timerPool, 0, lastFrame.timerLast,
				queryResults.size() * sizeof(uint64_t), queryResults.data(), sizeof(uint64_t),
				VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
	}

	std::vector<uint64_t> statResults(lastFrame.statLast);

	if (lastFrame.statLast != 0) {
		// To do this async, VK_QUERY_RESULT_WITH_AVAILABILITY_BIT checks readyness
		vkGetQueryPoolResults(m_device, lastFrame.statPool, 0, lastFrame.statLast,
				statResults.size() * sizeof(uint64_t), statResults.data(), sizeof(uint64_t),
				VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
	}

	for (auto& timer : lastFrame.timers) {
		auto begin = queryResults[timer.startTimestamp];
		auto end = queryResults[timer.endTimestamp];
		auto timestamp = end - begin;

		m_timing[timer.name] = (static_cast<double>(timestamp) * m_period) / 1000000.0;
	}

	for (auto& stat : lastFrame.statRecorders) {
		m_stats[stat.name] = statResults[stat.query];
	}
}

void VulkanProfiler::add_timer(ScopeTimer timer) {
	m_frames[m_currentFrame].timers.emplace_back(std::move(timer));
}

void VulkanProfiler::add_stat(StatRecorder stat) {
	m_frames[m_currentFrame].statRecorders.emplace_back(std::move(stat));
}

double VulkanProfiler::get_stat(const std::string& name) const {
	if (auto it = m_timing.find(name); it != m_timing.end()) {
		return it->second;
	}

	return 0.0;
}

const std::unordered_map<std::string, double>& VulkanProfiler::get_timing_data() const {
	return m_timing;
}

const std::unordered_map<std::string, int32_t>& VulkanProfiler::get_stat_data() const {
	return m_stats;
}

uint32_t VulkanProfiler::get_timestamp_id() {
	auto q = m_frames[m_currentFrame].timerLast;
	++m_frames[m_currentFrame].timerLast;

	return q;
}

uint32_t VulkanProfiler::get_stat_id() {
	auto q = m_frames[m_currentFrame].statLast;
	++m_frames[m_currentFrame].statLast;

	return q;
}

VkQueryPool VulkanProfiler::get_timer_pool() const {
	return m_frames[m_currentFrame].timerPool;
}

VkQueryPool VulkanProfiler::get_stat_pool() const {
	return m_frames[m_currentFrame].statPool;
}

