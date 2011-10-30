# JsonSpirit

JsonSpirit tries to implement a very script (read: ruby, python, perl) like interface to json 
structured data without loosing type safety.

It is build around a single type (json) that provides functions to parse (e.g. from_string) or 
serialize (e.g. to_string) as well as querying the type and converting the type if neccessary.

No external syntax file or parser generator is used, the parser is written in C++ useing the
awesome spirit library.

