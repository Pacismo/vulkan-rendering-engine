#pragma once
#include <cstdint>

#define B_OPERATOR(T1, T2, OP)                                                                                         \
    inline constexpr FieldType operator OP(T1 l, T2 r)                                                                 \
    {                                                                                                                  \
        return FieldType(FieldTypeRep(l) OP FieldTypeRep(r));                                                          \
    }
#define U_OPERATOR(T, OP)                                                                                              \
    inline constexpr FieldType operator OP(T v)                                                                        \
    {                                                                                                                  \
        return FieldType(OP FieldTypeRep(v));                                                                          \
    }

namespace engine
{
    namespace reflection
    {
        using FieldTypeRep = uint16_t;

        /// Determines the type of the field.
        ///
        /// This is a bitfield; documentation will give a hint of what the types are.
        enum class FieldTypeBits : FieldTypeRep
        {
            None = 0b0000'0000'0000'0000,

            /// Applies to:
            /// - Int
            /// - Uint
            /// - Float
            Vec1 = 0b0000'0000'0000'0000,
            /// Applies to:
            /// - Int
            /// - Uint
            /// - Float
            Vec2 = 0b0000'0000'0100'0000,
            /// Applies to:
            /// - Int
            /// - Uint
            /// - Float
            Vec3 = 0b0000'0000'1000'0000,
            /// Applies to:
            /// - Int
            /// - Uint
            /// - Float
            Vec4 = 0b0000'0000'1100'0000,

            /// Applies to:
            /// - Int
            /// - Uint
            /// - Float
            Mat_1x = Vec1,
            /// Applies to:
            /// - Int
            /// - Uint
            /// - Float
            Mat_2x = Vec2,
            /// Applies to:
            /// - Int
            /// - Uint
            /// - Float
            Mat_3x = Vec3,
            /// Applies to:
            /// - Int
            /// - Uint
            /// - Float
            Mat_4x = Vec4,

            /// Applies to:
            /// - Int
            /// - Uint
            /// - Float
            Mat_x1 = 0b0000'0000'0000'0000,
            /// Applies to:
            /// - Int
            /// - Uint
            /// - Float
            Mat_x2 = 0b0000'0001'0000'0000,
            /// Applies to:
            /// - Int
            /// - Uint
            /// - Float
            Mat_x3 = 0b0000'0010'0000'0000,
            /// Applies to:
            /// - Int
            /// - Uint
            /// - Float
            Mat_x4 = 0b0000'0011'0000'0000,

            /// Applies to:
            /// - Int
            /// - Uint
            Bits8  = 0b0000'0000'0000'0000,
            /// Applies to:
            /// - Int
            /// - Uint
            Bits16 = 0b0000'0000'0001'0000,
            /// Applies to:
            /// - Int
            /// - Uint
            /// - Float
            Bits32 = 0b0000'0000'0010'0000,
            /// Applies to:
            /// - Int
            /// - Uint
            /// - Float
            Bits64 = 0b0000'0000'0011'0000,

            TypeBits  = 0b1111,
            WidthBits = 0b11'0000,

            /// Integer types
            Int = 0b0001,

            Int8  = Bits8 | Int,
            Int16 = Bits16 | Int,
            Int32 = Bits32 | Int,
            Int64 = Bits64 | Int,

            Uint = 0b0010,

            Uint8  = Bits8 | Uint,
            Uint16 = Bits16 | Uint,
            Uint32 = Bits32 | Uint,
            Uint64 = Bits64 | Uint,

            Float = 0b0011,

            Float32 = Bits32 | Float,
            Float64 = Bits64 | Float,

            String = 0b0100,

            CString   = 0b0000'0100,
            CxxString = 0b0001'0100,

            Boolean = 0b0101,
        };

        struct FieldType
        {
            FieldTypeRep value = 0;

            inline constexpr FieldType(FieldTypeBits value)
                : value(FieldTypeRep(value))
            { }

            inline constexpr FieldType(FieldTypeRep value)
                : value(value)
            { }

            inline constexpr operator bool() const { return value != 0; }

            explicit inline constexpr operator FieldTypeBits() const { return FieldTypeBits(value); }

            explicit inline constexpr operator FieldTypeRep() const { return value; }

            inline constexpr bool contains(FieldType bits) const { return (value & bits.value) == bits.value; }
        };

        U_OPERATOR(FieldTypeBits, ~)
        U_OPERATOR(FieldType, ~)

        B_OPERATOR(FieldTypeBits, FieldTypeBits, &)
        B_OPERATOR(FieldTypeBits, FieldTypeBits, |)
        B_OPERATOR(FieldTypeBits, FieldTypeBits, ^)

        B_OPERATOR(FieldType, FieldTypeBits, &)
        B_OPERATOR(FieldType, FieldTypeBits, |)
        B_OPERATOR(FieldType, FieldTypeBits, ^)

        B_OPERATOR(FieldTypeBits, FieldType, &)
        B_OPERATOR(FieldTypeBits, FieldType, |)
        B_OPERATOR(FieldTypeBits, FieldType, ^)

        B_OPERATOR(FieldType, FieldType, &)
        B_OPERATOR(FieldType, FieldType, |)
        B_OPERATOR(FieldType, FieldType, ^)

    } // namespace reflection
} // namespace engine

#undef B_OPERATOR
#undef U_OPERATOR
