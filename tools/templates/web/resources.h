#pragma once

#define RES_ENUM(var, E) \
    E(app_name, "fu-web-template", var) \
    E(form_submit, "do it!", var) \
    E(hello_world, "hello world!", var) \
    E(hello_table, "hello", var) \

ENUM_BLOB(res, RES_ENUM)

