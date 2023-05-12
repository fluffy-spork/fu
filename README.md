# fu

Experimental Unconventional C Library and Web/App Framework

Goal
Write a new C library without requiring the use of existing conventions or
standards.

See [fu-web-template](https://github.com/fluffy-spork/fu-web-template) for an example.

Conventions (incomplete)
* conventions are not pervasively used and some things need to be updated

* Format code with the most important information the first thing on
  the line for readability

* thing_t for typedef, thing_e for enums, thing_s for non-typedef's structs

* Names should have the most important part first with the namespace a suffix

    add_blob() instead of blob_add()

* function definitions have the return value above the name so the name is first

    int
    add_blob(blob_t * b)
    {
    }

* try not to use macros

* use X-Macros for enums with names (ENUM_BLOB) and some other cases

    https://en.wikipedia.org/wiki/X_Macro

* for most functions, use an int return type of 0 success, non-zero for failure
  with errno set or a similar error value on the functions main struct

* Sometimes OR'ing functions with error returns is used to consolidate error handling.
  Not sure how good of an idea this is, but makes code shorter.  requires functions to
  log their own errors which seems generally useful

    if (foo()
        || bar()
        || baz())
    {
        log error
        return -1;
    }

