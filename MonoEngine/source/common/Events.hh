#pragma once
#include <common/Base.hh>
#include <events/Event.hh>

namespace Monoworks 
{

		// non-data events

		/// @brief Triggered when the window is closed.
		MW_DEFINE_DISPATCHABLE_EVENT( WindowClose );

		/// @brief Triggered when the window gains focus.
		MW_DEFINE_DISPATCHABLE_EVENT( WindowFocus );

		/// @brief Triggered when the window loses focus.
		MW_DEFINE_DISPATCHABLE_EVENT( WindowLostFocus );

		/// @brief Triggered for each application tick.
		MW_DEFINE_DISPATCHABLE_EVENT( AppTick );
		
		/// @brief Triggered when the application updates its logic.
		MW_DEFINE_DISPATCHABLE_EVENT( AppUpdate );

		/// @brief Triggered when the application renders a frame.
		MW_DEFINE_DISPATCHABLE_EVENT( AppRender );
		
		/// @brief Triggered when the application starts an entire frame cycle (before deffered event-processing).
		MW_DEFINE_DISPATCHABLE_EVENT( AppFrame );


		namespace Events
		{
		// data events 

		// window

		/// @brief Triggered when the window minimize state changes.
		struct SWindowMinimize
		{
			/// @brief True if the window is minimized, false if restored.
			bool		Minimized = false;
		};

		/// @brief Triggered when the window is resized.
		struct SWindowResize
		{
			/// @brief The new window dimensions.
			SExtent2D	NewExtent;
		};
#
		// renderer

		/// @brief Triggered when viewport/renderable extent space changes
		struct SViewportResize
		{
			/// @brief The new viewport dimensions.
			SExtent2D	NewExtent;
		};

		// keyboard

		/// @brief Triggered when a keyboard key is pressed.
		struct SKeyPressed
		{
			/// @brief The scancode of the pressed key.
			Scancode	Code;
			/// @brief The amount of times the key event has repeated.
			s32			RepeatCount;
		};

		/// @brief Triggered when a keyboard key is released.
		struct SKeyReleased
		{
			/// @brief The scancode of the released key.
			Scancode	Code;
		};

		/// @brief Triggered when a character is typed.
		struct SKeyTyped
		{
			/// @brief The character code that was typed.
			Scancode	Code;
		};

		// mouse

		/// @brief Triggered when a mouse button is pressed.
		struct SMouseButtonPressed
		{
			/// @brief The mouse button that was pressed.
			Mousebutton Button;

			/// @brief The X position of the mouse cursor.
			f32			MouseX;
			/// @brief The Y position of the mouse cursor.
			f32			MouseY;
		};

		/// @brief Triggered when a mouse button is released.
		struct SMouseButtonReleased
		{
			/// @brief The mouse button that was released.
			Mousebutton Button;

			/// @brief The X position of the mouse cursor.
			f32			MouseX;
			/// @brief The Y position of the mouse cursor.
			f32			MouseY;
		};

		/// @brief Triggered when the mouse cursor is moved.
		struct SMouseMoved
		{
			/// @brief The new X position of the mouse cursor.
			f32			MouseX;
			/// @brief The new Y position of the mouse cursor.
			f32			MouseY;
		};

		/// @brief Triggered when the mouse wheel is scrolled.
		struct SMouseScrolled
		{
			/// @brief The scroll offset along the X axis.
			f32			XOffset;
			/// @brief The scroll offset along the Y axis.
			f32			YOffset;
		};


	}


}

