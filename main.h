#ifndef TPYTHON_H_
#define TPYTHON_H_

#define introspect
#define array_count(arr) sizeof(arr)/sizeof(arr[0])

//TODO remove dependency look into std::string performance potentially remove dependancy
//or make useage more performant and idomatic

struct PythonPath {
        char path_buffer[2048];
        char *file_part;
        size_t length_from_file_part;
};


#endif // TPYTHON_H_
