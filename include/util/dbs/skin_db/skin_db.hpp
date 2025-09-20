#ifndef SKIN_DB_HPP
#define SKIN_DB_HPP

class skin_db_c
{
public:
    bool load_all( );

    int get( const std::string& skin_name ) const;
    const ankerl::unordered_dense::map<std::string, int>& get_all_skins( ) const;

private:
    ankerl::unordered_dense::map<std::string, int> skins;

    bool fetch_url_text( const std::string& url, std::string& out_text );
};

#endif // !SKIN_DB_HPP
