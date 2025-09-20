#ifndef DEATH_MATCH_HPP
#define DEATH_MATCH_HPP

namespace core::settings
{
    struct death_match_c
    {
        bool enabled = false;
        bool highlight_all = false;
        bool ignore_team = false;
        bool show_enemy_names = false;
    };
}

#endif // !DEATH_MATCH_HPP
