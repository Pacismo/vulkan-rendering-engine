#include "ObjectMutator.hpp"
#include <fmt/format.h>
#include <imgui_stdlib.h>
#include <reflection/datastructure.hpp>

using engine::Object;
using namespace engine::reflection;

ObjectMutator::ObjectMutator(std::vector<std::shared_ptr<engine::Object>> &objects)
    : Applet("Object Mutator", false, true)
    , m_objects(&objects)
    , m_index(0)
{ }

ObjectMutator::~ObjectMutator() = default;

void ObjectMutator::next()
{
    m_index = (m_index + 1) % m_objects->size();
}

void ObjectMutator::prev()
{
    m_index = (m_index - 1) % m_objects->size();
}

static std::string get_object_ptr_str(Object &obj)
{
    return fmt::format("0x{:016X}", (size_t)&obj);
}

static std::string get_object_name(Object &obj)
{
    if (obj.name.empty())
        return get_object_ptr_str(obj);
    else
        return obj.name;
}

void field_mutator(Object *p_obj, const Field &field)
{
    uint8_t *pfield = ((uint8_t *)p_obj) + field.offset;

    switch (FieldTypeBits(field.type & (FieldTypeBits::TypeBits | FieldTypeBits::WidthBits))) {
    case FieldTypeBits::Int8:
        break;
    case FieldTypeBits::Int16:
        break;
    case FieldTypeBits::Int32:
        break;
    case FieldTypeBits::Int64:
        break;

    case FieldTypeBits::Uint8:
        break;
    case FieldTypeBits::Uint16:
        break;
    case FieldTypeBits::Uint32:
        break;
    case FieldTypeBits::Uint64:
        break;

    case FieldTypeBits::Float32: {
        if (field.type.contains(FieldTypeBits::Vec4))
            ImGui::DragFloat4(field.name, (float *)pfield, 1.0);
        else if (field.type.contains(FieldTypeBits::Vec3))
            ImGui::DragFloat3(field.name, (float *)pfield, 1.0);
        else if (field.type.contains(FieldTypeBits::Vec2))
            ImGui::DragFloat2(field.name, (float *)pfield, 1.0);
        else
            ImGui::DragFloat(field.name, (float *)pfield, 1.0);
        break;
    }
    case FieldTypeBits::Float64: {
        std::string s;
        float      *f = (float *)pfield;

        if (field.type.contains(FieldTypeBits::Vec4))
            s = fmt::format("[{:0.3f}, {:0.3f}, {:0.3f}, {:0.3f}]", f[0], f[1], f[2], f[3]);
        else if (field.type.contains(FieldTypeBits::Vec3))
            s = fmt::format("[{:0.3f}, {:0.3f}, {:0.3f}]", f[0], f[1], f[2]);
        else if (field.type.contains(FieldTypeBits::Vec2))
            s = fmt::format("[{:0.3f}, {:0.3f}]", f[0], f[1]);
        else
            s = fmt::format("[{:0.3f}]", f[0]);

        ImGui::Text("%s", s.c_str());
        break;
    }
    case FieldTypeBits::String:
        ImGui::InputText(field.name, (std::string *)pfield);
        break;
    case FieldTypeBits::Boolean:
        ImGui::Checkbox(field.name, (bool *)pfield);
        break;

    default:
        break;
    }
}

void ObjectMutator::populate(ImGuiViewport *viewport)
{
    if (!m_objects)
        return;
    if (m_index >= m_objects->size())
        m_index = 0;
    if (m_objects->size() == 0)
        return;

    std::string obj_name = "";
    std::string obj_ptr  = "";
    if (m_index < m_objects->size())
        obj_name = get_object_name(*m_objects->at(m_index)), obj_ptr = get_object_ptr_str(*m_objects->at(m_index));

    if (ImGui::BeginCombo("Object", obj_name.c_str())) {
        for (size_t i = 0; i < m_objects->size(); ++i) {
            Object &obj = *m_objects->at(i);

            obj_name = get_object_name(obj);
            obj_ptr  = get_object_ptr_str(obj);

            if (ImGui::Selectable(obj_name.c_str(), m_index == i))
                m_index = i;
            ImGui::SetItemTooltip("%s", obj_ptr.c_str());

            if (m_index == i)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("%s", obj_ptr.c_str());

    auto ds = m_objects->at(m_index)->get_rep();
    while (ds) {
        if (ImGui::CollapsingHeader(ds->name))
            for (size_t i = 0; i < ds->field_count; ++i)
                field_mutator(m_objects->at(m_index).get(), ds->fields[i]);

        ds = ds->supertype;
    }

    return;
}
