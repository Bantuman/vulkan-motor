#include "video_rect.hpp"

#include <asset/video_cache.hpp>
#include <rendering/renderer/ui_renderer.hpp>

void Game::VideoRect::set_video(ECS::Manager&, Instance& selfInstance, const std::string& filepath)
{
	m_videoName = filepath;
	m_video = g_videoCache->get_or_load<VideoTextureLoader>(m_videoName, *g_renderContext, m_videoName, true, false);
}

VideoStatus Game::VideoRect::get_status() const
{
	return m_status;
}

std::shared_ptr<VideoTexture> Game::VideoRect::get_texture() const
{
	return m_video;
}

void Game::VideoRect::set_looping(bool isLooping)
{
	m_isLooping = isLooping;
}

void Game::VideoRect::update(ECS::Manager& ecs, Instance& selfInst, RenderContext& ctx, float deltaTime)
{
	if (!m_video)
		return;
	double fps = m_video->get_fps();
	// FIXME: Update sound
	m_frameTimer += deltaTime;
	while (m_frameTimer >= 1.0 / fps)
	{
		FrameStatus status = m_video->iterate_frame();
		if (status == FrameStatus::END_OR_ERROR)
		{
			if (m_isLooping)
			{
				m_status = VideoStatus::PLAYING;
				m_video->set_frame(0);
			}
			else 
			{
				m_status = VideoStatus::END;
			}
		}

		m_video->write_image(ctx, m_absoluteSize.x, m_absoluteSize.y);
		mark_for_redraw(ecs, selfInst);
		m_frameTimer -= 1.0 / fps;
	}
}

void Game::VideoRect::clear(ECS::Manager& ecs)
{
	GuiObject::clear(ecs);
	clear_internal(ecs);
}

void Game::VideoRect::redraw(ECS::Manager& ecs, RectOrderInfo orderInfo, const ViewportGui& viewport, const Math::Rect* clipRect)
{
	GuiObject::redraw(ecs, orderInfo, viewport, clipRect);
	redraw_internal(ecs, orderInfo, viewport, clipRect);
}

void Game::VideoRect::update_rect_transforms(ECS::Manager& ecs, const Math::Vector2& invScreen)
{
	GuiObject::update_rect_transforms(ecs, invScreen);
}

void Game::VideoRect::clear_internal(ECS::Manager& ecs)
{
	if (m_videoEntity != ECS::INVALID_ENTITY)
	{
		try_remove_rect_raw(ecs, m_videoEntity);
	}
}

void Game::VideoRect::redraw_internal(ECS::Manager& ecs, RectOrderInfo& orderInfo, const ViewportGui& viewport, const Math::Rect* clipRect)
{
	if (!m_video)
	{
		return;
	}
	RectInstance rect{};
	rect.color = { 1.f, 1.f, 1.f, 1.f }; // m_imageColor3.to_vector4(1.f - m_imageTransparency);
	rect.samplerIndex = static_cast<uint32_t>(m_resampleMode);
	rect.imageIndex = g_uiRenderer->get_image_index(*m_video->get_image_view());

	Math::Vector2 screenSize = get_absolute_size();
	rect.texLayout = Math::Vector4(0, 0, 1, 1);

	make_screen_transform(rect.transform, get_global_transform(), screenSize, viewport.get_inv_size());
	emit_rect(ecs, m_videoEntity, rect, Math::Vector2(), orderInfo, clipRect);
}
