# EXPERIMENTAL Unconventional C Library and SQLite Web/App Framework

A new C library without using existing conventions or standards.

Nothing is sacred.

## Getting Started

Documentation is extremely poor.


```
mkdir new-web-project && cd new-web-project
git init
git submodule add https://github.com/fluffy-spork/fu
# basic hello world
./fu/tools/jcc init-web
# builds appimage and runs in dev mode
./jcc

curl http://localhost:8080
```

In the project I'm building with fu, it runs with systemd as a service

/etc/systemd/system/<app>.server
```
[Unit]
Description=app
After=network.target auditd.service

[Service]
ExecStart=/usr/local/sbin/app-x86_64.AppImage $STATE_DIRECTORY
User=app-user
Group=app-user
StateDirectory=app
StateDirectoryMode=0700

[Install]
WantedBy=multi-user.target
```

## Goals

* Linux ONLY
  * cross platform just adds pointless complexity.  If people want to use non-linux that
    can use Windows Subsystem for Linux or a VM

* Make it easy to create a new app

* Build and run in under 1 second
  * building the AppImage can take a few seconds longer on slow machines, but
    isn't required during dev

* Startup fast and kill fast.  Assume non-clean shutdown in all cases.  You
  have to support it anyway.
  * should be able to run on a cheaper spot instance and have the app restarted
    every night or something.  There's a lot of cases where this is fine as
    long as it restarts when killed.

* Have all the source code available and build everything always
  * makes it easy to try changes/fix bugs in framework.

* embedded pre-fork web server
  * only implements what's needed which is relatively small
  * forked processes mean each request is isolated for user data and crashing
  * (TODO) include https support and letsencrypt so there's no manual work to
    get https working.  Will initially link to standard linux library like
    openssl, but eventually include from scratch only https support in fu
  * Currently use haproxy for https and http redirect to https
  * file upload by direct POST from javascript.  avoids encoding, etc.
    currently depends on ffmpeg for media processing.

* generate HTML with function calls
  * No templating
  * easy to create pages that look similar
  * don't have to learn another language
  * methods are type checked, etc at build
  * normal usage won't requiring typing any HTML
  * don't necessarily have to even know HTML is most cases
  * (TODO) integrate more standard HTML gen functions from fluffy

* (TODO) generate CSS with function calls
  * integrate existing CSS styles from fluffy

* Minimal javascript
  * (TODO) integrate javascript for file upload from fluffy

* Build to AppImage for deploy
  * everything is included like web resources that are directly served from the
    AppImage
  * Probably primarily for running "desktop apps" that use a web browser for UI
  * (TODO) implement the AppImage creation instead of depending on an included
    binary

* (TODO) Build to docker for multi-instance SaaS deployment

* No load balancing or failover
  * From my experience it has been a lot of effort, overhead, and comlexity for
    almost never being used
  * targetting multi-instance SaaS instead of multi-tenant monolith for load
  * machines are now huge!!!  Just don't worry about fluctuating load.

* BYOS: Bring Your Own SQLite
  * SQLite is required, but is built at the app level so the dev can always use
    the version they need.
  * exception to the build everything always goal as SQLite is infrequently
    changed and slow to build

* (TODO) real time SQLite backup off server

* Easy DB interface
  * currently there are many different experiments.  Some will be removed/changed
  * easy db upgrades in normal cases
  * likely move to external sql files that are all compiled to prepared
    statements at startup.  Allows checking syntax and table/column names
    without having to run the actual page, etc.


## Conventions (incomplete)
* conventions are not pervasively used and some things need to be updated

* Format code with the most important information the first thing on
  the line for readability

* thing_t for typedef, thing_e for enums, thing_s for non-typedef's structs

* Names should have the most important part first with the namespace a suffix

    for blob_t use add_blob() instead of blob_add()

* function definitions have the return value above the name so the name is first

```
    int
    add_blob(blob_t * b)
    {
        return 0;
    }
```

* try not to use macros

* use X-Macros for enums with names (ENUM_BLOB) and some other cases

    https://en.wikipedia.org/wiki/X_Macro

* for most functions, use an int return type of 0 success, non-zero for failure
  with errno set or a similar error value on the functions main struct

* Sometimes OR'ing functions with error returns is used to consolidate error handling.
  Not sure how good of an idea this is, but makes code shorter.  requires functions to
  log their own errors which seems generally useful

```
    if (foo()
        || bar()
        || baz())
    {
        log error
        return -1;
    }
```

