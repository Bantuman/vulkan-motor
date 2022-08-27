#include "video_rect.hpp"

#include <asset/video_cache.hpp>

void Game::VideoRect::set_video(ECS::Manager&, Instance& selfInstance, const std::string& filepath)
{
	m_videoName = filepath;
	m_video = g_videoCache->get_or_load<VideoTextureLoader>(m_videoName, *g_renderContext, m_videoName, false, true);
}

void Game::VideoRect::clear(ECS::Manager& ecs)
{
	GuiObject::clear(ecs);
}

void Game::VideoRect::redraw(ECS::Manager& ecs, RectOrderInfo info, const ViewportGui& viewport, const Math::Rect* clipRect)
{
	GuiObject::redraw(ecs, info, viewport, clipRect);
}

void Game::VideoRect::update_rect_transforms(ECS::Manager& ecs, const Math::Vector2& invScreen)
{
	GuiObject::update_rect_transforms(ecs, invScreen);
}

void Game::VideoRect::clear_internal(ECS::Manager& ecs)
{
}

void Game::VideoRect::redraw_internal(ECS::Manager& ecs, RectOrderInfo&, const ViewportGui&, const Math::Rect* clipRect)
{
}
