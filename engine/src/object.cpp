#include "object.hpp"

using namespace engine::reflection;

namespace engine
{
    static const Field OBJECT_FIELDS[] = {
        Field("location", FieldTypeBits::Float32 | FieldTypeBits::Vec3, offsetof(Object, transform.location)),
        Field("rotation", FieldTypeBits::Float32 | FieldTypeBits::Vec3, offsetof(Object, transform.rotation)),
        Field("scale", FieldTypeBits::Float32 | FieldTypeBits::Vec3, offsetof(Object, transform.scale)),
        Field("name", FieldTypeBits::String, offsetof(Object, name)),
    };

    const Datastructure OBJECT_REP = Datastructure("Object", OBJECT_FIELDS);

    Object::~Object() = default;

    void Object::process(double delta) { }

    void Object::physics_process(double delta) { }

    const ::engine::reflection::Datastructure *Object::get_rep() const
    {
        return &OBJECT_REP;
    }
} // namespace engine