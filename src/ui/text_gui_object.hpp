#pragma once

#include <ui/font_id.hpp>
#include <ui/gui_object.hpp>

#include <vector>

namespace RichText {

class TextInfoStream;

}

namespace Game {

class TextGuiObject : public GuiObject {
	public:
		std::string m_text;
		std::string m_contentText; // computed
		Math::Color3 m_textColor3;
		Math::Color3 m_textStrokeColor3;
		Math::Vector2 m_textBounds; // computed
		float m_textTransparency;
		float m_textStrokeTransparency;
		float m_textSize;
		float m_lineHeight;
		int32_t m_maxVisibleGraphemes;
		FontID m_fontID;
		TextXAlignment m_textXAlignment;
		TextYAlignment m_textYAlignment;
		TextTruncate m_textTruncate;
		bool m_textScaled;
		bool m_textWrapped;
		bool m_richText;
		bool m_textFits; // computed

		const std::string& get_text() const;
		void set_text(ECS::Manager&, Instance& selfInstance, const std::string&);
		void set_text_color3(ECS::Manager&, const Math::Color3&);
		void set_text_stroke_color3(ECS::Manager&, const Math::Color3&);
		void set_text_transparency(ECS::Manager&, Instance& selfInstance, float);
		void set_text_stroke_transparency(ECS::Manager&, Instance& selfInstance, float);
		void set_text_size(ECS::Manager&, Instance& selfInstance, uint8_t);
		void set_line_height(ECS::Manager&, Instance& selfInstance, float);
		void set_max_visible_graphemes(ECS::Manager&, Instance& selfInstance, int32_t);
		void set_font(ECS::Manager&, Instance& selfInstance, FontID);
		void set_text_x_alignment(ECS::Manager&, Instance& selfInstance, TextXAlignment);
		void set_text_y_alignment(ECS::Manager&, Instance& selfInstance, TextYAlignment);
		void set_text_truncate(ECS::Manager&, Instance& selfInstance, TextTruncate);
		void set_text_scaled(ECS::Manager&, Instance& selfInstance, bool);
		void set_text_wrapped(ECS::Manager&, Instance& selfInstance, bool);
		void set_rich_text(ECS::Manager&, Instance& selfInstance, bool);

		void clear(ECS::Manager&) override;
		void redraw(ECS::Manager&, RectOrderInfo, const ViewportGui&,
				const Math::Rect* clipRect) override;

		void update_rect_transforms(ECS::Manager&, const Math::Vector2& invScreen);
	private:
		std::vector<ECS::Entity> m_textRects;
		std::vector<ECS::Entity> m_strokeRects;

		void clear_internal(ECS::Manager&);
		void redraw_internal(ECS::Manager&, RectOrderInfo&, const ViewportGui&,
				const Math::Rect* clipRect);

		void draw_text_line(ECS::Manager&, const Math::Vector2& invScreen, RectOrderInfo&,
				const Math::Rect* clipRect, RichText::TextInfoStream&, size_t begin, size_t end,
				size_t lineNumber, float totalHeight, float textWidth, float textSize,
				float baseline, uint32_t& rectIndex, uint32_t& strokeIndex);

		ECS::Entity& get_or_create_rect(ECS::Manager&, uint32_t index);
		ECS::Entity& get_or_create_stroke(ECS::Manager&, uint32_t index);
};

}

