#pragma once
#include <imgui.h>
#include <string>
#include <string_view>

namespace engine::gui
{
    class Applet
    {
      public:
        Applet(std::string_view title, bool draw = false, bool closeable = false, ImGuiWindowFlags window_flags = 0);
        virtual ~Applet();

        /// Draw the window to the screen.
        ///
        /// By default, gives the window:
        ///  - The title
        ///  - A closing widget, if enabled
        ///  - The provided flags
        ///
        /// The window is drawn if the visible flag is `true`.
        ///
        /// Overriding should be done if additional changes must be made to the window
        virtual void draw(ImGuiViewport *viewport = ImGui::GetMainViewport());

        void visible(bool);
        bool visible() const;

        void closeable(bool);
        bool closeable() const;

        void             flags(ImGuiWindowFlags);
        ImGuiWindowFlags flags() const;

        void add_flags(ImGuiWindowFlags);
        void remove_flags(ImGuiWindowFlags);

        void             title(std::string_view new_title);
        std::string_view title() const;

        operator bool &();

      protected:
        virtual void populate(ImGuiViewport *viewport);

      private:
        std::string      m_title;
        ImGuiWindowFlags m_flags;
        bool             m_closeable;
        bool             m_visible;
    };
} // namespace engine::gui