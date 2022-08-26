#pragma once

#include <ecs/ecs_fwd.hpp>

#include <ui/ui_enumerations.hpp>
#include <algorithm>

#include <math/vector2.hpp>

namespace Game { 

struct Instance;
class GuiObject;

class ViewportGui {
	public:
		void set_display_order(ECS::Manager&, Instance& selfInst, int32_t);
		void set_z_index_behavior(ECS::Manager&, Instance& selfInst, ZIndexBehavior);
		void set_enabled(ECS::Manager&, Instance& selfInst, bool);
		void set_ignore_gui_inset(ECS::Manager&, Instance& selfInst, bool);

		// Internal
		void set_absolute_position(Math::Vector2);
		void set_absolute_size(Math::Vector2);
		void mark_for_redraw(ECS::Manager&, Instance& selfInst);
		void clear_redraw();

		const Math::Vector2& get_absolute_position() const;
		const Math::Vector2& get_absolute_size() const;
		const Math::Vector2& get_inv_size() const;

		uint32_t get_display_order() const;
		ZIndexBehavior get_z_index_behavior() const;
		bool is_enabled() const;
		bool ignore_gui_inset() const;

		bool needs_redraw() const;
	protected:
		// Internal values
		bool m_needsRedraw = true;

		friend GuiObject;
	private:
		// Computed Data
		Math::Vector2 m_absolutePosition = {};
		Math::Vector2 m_absoluteSize;
		Math::Vector2 m_invSize; // nonstandard internal value
		// End Computed Data
		
		uint32_t m_displayOrder = 0x80'00'00'00u;
		ZIndexBehavior m_zIndexBehavior = ZIndexBehavior::SIBLING;
		bool m_enabled = true;
		bool m_ignoreGuiInset = false;

};

}

