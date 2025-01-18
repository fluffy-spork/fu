#pragma once

// No longer active endpoint should update the implementation to not
// implemented any more or whatever other useful error.  redirect to a new
// implementation.  nothing should be removed from the ENDPOINT_TABLE

// NOTE(jason): Use GET form and POST form to same endpoint so that on error it shows the
// form again and on success redirects to get of the form with an id or a
// different page.

#define ENDPOINT_TABLE(E) \
    E("/unknown", unknown, "unknown", method_not_implemented_handler, NULL) \
    E("/", home, "home", home_handler, NULL) \
    E("/res/*", res, "resources", res_handler, NULL) \
    E("/res/main.css", css, "css", css_handler, NULL) \
    E("/favicon.ico", favicon, "favicon", favicon_handler, NULL) \
    E("/hello", hello, "hello", hello_handler, fields.msg) \


// TODO(jason): should the handler implementations be here too???
// would require the endpoints being defined before they can be created

