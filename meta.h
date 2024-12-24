#ifndef META_H_
#define META_H_
#include <stdint.h>
#define introspect

enum MetaType {
        TYPE_int,
        TYPE_float,
        TYPE_bool,
        TYPE_AstNode_PTR,
        TYPE_TypeInfo_PTR,
        TYPE_TypeInfoType,
        TYPE_SymbolTableEntry_PTR,
};

struct StructMemberDefinition {
        MetaType type;
        const char *name;
        uint64_t offset;
};

struct EnumMemberDefinition {
        const char *name;
        int value;
};

#endif // META_H_
