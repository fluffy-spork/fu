#pragma once

// should this really be a separate file?

typedef enum {
    NONE_FIELD_TYPE,
    BLOB_FIELD_TYPE,
    INTEGER_FIELD_TYPE,
    FLOAT_FIELD_TYPE,
    DATE_FIELD_TYPE,
} field_type_t;

typedef struct {
    field_type_t type;
    blob_t * name;
    blob_t * label;
    u16 max_size;
} field_t;

field_t *
field(field_type_t type, blob_t * name, blob_t * label, u16 max_size)
{
    field_t * field = malloc(sizeof(field_t));
    if (field) {
        *field = (field_t){
            .type = type,
            .name = name,
            .label = label,
            .max_size = max_size
        };
    }

    return field;
}

