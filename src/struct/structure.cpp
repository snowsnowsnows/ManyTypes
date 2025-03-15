#include "struct/structure.h"
#include "database.h"

bool structure_t::add_field( const base_field_t& field )
{
    if ( !is_field_valid( field ) )
        return false;

    s_fields.push_back( field );
    return true;
}

bool structure_t::is_field_valid( const base_field_t& next_field )
{
    // cannot be an array and bitfield
    if ( next_field.is_array && next_field.is_bit_field )
        return false;

    // verify size of type
    {
        uint32_t type_size = type_database_t::get( ).size_of( next_field.type_id );
        if ( next_field.is_array )
            type_size *= next_field.arr_size;

        if ( type_size != next_field.bit_size )
            return false;
    }

    if ( !is_union )
    {
        if ( in_order_insert )
        {
            // verify that next from previous structure
            bool overlap_check = true;
            if ( !s_fields.empty( ) )
            {
                const base_field_t& back = s_fields.back( );
                const auto last_avail = back.bit_offset + back.bit_size;

                overlap_check = last_avail <= next_field.bit_offset;
            }

            return overlap_check;
        }

        // verify no overlap by checking all fields
        auto overlap_check = !std::ranges::any_of( this->s_fields, [&next_field] ( const base_field_t& field )
        {
            if ( field.bit_offset <= next_field.bit_offset &&
                field.bit_offset + field.bit_size > next_field.bit_offset )
                return true;

            return false;
        } );

        return overlap_check;
    }

    return true;
}

size_t structure_t::size_of( )
{
}
