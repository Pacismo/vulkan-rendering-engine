#include "gui/applet.hpp"

namespace engine::gui
{
    Applet::Applet(std::string_view title, bool visible, bool closeable, ImGuiWindowFlags window_flags)
        : m_title(title)
        , m_visible(visible)
        , m_closeable(closeable)
        , m_flags(window_flags)
    { }

    Applet::~Applet() { }

    void Applet::draw(ImGuiViewport *viewport)
    {
        if (ImGui::Begin(m_title.c_str(), m_closeable ? &m_visible : nullptr, m_flags))
            populate(viewport);
        ImGui::End();
    }

    void Applet::visible(bool v)
    {
        m_visible = v;
    }

    bool Applet::visible() const
    {
        return m_visible;
    }

    void Applet::closeable(bool v)
    {
        m_closeable = v;
    }

    bool Applet::closeable() const
    {
        return m_closeable;
    }

    void Applet::flags(ImGuiWindowFlags v)
    {
        m_flags = v;
    }

    ImGuiWindowFlags Applet::flags() const
    {
        return m_flags;
    }

    void Applet::add_flags(ImGuiWindowFlags flags) {
        m_flags |= flags;
    }

    void Applet::remove_flags(ImGuiWindowFlags flags) {
        m_flags &= ~flags;
    }

    void Applet::title(std::string_view new_title)
    {
        m_title = new_title;
    }

    std::string_view Applet::title() const
    {
        return m_title;
    }

    Applet::operator bool &()
    {
        return m_visible;
    }

    void Applet::populate(ImGuiViewport *viewport) { }
} // namespace engine::gui