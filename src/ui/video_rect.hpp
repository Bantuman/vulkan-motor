#pragma once

#include <ui/gui_image_emitter.hpp>
#include <rendering/video_texture.hpp>

namespace Game {
	class VideoRect : public GuiImageEmitter {
	public:
		void set_video(ECS::Manager&, Instance& selfInstance, const std::string&);

		void clear(ECS::Manager&) override;
		void redraw(ECS::Manager&, RectOrderInfo, const ViewportGui&,
			const Math::Rect* clipRect) override;

		void update_rect_transforms(ECS::Manager&, const Math::Vector2& invScreen);
	private:
		std::string m_videoName;
		std::shared_ptr<VideoTexture> m_video;
		void clear_internal(ECS::Manager&);
		void redraw_internal(ECS::Manager&, RectOrderInfo&, const ViewportGui&,
			const Math::Rect* clipRect);
	};

}

