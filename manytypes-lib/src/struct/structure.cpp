#include "manytypes-lib/types/structure.h"
#include "manytypes-lib/db/database.h"

bool structure_t::add_field( const base_field_t& field )
{
    const auto overlap = std::ranges::any_of(
        s_fields,
        [&field] ( auto& s_field )
        {
            if ( s_field.bit_offset >= field.bit_offset &&
                s_field.bit_offset + s_field.bit_size > field.bit_offset )
                return true;

            return false;
        } );

    if ( overlap )
        return false;

    s_fields.push_back( field );
    return true;
}

std::vector<type_id> structure_t::get_dependencies( )
{
    std::vector<type_id> dependencies;
    dependencies.reserve( s_fields.size( ) );

    for ( auto& member : s_fields )
        dependencies.push_back( member.type_id );

    return dependencies;
}