#pragma once

#include <ui/gui_image_emitter.hpp>
#include <rendering/video_texture.hpp>

enum class VideoStatus
{
	PLAYING = 0,
	END = 1,
	PAUSED = 2
};
namespace Game {
	class VideoRect : public GuiImageEmitter {
	public:
		void set_video(ECS::Manager&, Instance& selfInstance, const std::string&);
		void set_looping(bool isLooping);

		VideoStatus get_status() const;
		std::shared_ptr<VideoTexture> get_texture() const;

		void update(ECS::Manager& ecs, Instance& selfInst, RenderContext& ctx, float deltaTime);
		void clear(ECS::Manager&) override;
		void redraw(ECS::Manager&, RectOrderInfo, const ViewportGui&,
			const Math::Rect* clipRect) override;

		void update_rect_transforms(ECS::Manager&, const Math::Vector2& invScreen);
	private:
		VideoStatus m_status;
		double m_frameTimer;
		bool m_isLooping;
		ECS::Entity m_videoEntity = ECS::INVALID_ENTITY;
		ResamplerMode m_resampleMode;
		Math::Color3 m_imageColor3;
		float m_imageTransparency;
		std::string m_videoName;
		std::shared_ptr<VideoTexture> m_video;
		void clear_internal(ECS::Manager&);
		void redraw_internal(ECS::Manager&, RectOrderInfo&, const ViewportGui&,
			const Math::Rect* clipRect);
	};

}

