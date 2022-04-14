# mainframe

![mainframe](docs/mainframe-medium.jpg)

mainframe is a C++ dataframe library. It features C++-ish container ergonomics like iterators, copy-on-write column data management, and a flexible expression syntax that allows for efficient row filtering and column population. 

mainframe supports flexible data of nearly any type for columns - as long as they are default constructible

        frame<year_month_day, double, bool> f1;
        f1.set_column_names( "date", "temperature", "rain" );
        f1.push_back( 2022_y/January/2, 10.0, false );
        f1.push_back( 2022_y/January/3, 11.1, true );
        f1.push_back( 2022_y/January/4, 12.2, false );
        f1.push_back( 2022_y/January/5, 13.3, false );
        f1.push_back( 2022_y/January/6, 14.4, true );
        f1.push_back( 2022_y/January/7, 15.5, false );
        f1.push_back( 2022_y/January/8, 9.1, true );

mainframe supports an expression format that allows for simple row filtering. Here, `fcoldandrain` is a new dataframe that will contain a single row for `2022/January/8`:

         frame<year_month_day, double, bool> fcoldandrain = f1.rows( _1 <= 12 && _2 == true && _0 > 2022_y/January/4 );

Expressions also work well for creating new data columns

        auto f2 = f1.new_series<year_month_day>( "next year", _0 + years(1) );

