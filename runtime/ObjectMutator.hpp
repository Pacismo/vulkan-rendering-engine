#pragma once
#include <gui/applet.hpp>
#include <object.hpp>
#include <window.hpp>

class ObjectMutator final : public engine::gui::Applet
{
    using ObjectsPtr = std::vector<std::shared_ptr<engine::Object>> *;

  public:
    ObjectMutator(std::vector<std::shared_ptr<engine::Object>> &objects);
    ~ObjectMutator();

    void next();
    void prev();

  protected:
    void populate(ImGuiViewport *viewport) override;

  private:
    ObjectsPtr m_objects;
    size_t     m_index;
};