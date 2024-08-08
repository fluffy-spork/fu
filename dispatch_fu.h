#pragma once

#include <dispatch/dispatch.h>

// NOTE(jason): shorter and easier to tell which queue when reading

void
user_async_dispatch(void (^block)(void))
{
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), block);
}

void
main_async_dispatch(void (^block)(void))
{
    dispatch_async(dispatch_get_main_queue(), block);
}

