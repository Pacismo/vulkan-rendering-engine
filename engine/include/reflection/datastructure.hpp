#pragma once
#include "fieldtype.hpp"
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace engine::reflection
{
    /// Representation of a field in a datastructure
    struct Field
    {
        const char *name;
        FieldType   type;
        size_t      offset;

        inline constexpr Field(const char *name, FieldType type, size_t offset)
            : name(name)
            , type(type)
            , offset(offset)
        { }
    };

    /// Representation of a single-inheritance datastructure.
    struct Datastructure
    {
        const char          *name;
        const Field         *fields;
        size_t               field_count;
        const Datastructure *supertype;

        template<size_t F>
        inline constexpr Datastructure(const char *name, const Field (&fields)[F],
                                       const Datastructure *supertype = nullptr)
            : name(name)
            , fields(fields)
            , field_count(F)
            , supertype(supertype)
        { }
    };
} // namespace engine::reflection